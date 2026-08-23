// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "Mass/Minions/Processors/SWMinionTargetingProcessor.h"

#include "Mass/Minions/SWMinionLaneWaveSubsystem.h"
#include "Mass/Minions/SWMinionMassTypes.h"
#include "Combat/Targeting/SWCombatTargetRegistrySubsystem.h"
#include "Engine/World.h"
#include "MassActorSubsystem.h"
#include "MassCommonFragments.h"
#include "MassCommonTypes.h"
#include "MassEntityManager.h"
#include "MassExecutionContext.h"
#include "MassSignalSubsystem.h"
#include "MassStateTreeTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SWMinionTargetingProcessor)

USWMinionTargetingProcessor::USWMinionTargetingProcessor()
	: EntityQuery(*this)
{
	// 由 USWMinionLaneWaveSubsystem 按 World 生命周期显式动态注册。
	bAutoRegisterWithProcessingPhases = false;
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::Standalone | EProcessorExecutionFlags::Server);
	ProcessingPhase = EMassProcessingPhase::PrePhysics;
	// 目标候选来自 Actor Registry，必须先完成 Mass Transform 到 Actor 的同步，避免读取同帧旧位置。
	ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::UpdateWorldFromMass);
	// Registry、Actor 弱引用解引用和 Signal 写入都依赖 UObject，必须在 Game Thread。
	bRequiresGameThreadExecution = true;
}

void USWMinionTargetingProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassActorFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FSWMinionTeamFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FSWMinionLaneFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FSWMinionIntentFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FSWMinionCombatFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FSWMinionTargetFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FSWMinionLeashFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FSWMinionTimingFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddConstSharedRequirement<FSWMinionArchetypeSharedFragment>();
	EntityQuery.AddTagRequirement<FSWMinionReadyTag>(EMassFragmentPresence::All);
	EntityQuery.AddTagRequirement<FSWMinionDeadTag>(EMassFragmentPresence::None);
}

void USWMinionTargetingProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UWorld* const World = EntityManager.GetWorld();
	USWCombatTargetRegistrySubsystem* const TargetRegistry = World ? World->GetSubsystem<USWCombatTargetRegistrySubsystem>() : nullptr;
	USWMinionLaneWaveSubsystem* const LaneWaveSubsystem = World ? World->GetSubsystem<USWMinionLaneWaveSubsystem>() : nullptr;
	UMassSignalSubsystem* const SignalSubsystem = World ? World->GetSubsystem<UMassSignalSubsystem>() : nullptr;
	if (!World || !TargetRegistry || !LaneWaveSubsystem || !SignalSubsystem)
	{
		return;
	}

	const float ServerTime = World->GetTimeSeconds();
	EntityQuery.ForEachEntityChunk(Context, [TargetRegistry, LaneWaveSubsystem, SignalSubsystem, ServerTime](FMassExecutionContext& ChunkContext)
	{
		const FSWMinionArchetypeSharedFragment& Archetype = ChunkContext.GetConstSharedFragment<FSWMinionArchetypeSharedFragment>();
		const TConstArrayView<FTransformFragment> Transforms = ChunkContext.GetFragmentView<FTransformFragment>();
		const TConstArrayView<FMassActorFragment> Actors = ChunkContext.GetFragmentView<FMassActorFragment>();
		const TConstArrayView<FSWMinionTeamFragment> Teams = ChunkContext.GetFragmentView<FSWMinionTeamFragment>();
		const TConstArrayView<FSWMinionLaneFragment> Lanes = ChunkContext.GetFragmentView<FSWMinionLaneFragment>();
		const TConstArrayView<FSWMinionIntentFragment> Intents = ChunkContext.GetFragmentView<FSWMinionIntentFragment>();
		const TConstArrayView<FSWMinionCombatFragment> Combats = ChunkContext.GetFragmentView<FSWMinionCombatFragment>();
		TArrayView<FSWMinionTargetFragment> Targets = ChunkContext.GetMutableFragmentView<FSWMinionTargetFragment>();
		TArrayView<FSWMinionLeashFragment> Leashes = ChunkContext.GetMutableFragmentView<FSWMinionLeashFragment>();
		TArrayView<FSWMinionTimingFragment> Timings = ChunkContext.GetMutableFragmentView<FSWMinionTimingFragment>();

		const float Interval = FMath::Max(0.05f, Archetype.TargetScanIntervalSeconds);
		// 可实际完成的追击距离不能超过“攻击停靠距离 + Leash”。TargetingRange
		// 是感知上限；若在更远处直接锁敌，静止目标也必然在进入攻击距离前触发
		// 回线，两个相向小兵会因此形成前进/返回振荡。
		for (FMassExecutionContext::FEntityIterator EntityIt = ChunkContext.CreateEntityIterator(); EntityIt; ++EntityIt)
		{
			const ESWMinionBehaviorIntent Behavior = Intents[EntityIt].Behavior;
			if (Behavior != ESWMinionBehaviorIntent::Advancing && Behavior != ESWMinionBehaviorIntent::Engaging
				&& Behavior != ESWMinionBehaviorIntent::Attacking && Behavior != ESWMinionBehaviorIntent::Returning)
			{
				continue;
			}

			FSWMinionTimingFragment& Timing = Timings[EntityIt];
			if (Timing.NextSenseServerTime <= 0.f)
			{
				const uint32 PhaseHash = GetTypeHash(ChunkContext.GetEntity(EntityIt));
				Timing.NextSenseServerTime = ServerTime + (static_cast<float>(PhaseHash % 1000u) / 1000.f) * Interval;
				continue;
			}
			if (ServerTime < Timing.NextSenseServerTime)
			{
				continue;
			}
			Timing.NextSenseServerTime = ServerTime + Interval;

			const AActor* const SourceActor = Actors[EntityIt].Get(FMassActorFragment::EActorAccess::OnlyWhenAlive);
			if (!SourceActor || !SourceActor->HasAuthority())
			{
				continue;
			}

			FSWMinionTargetFragment& Target = Targets[EntityIt];
			FSWMinionLeashFragment& Leash = Leashes[EntityIt];

			if (Behavior == ESWMinionBehaviorIntent::Returning)
			{
				continue;
			}

			if ((Behavior == ESWMinionBehaviorIntent::Engaging || Behavior == ESWMinionBehaviorIntent::Attacking) && Target.TargetActor.IsValid())
			{
				FTransform AnchorTransform;
				if (LaneWaveSubsystem->TrySampleLaneTransform(Lanes[EntityIt].LaneId, Leashes[EntityIt].AnchorLaneDistance, Lanes[EntityIt].Direction, AnchorTransform)
					&& FVector::DistSquared2D(Transforms[EntityIt].GetTransform().GetLocation(), AnchorTransform.GetLocation()) > FMath::Square(Archetype.LeashDistance))
				{
					Target.TargetActor.Reset();
					Target.TargetId = 0;
					Target.LastValidServerTime = 0.f;
					Target.bIsWithinAttackRange = false;
					SignalSubsystem->SignalEntityDeferred(ChunkContext, UE::Mass::Signals::NewStateTreeTaskRequired, ChunkContext.GetEntity(EntityIt));
					continue;
				}
			}

			FSWMinionTargetQuery Query;
			Query.SourceActor = SourceActor;
			Query.SourceTeam = Teams[EntityIt].TeamId;
			Query.SourceLocation = Transforms[EntityIt].GetTransform().GetLocation();
			Query.CurrentTarget = Target.TargetActor;
			Query.CurrentTargetId = Target.TargetId;
			Query.AcquisitionRange = FMath::Min(Archetype.TargetingRange,
				FMath::Max(0.f, Archetype.LeashDistance + Combats[EntityIt].AttackRange));
			const FSWMinionTargetResult Result = TargetRegistry->FindBestTarget(Query);

			const bool bWasWithinAttackRange = Target.bIsWithinAttackRange;
			const bool bAcquiredNewTarget = Target.TargetId == 0 && Result.HasTarget();
			Target.bIsWithinAttackRange = Result.HasTarget()
				&& FVector::DistSquared2D(Query.SourceLocation, Result.TargetActor->GetActorLocation()) <= FMath::Square(Combats[EntityIt].AttackRange);
			const bool bChanged = Target.TargetId != Result.TargetId || Target.TargetActor != Result.TargetActor
				|| bWasWithinAttackRange != Target.bIsWithinAttackRange;
			if (!bChanged)
			{
				if (Result.HasTarget())
				{
					Target.LastValidServerTime = ServerTime;
				}
				continue;
			}

			Target.TargetActor = Result.TargetActor;
			Target.TargetId = Result.TargetId;
			Target.LastValidServerTime = Result.HasTarget() ? ServerTime : 0.f;
			if (bAcquiredNewTarget)
			{
				// Leash Anchor 只在首次锁定目标时记录。进入/离开攻击距离会改变范围快照，
				// 但不能重写回线位置，否则回线点会随战斗抖动。
				Leash.AnchorLaneDistance = Lanes[EntityIt].DistanceAlongLane;
			}

			// 默认 Mass StateTree Processor 订阅该引擎 Signal；它被唤醒后再由只读 Condition 选择 Transition。
			SignalSubsystem->SignalEntityDeferred(ChunkContext, UE::Mass::Signals::NewStateTreeTaskRequired, ChunkContext.GetEntity(EntityIt));
		}
	});
}
