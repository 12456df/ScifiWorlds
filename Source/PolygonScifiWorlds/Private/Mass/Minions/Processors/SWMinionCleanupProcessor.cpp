// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "Mass/Minions/Processors/SWMinionCleanupProcessor.h"

#include "Mass/Minions/SWMinionLaneWaveSubsystem.h"
#include "Mass/Minions/SWMinionMassTypes.h"
#include "Engine/World.h"
#include "MassActorSubsystem.h"
#include "MassEntityManager.h"
#include "MassExecutionContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SWMinionCleanupProcessor)

USWMinionCleanupProcessor::USWMinionCleanupProcessor()
	: EntityQuery(*this)
{
	// 由 USWMinionLaneWaveSubsystem 按 World 生命周期显式动态注册。
	bAutoRegisterWithProcessingPhases = false;
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::Standalone | EProcessorExecutionFlags::Server);
	ProcessingPhase = EMassProcessingPhase::FrameEnd;
	// 销毁 Actor 及维护 Actor↔Entity 映射需要 Game Thread。
	bRequiresGameThreadExecution = true;
}

void USWMinionCleanupProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FMassActorFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FSWMinionTimingFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddTagRequirement<FSWMinionDeadTag>(EMassFragmentPresence::All);
}

void USWMinionCleanupProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UWorld* const World = EntityManager.GetWorld();
	UMassActorSubsystem* const ActorSubsystem = World ? World->GetSubsystem<UMassActorSubsystem>() : nullptr;
	USWMinionLaneWaveSubsystem* const WaveSubsystem = World ? World->GetSubsystem<USWMinionLaneWaveSubsystem>() : nullptr;
	if (!World || !ActorSubsystem)
	{
		return;
	}

	const float ServerTime = World->GetTimeSeconds();
	EntityQuery.ForEachEntityChunk(Context, [ActorSubsystem, WaveSubsystem, ServerTime](FMassExecutionContext& ExecutionContext)
	{
		TArrayView<FMassActorFragment> ActorFragments = ExecutionContext.GetMutableFragmentView<FMassActorFragment>();
		const TConstArrayView<FSWMinionTimingFragment> Timings = ExecutionContext.GetFragmentView<FSWMinionTimingFragment>();

		for (FMassExecutionContext::FEntityIterator EntityIt = ExecutionContext.CreateEntityIterator(); EntityIt; ++EntityIt)
		{
			if (Timings[EntityIt].CleanupServerTime <= 0.f || ServerTime < Timings[EntityIt].CleanupServerTime)
			{
				continue;
			}

			const FMassEntityHandle EntityHandle = ExecutionContext.GetEntity(EntityIt);
			FMassActorFragment& ActorFragment = ActorFragments[EntityIt];
			AActor* const MinionActor = ActorFragment.GetMutable(FMassActorFragment::EActorAccess::IncludePendingKill);
			ActorFragment.ResetAndUpdateHandleMap(ActorSubsystem);
			if (IsValid(MinionActor) && !MinionActor->IsActorBeingDestroyed())
			{
				MinionActor->Destroy();
			}

			if (WaveSubsystem)
			{
				WaveSubsystem->RemoveActiveMinionEntityAuthority(EntityHandle);
			}
			ExecutionContext.Defer().DestroyEntity(EntityHandle);
		}
	});
}
