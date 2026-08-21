// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "Mass/Minions/SWMinionActorReadyObserver.h"

#include "Character/SWCharacter_Minion.h"
#include "Mass/Minions/SWMinionMassTypes.h"
#include "MassActorSubsystem.h"
#include "MassExecutionContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SWMinionActorReadyObserver)

DEFINE_LOG_CATEGORY_STATIC(LogSWMinionActorReadyObserver, Log, All);

USWMinionActorReadyObserver::USWMinionActorReadyObserver()
	: ReadyQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::Standalone | EProcessorExecutionFlags::Server);
	ObservedType = FSWMinionReadyTag::StaticStruct();
	ObservedOperations = EMassObservedOperationFlags::Add;
}

void USWMinionActorReadyObserver::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	ReadyQuery.AddRequirement<FMassActorFragment>(EMassFragmentAccess::ReadOnly);
	ReadyQuery.AddRequirement<FSWMinionIdentityFragment>(EMassFragmentAccess::ReadOnly);
	ReadyQuery.AddRequirement<FSWMinionTeamFragment>(EMassFragmentAccess::ReadOnly);
}

void USWMinionActorReadyObserver::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	ReadyQuery.ForEachEntityChunk(Context, [](FMassExecutionContext& ExecutionContext)
	{
		const TConstArrayView<FMassActorFragment> ActorFragments = ExecutionContext.GetFragmentView<FMassActorFragment>();
		const TConstArrayView<FSWMinionIdentityFragment> IdentityFragments = ExecutionContext.GetFragmentView<FSWMinionIdentityFragment>();
		const TConstArrayView<FSWMinionTeamFragment> TeamFragments = ExecutionContext.GetFragmentView<FSWMinionTeamFragment>();

		for (FMassExecutionContext::FEntityIterator EntityIt = ExecutionContext.CreateEntityIterator(); EntityIt; ++EntityIt)
		{
			const ASWCharacter_Minion* const MinionActor = Cast<ASWCharacter_Minion>(ActorFragments[EntityIt].Get());
			const bool bIsBridgeValid = IsValid(MinionActor)
				&& MinionActor->GetAbilitySystemComponent()
				&& MinionActor->GetMinionUnitId() == IdentityFragments[EntityIt].UnitId
				&& MinionActor->GetTeamId() == TeamFragments[EntityIt].TeamId
				&& MinionActor->GetMassEntityHandleAuthority().IsValid();

			ensureMsgf(bIsBridgeValid,
				TEXT("Ready 小兵 Entity 存在不完整的 Actor/ASC Bridge：Unit=%s Team=%d。"),
				*IdentityFragments[EntityIt].UnitId.ToString(), static_cast<uint8>(TeamFragments[EntityIt].TeamId));
		}
	});
}
