// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "Mass/Minions/SWMinionEntityFactory.h"

#include "Character/SWCharacter_Minion.h"
#include "Lane/SWLaneRoute.h"
#include "Mass/Minions/SWMinionDefinition.h"
#include "Mass/Minions/SWMinionSpawnInitializerProcessor.h"
#include "MassActorSubsystem.h"
#include "MassEntityConfigAsset.h"
#include "MassEntityManager.h"
#include "MassSpawnerSubsystem.h"
#include "StructUtils/StructView.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SWMinionEntityFactory)

DEFINE_LOG_CATEGORY_STATIC(LogSWMinionEntityFactory, Log, All);

bool USWMinionEntityFactory::SpawnBatchAuthority(const FSWMinionSpawnBatchRequest& Request, FSWMinionSpawnBatchResult& OutResult)
{
	OutResult = FSWMinionSpawnBatchResult();

	if (!IsAuthorityWorld())
	{
		OutResult.FailureReason = TEXT("Minion Entity Factory can only run in a server or standalone World.");
		return false;
	}

	if (!ValidateRequest(Request, OutResult.FailureReason))
	{
		return false;
	}

	UMassSpawnerSubsystem* const SpawnerSubsystem = GetWorld()->GetSubsystem<UMassSpawnerSubsystem>();
	if (!SpawnerSubsystem)
	{
		OutResult.FailureReason = TEXT("UMassSpawnerSubsystem is unavailable.");
		return false;
	}

	UMassEntityConfigAsset* const EntityConfig = Request.MinionDefinition->EntityConfig;
	const FMassEntityTemplate& EntityTemplate = EntityConfig->GetOrCreateEntityTemplate(*GetWorld());
	if (!EntityTemplate.IsValid())
	{
		OutResult.FailureReason = FString::Printf(TEXT("EntityConfig '%s' produced an invalid Mass template."), *GetNameSafe(EntityConfig));
		return false;
	}

	FSWLaneRouteSnapshot RouteSnapshot;
	check(Request.LaneRoute->TryGetRouteSnapshot(RouteSnapshot));

	FSWMinionSpawnData SpawnData;
	SpawnData.Entries.Reserve(Request.SpawnTransforms.Num());
	const ESWLaneDirection Direction = ASWLaneRoute::GetDirectionForTeam(Request.TeamId);
	const float InitialDistance = Request.TeamId == ESWTeamId::TeamA ? 0.f : RouteSnapshot.Length;
	for (int32 Index = 0; Index < Request.SpawnTransforms.Num(); ++Index)
	{
		FSWMinionSpawnEntry& Entry = SpawnData.Entries.AddDefaulted_GetRef();
		Entry.SpawnTransform = Request.SpawnTransforms[Index];
		Entry.UnitId = Request.MinionDefinition->UnitId;
		Entry.TeamId = Request.TeamId;
		Entry.LaneId = RouteSnapshot.LaneId;
		Entry.LaneDirection = Direction;
		Entry.DistanceAlongLane = InitialDistance;
		Entry.WaveIndex = Request.WaveIndex;
		Entry.SpawnOrdinal = Request.FirstSpawnOrdinal + Index;
	}

	TArray<FMassEntityHandle> SpawnedEntities;
	TSharedPtr<FMassEntityManager::FEntityCreationContext> CreationContext = SpawnerSubsystem->SpawnEntities(
		EntityTemplate.GetTemplateID(),
		SpawnData.Entries.Num(),
		FConstStructView::Make(SpawnData),
		USWMinionSpawnInitializerProcessor::StaticClass(),
		SpawnedEntities);
	if (!CreationContext.IsValid() || SpawnedEntities.Num() != SpawnData.Entries.Num())
	{
		if (!SpawnedEntities.IsEmpty())
		{
			SpawnerSubsystem->DestroyEntities(SpawnedEntities);
		}

		OutResult.FailureReason = FString::Printf(TEXT("Requested %d minion entities but created %d."), SpawnData.Entries.Num(), SpawnedEntities.Num());
		return false;
	}

	// 实体初始化流水线完成后，才允许创建依赖其 Fragment 的 Actor Bridge。
	CreationContext.Reset();

	if (!CreateActorBridgeAuthority(Request, SpawnedEntities, OutResult.FailureReason))
	{
		DestroyBatchAuthority(SpawnedEntities);
		return false;
	}

	OutResult.SpawnedEntities = MoveTemp(SpawnedEntities);
	UE_LOG(LogSWMinionEntityFactory, Display, TEXT("Created %d minion Entity/Actor bridges: Unit=%s Team=%d Lane=%d Wave=%d."),
		OutResult.SpawnedEntities.Num(), *Request.MinionDefinition->UnitId.ToString(), static_cast<uint8>(Request.TeamId),
		static_cast<uint8>(RouteSnapshot.LaneId), Request.WaveIndex);
	return true;
}

void USWMinionEntityFactory::DestroyBatchAuthority(const TConstArrayView<FMassEntityHandle> EntityHandles)
{
	if (!IsAuthorityWorld() || EntityHandles.IsEmpty())
	{
		return;
	}

	UMassSpawnerSubsystem* const SpawnerSubsystem = GetWorld()->GetSubsystem<UMassSpawnerSubsystem>();
	if (!SpawnerSubsystem)
	{
		ensureMsgf(false, TEXT("销毁小兵批次时缺少 UMassSpawnerSubsystem。"));
		return;
	}

	FMassEntityManager& EntityManager = SpawnerSubsystem->GetEntityManagerChecked();
	UMassActorSubsystem* const ActorSubsystem = GetWorld()->GetSubsystem<UMassActorSubsystem>();
	for (const FMassEntityHandle EntityHandle : EntityHandles)
	{
		if (FMassActorFragment* const ActorFragment = EntityManager.GetFragmentDataPtr<FMassActorFragment>(EntityHandle))
		{
			AActor* const MinionActor = ActorFragment->GetMutable(FMassActorFragment::EActorAccess::IncludePendingKill);
			ActorFragment->ResetAndUpdateHandleMap(ActorSubsystem);
			if (IsValid(MinionActor) && !MinionActor->IsActorBeingDestroyed())
			{
				MinionActor->Destroy();
			}
		}
	}

	SpawnerSubsystem->DestroyEntities(EntityHandles);
}

bool USWMinionEntityFactory::IsAuthorityWorld() const
{
	const UWorld* const World = GetWorld();
	return World && (World->GetNetMode() == NM_Standalone || World->GetNetMode() == NM_ListenServer || World->GetNetMode() == NM_DedicatedServer);
}

bool USWMinionEntityFactory::ValidateRequest(const FSWMinionSpawnBatchRequest& Request, FString& OutFailure) const
{
	OutFailure.Reset();

	if (!Request.LaneRoute || !Request.MinionDefinition)
	{
		OutFailure = TEXT("LaneRoute and MinionDefinition are required.");
		return false;
	}

	if (Request.TeamId != ESWTeamId::TeamA && Request.TeamId != ESWTeamId::TeamB)
	{
		OutFailure = TEXT("Minion batches require TeamA or TeamB.");
		return false;
	}

	if (Request.WaveIndex < 0 || Request.FirstSpawnOrdinal < 0 || Request.SpawnTransforms.IsEmpty())
	{
		OutFailure = TEXT("WaveIndex, FirstSpawnOrdinal, and at least one spawn transform are required.");
		return false;
	}

	if (Request.MinionDefinition->UnitId.IsNone()
		|| !Request.MinionDefinition->EntityConfig
		|| !Request.MinionDefinition->MinionActorClass
		|| !Request.MinionDefinition->CombatantDefinition)
	{
		OutFailure = TEXT("MinionDefinition requires UnitId, EntityConfig, MinionActorClass, and CombatantDefinition.");
		return false;
	}

	FSWLaneRouteSnapshot RouteSnapshot;
	if (!Request.LaneRoute->TryGetRouteSnapshot(RouteSnapshot))
	{
		OutFailure = FString::Printf(TEXT("LaneRoute '%s' has no valid runtime snapshot."), *GetNameSafe(Request.LaneRoute));
		return false;
	}

	if (RouteSnapshot.LaneId == ESWLaneId::None || ASWLaneRoute::GetDirectionForTeam(Request.TeamId) == ESWLaneDirection::None)
	{
		OutFailure = TEXT("LaneRoute or TeamId did not produce a valid lane direction.");
		return false;
	}

	return true;
}

bool USWMinionEntityFactory::CreateActorBridgeAuthority(
	const FSWMinionSpawnBatchRequest& Request,
	const TConstArrayView<FMassEntityHandle> EntityHandles,
	FString& OutFailure)
{
	check(Request.MinionDefinition);
	check(EntityHandles.Num() == Request.SpawnTransforms.Num());

	UMassSpawnerSubsystem* const SpawnerSubsystem = GetWorld()->GetSubsystem<UMassSpawnerSubsystem>();
	if (!SpawnerSubsystem)
	{
		OutFailure = TEXT("UMassSpawnerSubsystem is unavailable while creating the Actor bridge.");
		return false;
	}

	FMassEntityManager& EntityManager = SpawnerSubsystem->GetEntityManagerChecked();
	for (int32 Index = 0; Index < EntityHandles.Num(); ++Index)
	{
		const FMassEntityHandle EntityHandle = EntityHandles[Index];
		FMassActorFragment* const ActorFragment = EntityManager.GetFragmentDataPtr<FMassActorFragment>(EntityHandle);
		if (!ActorFragment || ActorFragment->IsValid())
		{
			OutFailure = FString::Printf(TEXT("Entity %d is missing an available FMassActorFragment."), Index);
			return false;
		}

		ASWCharacter_Minion* const MinionActor = GetWorld()->SpawnActorDeferred<ASWCharacter_Minion>(
			Request.MinionDefinition->MinionActorClass,
			Request.SpawnTransforms[Index],
			nullptr,
			nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!MinionActor)
		{
			OutFailure = FString::Printf(TEXT("Failed to Deferred Spawn minion Actor %d."), Index);
			return false;
		}

		FSWMinionActorInitializationData InitializationData;
		InitializationData.UnitId = Request.MinionDefinition->UnitId;
		InitializationData.WaveIndex = Request.WaveIndex;
		InitializationData.TeamId = Request.TeamId;
		InitializationData.CombatLevel = Request.MinionDefinition->CombatLevel;
		InitializationData.CombatantDefinition = Request.MinionDefinition->CombatantDefinition;
		if (!MinionActor->InitializeMinionAuthority(InitializationData))
		{
			MinionActor->Destroy();
			OutFailure = FString::Printf(TEXT("Minion Actor %d rejected its initialization data."), Index);
			return false;
		}

		MinionActor->FinishSpawning(Request.SpawnTransforms[Index]);
		if (!MinionActor->GetAbilitySystemComponent() || !MinionActor->SetMassEntityHandleAuthority(EntityHandle))
		{
			MinionActor->Destroy();
			OutFailure = FString::Printf(TEXT("Minion Actor %d did not initialize its ASC or Mass handle."), Index);
			return false;
		}

		// Mass 拥有此 Actor 的生命周期；该调用同时登记 Actor -> Entity 的反向索引。
		ActorFragment->SetAndUpdateHandleMap(EntityHandle, MinionActor, true);
		EntityManager.AddTagToEntity(EntityHandle, FSWMinionReadyTag::StaticStruct());
		MinionActor->ForceNetUpdate();
	}

	return true;
}
