// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "Mass/Minions/Processors/SWMinionLaneMovementProcessor.h"

#include "Mass/Minions/SWMinionLaneWaveSubsystem.h"
#include "Mass/Minions/SWMinionMassTypes.h"
#include "Engine/World.h"
#include "MassCommonFragments.h"
#include "MassCommonTypes.h"
#include "MassEntityManager.h"
#include "MassExecutionContext.h"
#include "MassSignalSubsystem.h"
#include "MassStateTreeTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SWMinionLaneMovementProcessor)

USWMinionLaneMovementProcessor::USWMinionLaneMovementProcessor()
	: EntityQuery(*this)
{
	// 由 USWMinionLaneWaveSubsystem 按 World 生命周期显式动态注册。
	bAutoRegisterWithProcessingPhases = false;
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::Standalone | EProcessorExecutionFlags::Server);
	ProcessingPhase = EMassProcessingPhase::PrePhysics;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Movement;
	// 从 WorldSubsystem 读取冻结快照，明确限制为 Game Thread；纯 Fragment 算法后续可再拆分并行。
	bRequiresGameThreadExecution = true;
}

void USWMinionLaneMovementProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FSWMinionLaneFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FSWMinionSpatialFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FSWMinionIntentFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FSWMinionTargetFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FSWMinionCombatFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FSWMinionLeashFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddConstSharedRequirement<FSWMinionArchetypeSharedFragment>();
	EntityQuery.AddTagRequirement<FSWMinionReadyTag>(EMassFragmentPresence::All);
	EntityQuery.AddTagRequirement<FSWMinionDeadTag>(EMassFragmentPresence::None);
}

void USWMinionLaneMovementProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UWorld* const World = EntityManager.GetWorld();
	USWMinionLaneWaveSubsystem* const LaneWaveSubsystem = World ? World->GetSubsystem<USWMinionLaneWaveSubsystem>() : nullptr;
	UMassSignalSubsystem* const SignalSubsystem = World ? World->GetSubsystem<UMassSignalSubsystem>() : nullptr;
	if (!LaneWaveSubsystem)
	{
		return;
	}

	const float DeltaSeconds = FMath::Max(0.f, Context.GetDeltaTimeSeconds());
	EntityQuery.ForEachEntityChunk(Context, [LaneWaveSubsystem, SignalSubsystem, DeltaSeconds](FMassExecutionContext& ChunkContext)
	{
		const FSWMinionArchetypeSharedFragment& Archetype = ChunkContext.GetConstSharedFragment<FSWMinionArchetypeSharedFragment>();
		TArrayView<FTransformFragment> Transforms = ChunkContext.GetMutableFragmentView<FTransformFragment>();
		TArrayView<FSWMinionLaneFragment> Lanes = ChunkContext.GetMutableFragmentView<FSWMinionLaneFragment>();
		TArrayView<FSWMinionSpatialFragment> Spatials = ChunkContext.GetMutableFragmentView<FSWMinionSpatialFragment>();
		TArrayView<FSWMinionIntentFragment> Intents = ChunkContext.GetMutableFragmentView<FSWMinionIntentFragment>();
		const TConstArrayView<FSWMinionTargetFragment> Targets = ChunkContext.GetFragmentView<FSWMinionTargetFragment>();
		const TConstArrayView<FSWMinionCombatFragment> Combats = ChunkContext.GetFragmentView<FSWMinionCombatFragment>();
		TArrayView<FSWMinionLeashFragment> Leashes = ChunkContext.GetMutableFragmentView<FSWMinionLeashFragment>();

		for (FMassExecutionContext::FEntityIterator EntityIt = ChunkContext.CreateEntityIterator(); EntityIt; ++EntityIt)
		{
			FSWMinionLaneFragment& Lane = Lanes[EntityIt];
			FSWMinionSpatialFragment& Spatial = Spatials[EntityIt];
			FSWMinionIntentFragment& Intent = Intents[EntityIt];

			if (Lane.Direction == ESWLaneDirection::None)
			{
				Intent.DesiredVelocity = FVector::ZeroVector;
				LaneWaveSubsystem->ReportLaneSamplingFailureOnce(Lane.LaneId);
				continue;
			}

			auto ApplyLaneOffsets = [&Lane, &Spatial](FTransform& InOutTransform)
			{
				const FVector LateralOffset = InOutTransform.TransformVectorNoScale(FVector(0.f, Lane.LateralOffset, 0.f));
				InOutTransform.AddToTranslation(LateralOffset + FVector::UpVector * (Lane.VerticalOffset + Lane.GroundOffset) + Spatial.SeparationOffset);
			};

			if (Intent.Behavior == ESWMinionBehaviorIntent::Engaging)
			{
				const AActor* const TargetActor = Targets[EntityIt].TargetActor.Get();
				if (!IsValid(TargetActor))
				{
					Intent.DesiredVelocity = FVector::ZeroVector;
					continue;
				}

				FTransform& CurrentTransform = Transforms[EntityIt].GetMutableTransform();
				const FVector CurrentLocation = CurrentTransform.GetLocation();
				FVector TargetLocation = TargetActor->GetActorLocation();
				TargetLocation.Z = CurrentLocation.Z;
				const FVector ToTarget = TargetLocation - CurrentLocation;
				const FVector DirectDirectionToTarget = ToTarget.GetSafeNormal2D();
				if (DirectDirectionToTarget.IsNearlyZero())
				{
					Intent.DesiredVelocity = FVector::ZeroVector;
					continue;
				}

				// Separation 产生的偏移说明此单位刚与邻近小兵发生空间接触。追击时将它
				// 转成相对于目标方向的侧移偏好，使后排兵能从友军身边通过而不是一直顶住。
				// 它仍然始终具有朝向目标的前向分量，因此不会把战斗移动变成自由漫游。
				FVector DirectionToTarget = DirectDirectionToTarget;
				const FVector SeparationDirection = Spatial.SeparationOffset.GetSafeNormal2D();
				if (!SeparationDirection.IsNearlyZero() && Archetype.CombatDetourSteeringStrength > 0.f)
				{
					const float CrossZ = DirectDirectionToTarget.X * SeparationDirection.Y - DirectDirectionToTarget.Y * SeparationDirection.X;
					// 前后完全排队时 Separation 可能刚好与目标方向共线。此时用稳定 Entity
					// 哈希选择左右，避免所有后排继续沿同一条线顶住前排。
					const float SideSign = FMath::IsNearlyZero(CrossZ)
						? ((GetTypeHash(ChunkContext.GetEntity(EntityIt)) & 1u) == 0u ? -1.f : 1.f)
						: FMath::Sign(CrossZ);
					const FVector SideStepDirection(-DirectDirectionToTarget.Y, DirectDirectionToTarget.X, 0.f);
					DirectionToTarget = (DirectDirectionToTarget + SideStepDirection * SideSign * Archetype.CombatDetourSteeringStrength).GetSafeNormal2D();
				}

				// 追击不能以目标中心为终点。攻击距离是目标选择、StateTree 转换和攻击 GA
				// 共同使用的停靠边界；越过它会让两名小兵视觉重叠，并在低频索敌间隔内抖动。
				const float DistanceToTarget = ToTarget.Size2D();
				// StateTree、Attack Processor 与 Character 均以 AttackRange 判定合法攻击，
				// 因此移动不得刚好停在边界。预留内侧余量可吸收同帧分离推开与浮点误差，
				// 防止出现视觉上已接敌、但范围快照在 250cm 边界来回翻转的盲区。
				const float AttackStopDistance = FMath::Max(0.f, Combats[EntityIt].AttackRange - Archetype.AttackRangeArrivalBuffer);
				const float RemainingDistance = FMath::Max(0.f, DistanceToTarget - AttackStopDistance);
				if (RemainingDistance <= KINDA_SMALL_NUMBER)
				{
					CurrentTransform.SetRotation(FRotationMatrix::MakeFromXZ(DirectDirectionToTarget, FVector::UpVector).ToQuat());
					Intent.DesiredVelocity = FVector::ZeroVector;
					continue;
				}

				const float StepDistance = FMath::Min(RemainingDistance, Archetype.MoveSpeed * DeltaSeconds);
				CurrentTransform.SetLocation(CurrentLocation + DirectionToTarget * StepDistance);
				CurrentTransform.SetRotation(FRotationMatrix::MakeFromXZ(DirectionToTarget, FVector::UpVector).ToQuat());
				Intent.DesiredVelocity = DirectionToTarget * Archetype.MoveSpeed;
				continue;
			}

			if (Intent.Behavior == ESWMinionBehaviorIntent::Returning)
			{
				FTransform& CurrentTransform = Transforms[EntityIt].GetMutableTransform();
				if (Leashes[EntityIt].bNeedsForwardLaneRejoinProjection)
				{
					float ProjectedLaneDistance = 0.f;
					if (!LaneWaveSubsystem->TryProjectLaneDistance(Lane.LaneId, CurrentTransform.GetLocation(), ProjectedLaneDistance))
					{
						Intent.DesiredVelocity = FVector::ZeroVector;
						LaneWaveSubsystem->ReportLaneSamplingFailureOnce(Lane.LaneId);
						continue;
					}

					// 只在进入 Returning 的边沿投影一次。此后 DistanceAlongLane 仍沿原方向增长，
					// 因而小兵会继续向前并横向收敛，而不会倒退到最初的交战锚点。
					Lane.DistanceAlongLane = ProjectedLaneDistance;
					Leashes[EntityIt].bNeedsForwardLaneRejoinProjection = false;
				}

				float LaneLength = 0.f;
				if (!LaneWaveSubsystem->TryGetLaneLength(Lane.LaneId, LaneLength))
				{
					Intent.DesiredVelocity = FVector::ZeroVector;
					LaneWaveSubsystem->ReportLaneSamplingFailureOnce(Lane.LaneId);
					continue;
				}

				const float SignedDistanceDelta = (Lane.Direction == ESWLaneDirection::Forward ? 1.f : -1.f) * Archetype.MoveSpeed * DeltaSeconds;
				Lane.DistanceAlongLane = FMath::Clamp(Lane.DistanceAlongLane + SignedDistanceDelta, 0.f, LaneLength);

				Spatial.SeparationOffset = FMath::VInterpConstantTo(
					Spatial.SeparationOffset,
					FVector::ZeroVector,
					DeltaSeconds,
					Archetype.SeparationRelaxationSpeed);

				FTransform ForwardLaneTransform;
				if (!LaneWaveSubsystem->TrySampleLaneTransform(Lane.LaneId, Lane.DistanceAlongLane, Lane.Direction, ForwardLaneTransform))
				{
					Intent.DesiredVelocity = FVector::ZeroVector;
					LaneWaveSubsystem->ReportLaneSamplingFailureOnce(Lane.LaneId);
					continue;
				}

				ApplyLaneOffsets(ForwardLaneTransform);
				const FVector CurrentLocation = CurrentTransform.GetLocation();
				const FVector ToForwardLane = ForwardLaneTransform.GetLocation() - CurrentLocation;
				const float ArrivalTolerance = FMath::Max(0.f, Archetype.LaneRejoinTolerance);
				if (ToForwardLane.SizeSquared2D() <= FMath::Square(ArrivalTolerance))
				{
					const bool bJustReachedReturnAnchor = !Intent.bReachedReturnAnchor;
					CurrentTransform = ForwardLaneTransform;
					Intent.DesiredVelocity = FVector::ZeroVector;
					Intent.bReachedReturnAnchor = true;
					// Returning 不参与索敌，StateTree 也不会为该实体逐帧调度。
					// 因此必须在完成回线的边沿显式唤醒它，才能消费
					// Returning -> Advancing 的 OnTick 条件。
					if (bJustReachedReturnAnchor && SignalSubsystem)
					{
						SignalSubsystem->SignalEntityDeferred(ChunkContext, UE::Mass::Signals::NewStateTreeTaskRequired, ChunkContext.GetEntity(EntityIt));
					}
					continue;
				}

				const FVector DirectionToForwardLane = ToForwardLane.GetSafeNormal2D();
				const float RejoinSpeed = Archetype.MoveSpeed * FMath::Max(1.f, Archetype.LaneRejoinSpeedMultiplier);
				const float StepDistance = FMath::Min(ToForwardLane.Size2D(), RejoinSpeed * DeltaSeconds);
				CurrentTransform.SetLocation(CurrentLocation + DirectionToForwardLane * StepDistance);
				CurrentTransform.SetRotation(FRotationMatrix::MakeFromXZ(DirectionToForwardLane, FVector::UpVector).ToQuat());
				Intent.DesiredVelocity = DirectionToForwardLane * RejoinSpeed;
				Intent.bReachedReturnAnchor = false;
				continue;
			}

			if (Intent.Behavior != ESWMinionBehaviorIntent::Advancing)
			{
				Intent.DesiredVelocity = FVector::ZeroVector;
				continue;
			}
			Intent.bReachedReturnAnchor = false;
			Spatial.SeparationOffset = FMath::VInterpConstantTo(
				Spatial.SeparationOffset,
				FVector::ZeroVector,
				DeltaSeconds,
				Archetype.SeparationRelaxationSpeed);

			float LaneLength = 0.f;
			if (!LaneWaveSubsystem->TryGetLaneLength(Lane.LaneId, LaneLength))
			{
				Intent.DesiredVelocity = FVector::ZeroVector;
				LaneWaveSubsystem->ReportLaneSamplingFailureOnce(Lane.LaneId);
				continue;
			}

			const float SignedDistanceDelta = (Lane.Direction == ESWLaneDirection::Forward ? 1.f : -1.f) * Archetype.MoveSpeed * DeltaSeconds;
			Lane.DistanceAlongLane = FMath::Clamp(Lane.DistanceAlongLane + SignedDistanceDelta, 0.f, LaneLength);

			FTransform LaneTransform;
			if (!LaneWaveSubsystem->TrySampleLaneTransform(Lane.LaneId, Lane.DistanceAlongLane, Lane.Direction, LaneTransform))
			{
				Intent.DesiredVelocity = FVector::ZeroVector;
				LaneWaveSubsystem->ReportLaneSamplingFailureOnce(Lane.LaneId);
				continue;
			}

			ApplyLaneOffsets(LaneTransform);
			Transforms[EntityIt].GetMutableTransform() = LaneTransform;
			const bool bReachedRouteEnd = Lane.Direction == ESWLaneDirection::Forward
				? FMath::IsNearlyEqual(Lane.DistanceAlongLane, LaneLength)
				: FMath::IsNearlyZero(Lane.DistanceAlongLane);
			Intent.DesiredVelocity = bReachedRouteEnd ? FVector::ZeroVector : LaneTransform.GetUnitAxis(EAxis::X) * Archetype.MoveSpeed;
		}
	});
}
