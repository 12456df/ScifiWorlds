// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "Mass/Minions/SWMinionLaneWaveSubsystem.h"

#include "EngineUtils.h"
#include "GameMode/SWGameMode.h"
#include "Lane/SWLaneRoute.h"
#include "Mass/Minions/SWMinionDefinition.h"
#include "Mass/Minions/SWMinionEntityFactory.h"
#include "Mass/Minions/SWMinionWaveData.h"
#include "MassEntityManager.h"
#include "MassEntitySubsystem.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SWMinionLaneWaveSubsystem)

DEFINE_LOG_CATEGORY_STATIC(LogSWMinionLaneWave, Log, All);

namespace SWMinionLaneWave
{
	constexpr ESWLaneId RequiredLanes[] = { ESWLaneId::Top, ESWLaneId::Middle, ESWLaneId::Bottom };
	constexpr ESWTeamId RequiredTeams[] = { ESWTeamId::TeamA, ESWTeamId::TeamB };

	uint8 ToKey(const ESWLaneId LaneId)
	{
		return static_cast<uint8>(LaneId);
	}
}

bool USWMinionLaneWaveSubsystem::StartWavesAuthority(USWMinionWaveData* const InWaveData)
{
	if (!IsAuthorityWorld())
	{
		UE_LOG(LogSWMinionLaneWave, Warning, TEXT("Ignored wave-start request in non-authority World '%s'."), *GetNameSafe(GetWorld()));
		return false;
	}

	if (bWavesRunning)
	{
		UE_LOG(LogSWMinionLaneWave, Warning, TEXT("Wave subsystem is already running in World '%s'."), *GetNameSafe(GetWorld()));
		return false;
	}

	if (!IsMatchInProgress())
	{
		UE_LOG(LogSWMinionLaneWave, Warning, TEXT("Wave subsystem can only start while the match is InProgress."));
		return false;
	}

	FString ValidationFailure;
	if (!InWaveData || !ValidateWaveData(*InWaveData, ValidationFailure) || !CacheLaneRoutesAuthority())
	{
		UE_LOG(LogSWMinionLaneWave, Error, TEXT("Unable to start minion waves: %s"),
			ValidationFailure.IsEmpty() ? TEXT("the GameMap lane routes are invalid.") : *ValidationFailure);
		return false;
	}

	WaveData = InWaveData;
	EntityFactory = NewObject<USWMinionEntityFactory>(this);
	NextWaveIndex = 0;
	LastSpawnedWaveIndex = INDEX_NONE;
	bWavesRunning = true;

	GetWorld()->GetTimerManager().SetTimer(
		WaveTimer,
		this,
		&ThisClass::HandleWaveTimerElapsed,
		WaveData->WaveIntervalSeconds,
		true,
		WaveData->InitialWaveDelaySeconds);

	UE_LOG(LogSWMinionLaneWave, Display, TEXT("Started minion waves: initial delay %.1fs, interval %.1fs, hard cap %d."),
		WaveData->InitialWaveDelaySeconds, WaveData->WaveIntervalSeconds, WaveData->ActiveMinionHardCap);
	return true;
}

void USWMinionLaneWaveSubsystem::StopWavesAuthority()
{
	if (UWorld* const World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(WaveTimer);
	}

	bWavesRunning = false;
	WaveData = nullptr;
	EntityFactory = nullptr;
	LaneRoutes.Reset();
}

int32 USWMinionLaneWaveSubsystem::GetActiveMinionEntityCount() const
{
	const UWorld* const World = GetWorld();
	const UMassEntitySubsystem* const EntitySubsystem = World ? World->GetSubsystem<UMassEntitySubsystem>() : nullptr;
	if (!EntitySubsystem)
	{
		return 0;
	}

	const FMassEntityManager& EntityManager = EntitySubsystem->GetEntityManager();
	int32 ValidEntityCount = 0;
	for (const FMassEntityHandle Handle : ActiveMinionEntities)
	{
		ValidEntityCount += EntityManager.IsEntityValid(Handle) ? 1 : 0;
	}

	return ValidEntityCount;
}

void USWMinionLaneWaveSubsystem::Deinitialize()
{
	StopWavesAuthority();
	ActiveMinionEntities.Reset();
	Super::Deinitialize();
}

bool USWMinionLaneWaveSubsystem::IsAuthorityWorld() const
{
	const UWorld* const World = GetWorld();
	return World && (World->GetNetMode() == NM_Standalone || World->GetNetMode() == NM_ListenServer || World->GetNetMode() == NM_DedicatedServer);
}

bool USWMinionLaneWaveSubsystem::CacheLaneRoutesAuthority()
{
	LaneRoutes.Reset();
	UWorld* const World = GetWorld();
	if (!World)
	{
		return false;
	}

	for (TActorIterator<ASWLaneRoute> It(World); It; ++It)
	{
		ASWLaneRoute* const LaneRoute = *It;
		if (!LaneRoute)
		{
			continue;
		}

		FSWLaneRouteSnapshot RouteSnapshot;
		if (!LaneRoute->TryGetRouteSnapshot(RouteSnapshot))
		{
			UE_LOG(LogSWMinionLaneWave, Error, TEXT("Lane route '%s' has no valid server snapshot."), *LaneRoute->GetName());
			return false;
		}

		const uint8 LaneKey = SWMinionLaneWave::ToKey(RouteSnapshot.LaneId);
		if (LaneRoutes.Contains(LaneKey))
		{
			UE_LOG(LogSWMinionLaneWave, Error, TEXT("More than one lane route uses LaneId %d."), LaneKey);
			return false;
		}

		LaneRoutes.Add(LaneKey, LaneRoute);
	}

	for (const ESWLaneId RequiredLaneId : SWMinionLaneWave::RequiredLanes)
	{
		if (!LaneRoutes.Contains(SWMinionLaneWave::ToKey(RequiredLaneId)))
		{
			UE_LOG(LogSWMinionLaneWave, Error, TEXT("GameMap is missing a required lane route with LaneId %d."), static_cast<uint8>(RequiredLaneId));
			return false;
		}
	}

	return true;
}

bool USWMinionLaneWaveSubsystem::ValidateWaveData(const USWMinionWaveData& InWaveData, FString& OutFailure) const
{
	OutFailure.Reset();

	if (InWaveData.WaveIntervalSeconds <= 0.f || InWaveData.ActiveMinionHardCap <= 0 || InWaveData.Composition.IsEmpty())
	{
		OutFailure = TEXT("WaveData requires a positive interval, a positive hard cap, and at least one composition entry.");
		return false;
	}

	int32 PerLaneTeamCount = 0;
	for (const FSWMinionWaveCompositionEntry& Entry : InWaveData.Composition)
	{
		if (!Entry.MinionDefinition || Entry.Count <= 0 || Entry.FormationOffsets.Num() != Entry.Count)
		{
			OutFailure = TEXT("Every composition entry requires a MinionDefinition and exactly one FormationOffset per minion.");
			return false;
		}

		PerLaneTeamCount += Entry.Count;
	}

	const int32 TotalEntitiesPerWave = PerLaneTeamCount * UE_ARRAY_COUNT(SWMinionLaneWave::RequiredLanes) * UE_ARRAY_COUNT(SWMinionLaneWave::RequiredTeams);
	if (TotalEntitiesPerWave > InWaveData.ActiveMinionHardCap)
	{
		OutFailure = FString::Printf(TEXT("One complete wave requires %d entities, exceeding ActiveMinionHardCap %d."), TotalEntitiesPerWave, InWaveData.ActiveMinionHardCap);
		return false;
	}

	return true;
}

void USWMinionLaneWaveSubsystem::HandleWaveTimerElapsed()
{
	SpawnNextWaveAuthority();
}

bool USWMinionLaneWaveSubsystem::SpawnNextWaveAuthority()
{
	if (!bWavesRunning || !IsAuthorityWorld() || !IsMatchInProgress() || !WaveData || !EntityFactory)
	{
		return false;
	}

	PruneInvalidActiveEntities();

	int32 PerLaneTeamCount = 0;
	for (const FSWMinionWaveCompositionEntry& Entry : WaveData->Composition)
	{
		PerLaneTeamCount += Entry.Count;
	}

	const int32 TotalEntitiesPerWave = PerLaneTeamCount * UE_ARRAY_COUNT(SWMinionLaneWave::RequiredLanes) * UE_ARRAY_COUNT(SWMinionLaneWave::RequiredTeams);
	if (ActiveMinionEntities.Num() + TotalEntitiesPerWave > WaveData->ActiveMinionHardCap)
	{
		UE_LOG(LogSWMinionLaneWave, Warning, TEXT("Skipped wave %d: active %d + requested %d exceeds hard cap %d."),
			NextWaveIndex, ActiveMinionEntities.Num(), TotalEntitiesPerWave, WaveData->ActiveMinionHardCap);
		return false;
	}

	TArray<FMassEntityHandle> SpawnedThisWave;
	for (const ESWLaneId LaneId : SWMinionLaneWave::RequiredLanes)
	{
		ASWLaneRoute* const LaneRoute = LaneRoutes.FindRef(SWMinionLaneWave::ToKey(LaneId)).Get();
		FSWLaneRouteSnapshot RouteSnapshot;
		if (!LaneRoute || !LaneRoute->TryGetRouteSnapshot(RouteSnapshot))
		{
			UE_LOG(LogSWMinionLaneWave, Error, TEXT("Stopped minion waves because LaneId %d is no longer valid."), static_cast<uint8>(LaneId));
			StopWavesAuthority();
			return false;
		}

		for (const ESWTeamId TeamId : SWMinionLaneWave::RequiredTeams)
		{
			const FTransform& TeamSpawnTransform = TeamId == ESWTeamId::TeamA
				? RouteSnapshot.TeamASpawnTransform
				: RouteSnapshot.TeamBSpawnTransform;
			int32 NextSpawnOrdinal = 0;

			for (const FSWMinionWaveCompositionEntry& Entry : WaveData->Composition)
			{
				FSWMinionSpawnBatchRequest SpawnRequest;
				SpawnRequest.LaneRoute = LaneRoute;
				SpawnRequest.MinionDefinition = Entry.MinionDefinition;
				SpawnRequest.TeamId = TeamId;
				SpawnRequest.WaveIndex = NextWaveIndex;
				SpawnRequest.FirstSpawnOrdinal = NextSpawnOrdinal;
				SpawnRequest.SpawnTransforms.Reserve(Entry.Count);

				for (const FVector& LocalOffset : Entry.FormationOffsets)
				{
					FTransform SpawnTransform = TeamSpawnTransform;
					SpawnTransform.SetLocation(TeamSpawnTransform.TransformPositionNoScale(LocalOffset));
					SpawnRequest.SpawnTransforms.Add(SpawnTransform);
				}

				FSWMinionSpawnBatchResult SpawnResult;
				if (!EntityFactory->SpawnBatchAuthority(SpawnRequest, SpawnResult))
				{
					if (!SpawnedThisWave.IsEmpty())
					{
						EntityFactory->DestroyBatchAuthority(SpawnedThisWave);
					}

					UE_LOG(LogSWMinionLaneWave, Error, TEXT("Wave %d failed and was rolled back: %s"), NextWaveIndex, *SpawnResult.FailureReason);
					return false;
				}

				NextSpawnOrdinal += Entry.Count;
				SpawnedThisWave.Append(MoveTemp(SpawnResult.SpawnedEntities));
			}
		}
	}

	ActiveMinionEntities.Append(MoveTemp(SpawnedThisWave));
	LastSpawnedWaveIndex = NextWaveIndex++;
	UE_LOG(LogSWMinionLaneWave, Display, TEXT("Spawned wave %d with %d minion entities; active total is %d."),
		LastSpawnedWaveIndex, PerLaneTeamCount * UE_ARRAY_COUNT(SWMinionLaneWave::RequiredLanes) * UE_ARRAY_COUNT(SWMinionLaneWave::RequiredTeams), ActiveMinionEntities.Num());
	return true;
}

bool USWMinionLaneWaveSubsystem::IsMatchInProgress() const
{
	const ASWGameMode* const GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ASWGameMode>() : nullptr;
	return GameMode && GameMode->GetMatchState() == MatchState::InProgress;
}

void USWMinionLaneWaveSubsystem::PruneInvalidActiveEntities()
{
	UMassEntitySubsystem* const EntitySubsystem = GetWorld() ? GetWorld()->GetSubsystem<UMassEntitySubsystem>() : nullptr;
	if (!EntitySubsystem)
	{
		ActiveMinionEntities.Reset();
		return;
	}

	const FMassEntityManager& EntityManager = EntitySubsystem->GetEntityManager();
	ActiveMinionEntities.RemoveAllSwap([&EntityManager](const FMassEntityHandle Handle)
	{
		return !EntityManager.IsEntityValid(Handle);
	}, EAllowShrinking::No);
}
