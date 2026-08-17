#include "Mass/SmokeTest/SWMassSmokeTestSubsystem.h"

#include "HAL/IConsoleManager.h"
#include "Mass/SmokeTest/SWMassSmokeTestFragments.h"
#include "Mass/SmokeTest/SWMassSmokeTestSettings.h"
#include "MassEntityConfigAsset.h"
#include "MassEntityManager.h"
#include "MassEntitySubsystem.h"
#include "MassSpawnerSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SWMassSmokeTestSubsystem)

DEFINE_LOG_CATEGORY_STATIC(LogSWMassSmokeTest, Log, All);

namespace SWMassSmokeTest
{
	void Spawn(UWorld* World)
	{
		if (USWMassSmokeTestSubsystem* const Subsystem = World ? World->GetSubsystem<USWMassSmokeTestSubsystem>() : nullptr)
		{
			Subsystem->SpawnSmokeTestEntitiesAuthority();
		}
	}

	void Count(UWorld* World)
	{
		if (const USWMassSmokeTestSubsystem* const Subsystem = World ? World->GetSubsystem<USWMassSmokeTestSubsystem>() : nullptr)
		{
			UE_LOG(LogSWMassSmokeTest, Display, TEXT("Active smoke test entities: %d"), Subsystem->GetActiveSmokeTestEntityCount());
		}
	}

	void Clear(UWorld* World)
	{
		if (USWMassSmokeTestSubsystem* const Subsystem = World ? World->GetSubsystem<USWMassSmokeTestSubsystem>() : nullptr)
		{
			Subsystem->DestroySmokeTestEntitiesAuthority();
		}
	}

	FAutoConsoleCommandWithWorld SpawnCommand(
		TEXT("sw.MassSmoke.Spawn"),
		TEXT("Server/Standalone: creates 20 M10 Mass smoke-test entities."),
		FConsoleCommandWithWorldDelegate::CreateStatic(&Spawn),
		ECVF_Cheat);

	FAutoConsoleCommandWithWorld CountCommand(
		TEXT("sw.MassSmoke.Count"),
		TEXT("Prints the number of valid M10 Mass smoke-test entities in this World."),
		FConsoleCommandWithWorldDelegate::CreateStatic(&Count),
		ECVF_Cheat);

	FAutoConsoleCommandWithWorld ClearCommand(
		TEXT("sw.MassSmoke.Clear"),
		TEXT("Server/Standalone: destroys all M10 Mass smoke-test entities in this World."),
		FConsoleCommandWithWorldDelegate::CreateStatic(&Clear),
		ECVF_Cheat);
}

bool USWMassSmokeTestSubsystem::SpawnSmokeTestEntitiesAuthority(const int32 Count)
{
	if (!IsAuthorityWorld())
	{
		UE_LOG(LogSWMassSmokeTest, Warning, TEXT("Ignored spawn request in non-authority World '%s'."), *GetWorld()->GetName());
		return false;
	}

	if (Count <= 0)
	{
		UE_LOG(LogSWMassSmokeTest, Warning, TEXT("Smoke-test entity count must be positive; received %d."), Count);
		return false;
	}

	if (GetActiveSmokeTestEntityCount() > 0)
	{
		UE_LOG(LogSWMassSmokeTest, Warning, TEXT("Smoke-test entities already exist. Clear them before creating another batch."));
		return false;
	}

	const USWMassSmokeTestSettings* const Settings = GetDefault<USWMassSmokeTestSettings>();
	UMassEntityConfigAsset* const EntityConfig = Settings ? Settings->SmokeTestEntityConfig.LoadSynchronous() : nullptr;
	if (!EntityConfig)
	{
		UE_LOG(LogSWMassSmokeTest, Error, TEXT("Missing SmokeTestEntityConfig. Set it in Project Settings > Game > SW Mass Smoke Test."));
		return false;
	}

	UMassSpawnerSubsystem* const SpawnerSubsystem = GetWorld()->GetSubsystem<UMassSpawnerSubsystem>();
	if (!SpawnerSubsystem)
	{
		UE_LOG(LogSWMassSmokeTest, Error, TEXT("UMassSpawnerSubsystem is unavailable in World '%s'."), *GetWorld()->GetName());
		return false;
	}

	const FMassEntityTemplate& EntityTemplate = EntityConfig->GetOrCreateEntityTemplate(*GetWorld());
	if (!EntityTemplate.IsValid())
	{
		UE_LOG(LogSWMassSmokeTest, Error, TEXT("EntityConfig '%s' produced an invalid Mass entity template."), *GetNameSafe(EntityConfig));
		return false;
	}

	TArray<FMassEntityHandle> SpawnedEntities;
	TSharedPtr<FMassEntityManager::FEntityCreationContext> CreationContext = SpawnerSubsystem->SpawnEntities(EntityTemplate, Count, SpawnedEntities);
	if (!CreationContext.IsValid() || SpawnedEntities.Num() != Count)
	{
		if (!SpawnedEntities.IsEmpty())
		{
			SpawnerSubsystem->DestroyEntities(SpawnedEntities);
		}

		UE_LOG(LogSWMassSmokeTest, Error, TEXT("Mass smoke-test batch failed: requested %d, created %d."), Count, SpawnedEntities.Num());
		return false;
	}

	FMassEntityManager& EntityManager = SpawnerSubsystem->GetEntityManagerChecked();
	for (int32 Index = 0; Index < SpawnedEntities.Num(); ++Index)
	{
		FSWMassSmokeTestFragment* const Fragment = EntityManager.GetFragmentDataPtr<FSWMassSmokeTestFragment>(SpawnedEntities[Index]);
		if (!Fragment)
		{
			SpawnerSubsystem->DestroyEntities(SpawnedEntities);
			UE_LOG(LogSWMassSmokeTest, Error, TEXT("Mass smoke-test EntityConfig '%s' is missing FSWMassSmokeTestFragment; batch was rolled back."), *GetNameSafe(EntityConfig));
			return false;
		}

		Fragment->SpawnOrdinal = Index;
	}

	// CreationContext 保持到此函数结束，确保由 MassSpawner 延迟的创建通知在返回前完整提交。
	SmokeTestEntities = MoveTemp(SpawnedEntities);
	UE_LOG(LogSWMassSmokeTest, Display, TEXT("Created %d M10 Mass smoke-test entities in authority World '%s'."), SmokeTestEntities.Num(), *GetWorld()->GetName());
	return true;
}

int32 USWMassSmokeTestSubsystem::GetActiveSmokeTestEntityCount() const
{
	const UWorld* const World = GetWorld();
	const UMassEntitySubsystem* const EntitySubsystem = World ? World->GetSubsystem<UMassEntitySubsystem>() : nullptr;
	if (!EntitySubsystem)
	{
		return 0;
	}

	const FMassEntityManager& EntityManager = EntitySubsystem->GetEntityManager();
	int32 ActiveEntityCount = 0;
	for (const FMassEntityHandle Handle : SmokeTestEntities)
	{
		ActiveEntityCount += EntityManager.IsEntityValid(Handle) ? 1 : 0;
	}

	return ActiveEntityCount;
}

void USWMassSmokeTestSubsystem::DestroySmokeTestEntitiesAuthority()
{
	if (!IsAuthorityWorld())
	{
		UE_LOG(LogSWMassSmokeTest, Warning, TEXT("Ignored clear request in non-authority World '%s'."), *GetWorld()->GetName());
		return;
	}

	if (SmokeTestEntities.IsEmpty())
	{
		return;
	}

	if (UMassSpawnerSubsystem* const SpawnerSubsystem = GetWorld()->GetSubsystem<UMassSpawnerSubsystem>())
	{
		TArray<FMassEntityHandle> ValidEntities;
		const FMassEntityManager& EntityManager = SpawnerSubsystem->GetEntityManagerChecked();
		for (const FMassEntityHandle Handle : SmokeTestEntities)
		{
			if (EntityManager.IsEntityValid(Handle))
			{
				ValidEntities.Add(Handle);
			}
		}

		if (!ValidEntities.IsEmpty())
		{
			SpawnerSubsystem->DestroyEntities(ValidEntities);
		}
	}

	SmokeTestEntities.Reset();
	UE_LOG(LogSWMassSmokeTest, Display, TEXT("Cleared M10 Mass smoke-test entities in World '%s'."), *GetWorld()->GetName());
}

void USWMassSmokeTestSubsystem::Deinitialize()
{
	DestroySmokeTestEntitiesAuthority();
	Super::Deinitialize();
}

bool USWMassSmokeTestSubsystem::IsAuthorityWorld() const
{
	const UWorld* const World = GetWorld();
	return World && (World->GetNetMode() == NM_Standalone || World->GetNetMode() == NM_ListenServer || World->GetNetMode() == NM_DedicatedServer);
}
