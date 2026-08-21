// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "Mass/Minions/SWMinionLaneWaveSubsystem.h"

#include "Character/SWCharacter_Minion.h"
#include "EngineUtils.h"
#include "GameMode/SWGameMode.h"
#include "HAL/IConsoleManager.h"
#include "Lane/SWLaneRoute.h"
#include "Mass/Minions/SWMinionDefinition.h"
#include "Mass/Minions/SWMinionEntityFactory.h"
#include "Mass/Minions/Processors/SWMinionActorSyncProcessor.h"
#include "Mass/Minions/Processors/SWMinionSeparationProcessor.h"
#include "Mass/Minions/Processors/SWMinionAttackProcessor.h"
#include "Mass/Minions/Processors/SWMinionCleanupProcessor.h"
#include "Mass/Minions/Processors/SWMinionLaneMovementProcessor.h"
#include "Mass/Minions/Processors/SWMinionTargetingProcessor.h"
#include "Mass/Minions/SWMinionTargetRegistrySubsystem.h"
#include "Mass/Minions/SWMinionWaveData.h"
#include "MassActorSubsystem.h"
#include "MassEntityManager.h"
#include "MassEntitySubsystem.h"
#include "MassProcessor.h"
#include "MassSimulationSubsystem.h"
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

	void PrintDiagnostics(UWorld* World)
	{
		USWMinionLaneWaveSubsystem* const Subsystem = World ? World->GetSubsystem<USWMinionLaneWaveSubsystem>() : nullptr;
		if (!Subsystem)
		{
			UE_LOG(LogSWMinionLaneWave, Warning, TEXT("Minion diagnostics unavailable: World has no lane-wave subsystem."));
			return;
		}

		const FSWMinionRuntimeDiagnostics Diagnostics = Subsystem->GetRuntimeDiagnosticsAuthority();
		UE_LOG(LogSWMinionLaneWave, Display,
			TEXT("M11 Minion Diagnostics: Waves=%s LastWave=%d Tracked=%d ValidEntity=%d ActorBridge=%d DeadActor=%d WorldActors=%d Targets=%d Intent[None=%d Advance=%d Engage=%d Attack=%d Return=%d] Moving=%d InvalidLane=%d TransformMismatch=%d Spawned=%d Cleaned=%d."),
			Diagnostics.bWavesRunning ? TEXT("true") : TEXT("false"), Diagnostics.LastWaveIndex,
			Diagnostics.TrackedEntityCount, Diagnostics.ValidEntityCount, Diagnostics.ActorBridgeCount,
			Diagnostics.DeadActorCount, Diagnostics.WorldMinionActorCount, Diagnostics.RegisteredTargetCount,
			Diagnostics.IntentNoneCount, Diagnostics.IntentAdvancingCount, Diagnostics.IntentEngagingCount,
			Diagnostics.IntentAttackingCount, Diagnostics.IntentReturningCount, Diagnostics.MovingIntentCount,
			Diagnostics.InvalidLaneStateCount, Diagnostics.ActorTransformMismatchCount,
			Diagnostics.CumulativeSpawnedCount, Diagnostics.CumulativeCleanedUpCount);
	}

	FAutoConsoleCommandWithWorld DiagnosticsCommand(
		TEXT("sw.Minion.Diagnostics"),
		TEXT("Server/Standalone: prints M11 Mass Entity, Actor bridge, Target Registry and cleanup diagnostics for this World."),
		FConsoleCommandWithWorldDelegate::CreateStatic(&PrintDiagnostics),
		ECVF_Cheat);
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

	if (!EnsureRuntimeProcessorsRegisteredAuthority())
	{
		UE_LOG(LogSWMinionLaneWave, Error, TEXT("Unable to start minion waves because M11 runtime processors could not be registered."));
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
	CumulativeSpawnedMinionCount = 0;
	CumulativeCleanedUpMinionCount = 0;
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
	LaneRouteSnapshots.Reset();
	ReportedLaneSamplingFailures.Reset();
}

bool USWMinionLaneWaveSubsystem::TrySampleLaneTransform(
	const ESWLaneId LaneId,
	const float DistanceAlongLane,
	const ESWLaneDirection Direction,
	FTransform& OutTransform) const
{
	const FSWLaneRouteSnapshot* const Snapshot = LaneRouteSnapshots.Find(SWMinionLaneWave::ToKey(LaneId));
	return Snapshot && Snapshot->TrySampleTransform(DistanceAlongLane, Direction, OutTransform);
}

bool USWMinionLaneWaveSubsystem::TryProjectLaneDistance(
	const ESWLaneId LaneId,
	const FVector& WorldLocation,
	float& OutDistanceAlongLane) const
{
	const FSWLaneRouteSnapshot* const Snapshot = LaneRouteSnapshots.Find(SWMinionLaneWave::ToKey(LaneId));
	return Snapshot && Snapshot->TryProjectDistanceAlongLane(WorldLocation, OutDistanceAlongLane);
}

bool USWMinionLaneWaveSubsystem::TryGetLaneLength(const ESWLaneId LaneId, float& OutLength) const
{
	const FSWLaneRouteSnapshot* const Snapshot = LaneRouteSnapshots.Find(SWMinionLaneWave::ToKey(LaneId));
	if (!Snapshot || Snapshot->Length <= 0.f)
	{
		return false;
	}

	OutLength = Snapshot->Length;
	return true;
}

void USWMinionLaneWaveSubsystem::ReportLaneSamplingFailureOnce(const ESWLaneId LaneId)
{
	const uint8 LaneKey = SWMinionLaneWave::ToKey(LaneId);
	if (!ReportedLaneSamplingFailures.Contains(LaneKey))
	{
		ReportedLaneSamplingFailures.Add(LaneKey);
		UE_LOG(LogSWMinionLaneWave, Error, TEXT("M11 movement stopped for LaneId %d because its runtime route snapshot is invalid."), LaneKey);
	}
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

void USWMinionLaneWaveSubsystem::RemoveActiveMinionEntityAuthority(const FMassEntityHandle EntityHandle)
{
	if (!IsAuthorityWorld() || !EntityHandle.IsValid())
	{
		return;
	}

	const int32 EntityIndex = ActiveMinionEntities.IndexOfByKey(EntityHandle);
	if (EntityIndex != INDEX_NONE)
	{
		ActiveMinionEntities.RemoveAtSwap(EntityIndex, 1, EAllowShrinking::No);
		++CumulativeCleanedUpMinionCount;
	}
}

FSWMinionRuntimeDiagnostics USWMinionLaneWaveSubsystem::GetRuntimeDiagnosticsAuthority()
{
	FSWMinionRuntimeDiagnostics Diagnostics;
	if (!IsAuthorityWorld())
	{
		return Diagnostics;
	}

	Diagnostics.TrackedEntityCount = ActiveMinionEntities.Num();
	Diagnostics.LastWaveIndex = LastSpawnedWaveIndex;
	Diagnostics.bWavesRunning = bWavesRunning;
	Diagnostics.CumulativeSpawnedCount = CumulativeSpawnedMinionCount;
	Diagnostics.CumulativeCleanedUpCount = CumulativeCleanedUpMinionCount;

	UWorld* const World = GetWorld();
	UMassEntitySubsystem* const EntitySubsystem = World ? World->GetSubsystem<UMassEntitySubsystem>() : nullptr;
	if (EntitySubsystem)
	{
		FMassEntityManager& EntityManager = EntitySubsystem->GetMutableEntityManager();
		for (const FMassEntityHandle EntityHandle : ActiveMinionEntities)
		{
			if (!EntityManager.IsEntityValid(EntityHandle))
			{
				continue;
			}

			++Diagnostics.ValidEntityCount;
			const FSWMinionIntentFragment* const Intent = EntityManager.GetFragmentDataPtr<FSWMinionIntentFragment>(EntityHandle);
			if (!Intent)
			{
				++Diagnostics.IntentNoneCount;
			}
			else
			{
				switch (Intent->Behavior)
				{
				case ESWMinionBehaviorIntent::Advancing: ++Diagnostics.IntentAdvancingCount; break;
				case ESWMinionBehaviorIntent::Engaging: ++Diagnostics.IntentEngagingCount; break;
				case ESWMinionBehaviorIntent::Attacking: ++Diagnostics.IntentAttackingCount; break;
				case ESWMinionBehaviorIntent::Returning: ++Diagnostics.IntentReturningCount; break;
				case ESWMinionBehaviorIntent::None:
				default: ++Diagnostics.IntentNoneCount; break;
				}

				Diagnostics.MovingIntentCount += Intent->DesiredVelocity.IsNearlyZero() ? 0 : 1;
			}

			const FSWMinionLaneFragment* const Lane = EntityManager.GetFragmentDataPtr<FSWMinionLaneFragment>(EntityHandle);
			if (!Lane || Lane->LaneId == ESWLaneId::None || Lane->Direction == ESWLaneDirection::None)
			{
				++Diagnostics.InvalidLaneStateCount;
			}

			const FMassActorFragment* const ActorFragment = EntityManager.GetFragmentDataPtr<FMassActorFragment>(EntityHandle);
			const ASWCharacter_Minion* const MinionActor = ActorFragment ? Cast<ASWCharacter_Minion>(ActorFragment->Get()) : nullptr;
			if (!IsValid(MinionActor))
			{
				continue;
			}

			++Diagnostics.ActorBridgeCount;
			Diagnostics.DeadActorCount += MinionActor->IsDeadCommitted() ? 1 : 0;

			const FTransformFragment* const MassTransform = EntityManager.GetFragmentDataPtr<FTransformFragment>(EntityHandle);
			if (MassTransform && !MinionActor->GetActorLocation().Equals(MassTransform->GetTransform().GetLocation(), 1.f))
			{
				++Diagnostics.ActorTransformMismatchCount;
			}
		}
	}

	if (World)
	{
		for (TActorIterator<ASWCharacter_Minion> It(World); It; ++It)
		{
			if (!It->IsActorBeingDestroyed())
			{
				++Diagnostics.WorldMinionActorCount;
			}
		}

		if (USWMinionTargetRegistrySubsystem* const TargetRegistry = World->GetSubsystem<USWMinionTargetRegistrySubsystem>())
		{
			Diagnostics.RegisteredTargetCount = TargetRegistry->GetRegisteredTargetCount();
		}
	}

	return Diagnostics;
}

void USWMinionLaneWaveSubsystem::Deinitialize()
{
	StopWavesAuthority();
	UnregisterRuntimeProcessors();
	ActiveMinionEntities.Reset();
	Super::Deinitialize();
}

bool USWMinionLaneWaveSubsystem::IsAuthorityWorld() const
{
	const UWorld* const World = GetWorld();
	return World && (World->GetNetMode() == NM_Standalone || World->GetNetMode() == NM_ListenServer || World->GetNetMode() == NM_DedicatedServer);
}

bool USWMinionLaneWaveSubsystem::EnsureRuntimeProcessorsRegisteredAuthority()
{
	if (!IsAuthorityWorld())
	{
		return false;
	}

	UMassSimulationSubsystem* const SimulationSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UMassSimulationSubsystem>() : nullptr;
	if (!SimulationSubsystem || !SimulationSubsystem->IsSimulationStarted())
	{
		UE_LOG(LogSWMinionLaneWave, Error, TEXT("M11 runtime processors require an active UMassSimulationSubsystem."));
		return false;
	}

	if (!RuntimeProcessors.IsEmpty())
	{
		return true;
	}

	auto RegisterProcessor = [this, SimulationSubsystem]<typename TProcessor>()
	{
		TProcessor* const Processor = NewObject<TProcessor>(this);
		check(Processor);
		RuntimeProcessors.Add(Processor);
		SimulationSubsystem->RegisterDynamicProcessor(*Processor);
	};

	RegisterProcessor.template operator()<USWMinionLaneMovementProcessor>();
	RegisterProcessor.template operator()<USWMinionSeparationProcessor>();
	RegisterProcessor.template operator()<USWMinionActorSyncProcessor>();
	RegisterProcessor.template operator()<USWMinionTargetingProcessor>();
	RegisterProcessor.template operator()<USWMinionAttackProcessor>();
	RegisterProcessor.template operator()<USWMinionCleanupProcessor>();

	UE_LOG(LogSWMinionLaneWave, Display, TEXT("Registered %d M11 runtime Mass processors for World '%s'."), RuntimeProcessors.Num(), *GetNameSafe(GetWorld()));
	return true;
}

void USWMinionLaneWaveSubsystem::UnregisterRuntimeProcessors()
{
	UMassSimulationSubsystem* const SimulationSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UMassSimulationSubsystem>() : nullptr;
	if (SimulationSubsystem)
	{
		for (UMassProcessor* const Processor : RuntimeProcessors)
		{
			if (IsValid(Processor) && Processor->IsDynamic())
			{
				SimulationSubsystem->UnregisterDynamicProcessor(*Processor);
			}
		}
	}

	RuntimeProcessors.Reset();
}

bool USWMinionLaneWaveSubsystem::CacheLaneRoutesAuthority()
{
	LaneRoutes.Reset();
	LaneRouteSnapshots.Reset();
	ReportedLaneSamplingFailures.Reset();
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
		LaneRouteSnapshots.Add(LaneKey, MoveTemp(RouteSnapshot));
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
				SpawnRequest.FormationOffsets.Reserve(Entry.Count);

				for (const FVector& LocalOffset : Entry.FormationOffsets)
				{
					FTransform SpawnTransform = TeamSpawnTransform;
					SpawnTransform.SetLocation(TeamSpawnTransform.TransformPositionNoScale(LocalOffset));
					SpawnRequest.SpawnTransforms.Add(SpawnTransform);
					SpawnRequest.FormationOffsets.Add(LocalOffset);
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
	CumulativeSpawnedMinionCount += PerLaneTeamCount * UE_ARRAY_COUNT(SWMinionLaneWave::RequiredLanes) * UE_ARRAY_COUNT(SWMinionLaneWave::RequiredTeams);
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
