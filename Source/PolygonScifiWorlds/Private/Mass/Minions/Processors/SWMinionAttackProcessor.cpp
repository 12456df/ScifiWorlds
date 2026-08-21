// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "Mass/Minions/Processors/SWMinionAttackProcessor.h"

#include "Character/SWCharacter_Minion.h"
#include "Mass/Minions/SWMinionMassTypes.h"
#include "Engine/World.h"
#include "MassActorSubsystem.h"
#include "MassCommonFragments.h"
#include "MassCommonTypes.h"
#include "MassEntityManager.h"
#include "MassExecutionContext.h"
#include "MassSignalSubsystem.h"
#include "MassStateTreeTypes.h"
#include "Math/RotationMatrix.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SWMinionAttackProcessor)

USWMinionAttackProcessor::USWMinionAttackProcessor()
	: EntityQuery(*this)
{
	// 由 USWMinionLaneWaveSubsystem 按 World 生命周期显式动态注册。
	bAutoRegisterWithProcessingPhases = false;
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::Standalone | EProcessorExecutionFlags::Server);
	ProcessingPhase = EMassProcessingPhase::PrePhysics;
	ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::UpdateWorldFromMass);
	bRequiresGameThreadExecution = true;
}

void USWMinionAttackProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FMassActorFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FSWMinionIntentFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FSWMinionTargetFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddTagRequirement<FSWMinionReadyTag>(EMassFragmentPresence::All);
	EntityQuery.AddTagRequirement<FSWMinionDeadTag>(EMassFragmentPresence::None);
}

void USWMinionAttackProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UMassSignalSubsystem* const SignalSubsystem = EntityManager.GetWorld() ? EntityManager.GetWorld()->GetSubsystem<UMassSignalSubsystem>() : nullptr;
	EntityQuery.ForEachEntityChunk(Context, [SignalSubsystem](FMassExecutionContext& ChunkContext)
	{
		TArrayView<FMassActorFragment> Actors = ChunkContext.GetMutableFragmentView<FMassActorFragment>();
		TArrayView<FTransformFragment> Transforms = ChunkContext.GetMutableFragmentView<FTransformFragment>();
		TArrayView<FSWMinionIntentFragment> Intents = ChunkContext.GetMutableFragmentView<FSWMinionIntentFragment>();
		TArrayView<FSWMinionTargetFragment> Targets = ChunkContext.GetMutableFragmentView<FSWMinionTargetFragment>();

		for (FMassExecutionContext::FEntityIterator EntityIt = ChunkContext.CreateEntityIterator(); EntityIt; ++EntityIt)
		{
			FSWMinionIntentFragment& Intent = Intents[EntityIt];
			if (Intent.Behavior != ESWMinionBehaviorIntent::Attacking || !Intent.bAttackRequested)
			{
				continue;
			}

			// 必须先消费意图，防止 Actor/Ability 失败时同一个 StateTree 状态每帧反复发起激活。
			Intent.bAttackRequested = false;
			AActor* const Actor = Actors[EntityIt].GetMutable(FMassActorFragment::EActorAccess::OnlyWhenAlive);
			ASWCharacter_Minion* const Minion = Cast<ASWCharacter_Minion>(Actor);
			FSWMinionTargetFragment& TargetFragment = Targets[EntityIt];
			AActor* const Target = TargetFragment.TargetActor.Get();
			if (Minion && Target)
			{
				const FVector DirectionToTarget = (Target->GetActorLocation() - Transforms[EntityIt].GetTransform().GetLocation()).GetSafeNormal2D();
				if (!DirectionToTarget.IsNearlyZero())
				{
					// 与 Character 的即时转向成对写入：Actor 负责本帧攻击表现，Mass
					// Transform 仍是下一帧同步和网络位置的唯一来源。
					Transforms[EntityIt].GetMutableTransform().SetRotation(FRotationMatrix::MakeFromXZ(DirectionToTarget, FVector::UpVector).ToQuat());
				}
				const ESWMinionAttackAttemptResult AttemptResult = Minion->TryActivateMinionAttackAuthority(Target);
				if (AttemptResult == ESWMinionAttackAttemptResult::TargetDead
					|| AttemptResult == ESWMinionAttackAttemptResult::InvalidTarget
					|| AttemptResult == ESWMinionAttackAttemptResult::SameTeam)
				{
					// 攻击 Ability 结束后才触发下一次请求时，目标可能已经死亡。立即清理
					// 失效快照并唤醒 StateTree，避免最多等待一次低频索敌才离开 Attacking。
					TargetFragment.TargetActor.Reset();
					TargetFragment.TargetId = 0;
					TargetFragment.LastValidServerTime = 0.f;
					TargetFragment.bIsWithinAttackRange = false;
					if (SignalSubsystem)
					{
						SignalSubsystem->SignalEntityDeferred(ChunkContext, UE::Mass::Signals::NewStateTreeTaskRequired, ChunkContext.GetEntity(EntityIt));
					}
				}
				else if (AttemptResult == ESWMinionAttackAttemptResult::OutOfRange)
				{
					// Target 仍可能有效，只是被局部分离推离了攻击范围；回到 Engaging 而不是放弃目标。
					TargetFragment.bIsWithinAttackRange = false;
					if (SignalSubsystem)
					{
						SignalSubsystem->SignalEntityDeferred(ChunkContext, UE::Mass::Signals::NewStateTreeTaskRequired, ChunkContext.GetEntity(EntityIt));
					}
				}
			}
		}
	});
}
