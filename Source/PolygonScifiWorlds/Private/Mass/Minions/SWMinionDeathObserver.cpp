// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "Mass/Minions/SWMinionDeathObserver.h"

#include "AbilitySystem/Data/SWCombatantDefinition.h"
#include "Character/SWCharacter_Minion.h"
#include "Mass/Minions/SWMinionMassTypes.h"
#include "Engine/World.h"
#include "MassActorSubsystem.h"
#include "MassEntityManager.h"
#include "MassExecutionContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SWMinionDeathObserver)

USWMinionDeathObserver::USWMinionDeathObserver()
	: DeathQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::Standalone | EProcessorExecutionFlags::Server);
	ObservedType = FSWMinionDeadTag::StaticStruct();
	ObservedOperations = EMassObservedOperationFlags::Add;
	bRequiresGameThreadExecution = true;
}

void USWMinionDeathObserver::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	DeathQuery.AddRequirement<FMassActorFragment>(EMassFragmentAccess::ReadOnly);
	DeathQuery.AddRequirement<FSWMinionTimingFragment>(EMassFragmentAccess::ReadWrite);
}

void USWMinionDeathObserver::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UWorld* const World = EntityManager.GetWorld();
	if (!World)
	{
		return;
	}

	const float ServerTime = World->GetTimeSeconds();
	DeathQuery.ForEachEntityChunk(Context, [ServerTime](FMassExecutionContext& ExecutionContext)
	{
		const TConstArrayView<FMassActorFragment> ActorFragments = ExecutionContext.GetFragmentView<FMassActorFragment>();
		TArrayView<FSWMinionTimingFragment> Timings = ExecutionContext.GetMutableFragmentView<FSWMinionTimingFragment>();

		for (FMassExecutionContext::FEntityIterator EntityIt = ExecutionContext.CreateEntityIterator(); EntityIt; ++EntityIt)
		{
			const ASWCharacter_Minion* const MinionActor = Cast<ASWCharacter_Minion>(ActorFragments[EntityIt].Get(FMassActorFragment::EActorAccess::IncludePendingKill));
			const USWCombatantDefinition* const CombatantDefinition = MinionActor ? MinionActor->GetCombatantDefinition() : nullptr;
			const float CorpseLifetime = CombatantDefinition ? FMath::Max(0.f, CombatantDefinition->CorpseLifetimeSeconds) : 0.f;
			Timings[EntityIt].CleanupServerTime = ServerTime + CorpseLifetime;
		}
	});
}
