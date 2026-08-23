// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "Structures/SWStructureObjectiveSubsystem.h"

#include "Engine/World.h"
#include "GameMode/SWGameMode.h"
#include "HAL/IConsoleManager.h"
#include "Structures/SWDefenseStructure.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SWStructureObjectiveSubsystem)

DEFINE_LOG_CATEGORY_STATIC(LogSWStructureObjective, Log, All);

namespace
{
	const TCHAR* ToDiagnosticText(const ESWStructureKind StructureKind)
	{
		switch (StructureKind)
		{
		case ESWStructureKind::Tower: return TEXT("Tower");
		case ESWStructureKind::Crystal: return TEXT("Crystal");
		case ESWStructureKind::None:
		default: return TEXT("None");
		}
	}

	const TCHAR* ToDiagnosticText(const ESWTeamId TeamId)
	{
		switch (TeamId)
		{
		case ESWTeamId::TeamA: return TEXT("TeamA");
		case ESWTeamId::TeamB: return TEXT("TeamB");
		case ESWTeamId::None:
		default: return TEXT("None");
		}
	}

	void PrintStructureDiagnostics(UWorld* const World)
	{
		if (!World)
		{
			UE_LOG(LogSWStructureObjective, Warning, TEXT("Structure diagnostics unavailable: no World context."));
			return;
		}

		if (World->GetNetMode() == NM_Client)
		{
			UE_LOG(LogSWStructureObjective, Warning, TEXT("Structure diagnostics must run in the server or Standalone console."));
			return;
		}

		const USWStructureObjectiveSubsystem* const ObjectiveSubsystem = World->GetSubsystem<USWStructureObjectiveSubsystem>();
		if (!ObjectiveSubsystem)
		{
			UE_LOG(LogSWStructureObjective, Warning, TEXT("Structure diagnostics unavailable: no objective subsystem for World '%s'."), *World->GetName());
			return;
		}

		ObjectiveSubsystem->LogDiagnosticsAuthority();
	}

	FAutoConsoleCommandWithWorld StructureDiagnosticsCommand(
		TEXT("sw.Structure.Diagnostics"),
		TEXT("Server/Standalone: prints every registered defense structure and whether it can currently receive damage."),
		FConsoleCommandWithWorldDelegate::CreateStatic(&PrintStructureDiagnostics),
		ECVF_Cheat);
}

bool USWStructureObjectiveSubsystem::ShouldCreateSubsystem(UObject* const Outer) const
{
	if (!Super::ShouldCreateSubsystem(Outer))
	{
		return false;
	}

	const UWorld* const World = Cast<UWorld>(Outer);
	if (!World || (World->WorldType != EWorldType::Game && World->WorldType != EWorldType::PIE))
	{
		return false;
	}

	const ENetMode NetMode = World->GetNetMode();
	return NetMode == NM_Standalone || NetMode == NM_ListenServer || NetMode == NM_DedicatedServer;
}

void USWStructureObjectiveSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
}

void USWStructureObjectiveSubsystem::Deinitialize()
{
	if (UWorld* const World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(GraphRebuildTimer);
	}

	for (const TPair<FName, FRegisteredStructure>& Pair : StructuresById)
	{
		if (ASWDefenseStructure* const Structure = Pair.Value.Structure.Get())
		{
			Structure->GetOnDeathDelegate().Remove(Pair.Value.DeathDelegateHandle);
		}
	}

	StructuresById.Reset();
	DependentsByPrerequisiteId.Reset();
	DestroyedStructureIds.Reset();
	bGraphRebuildScheduled = false;
	bGraphValid = false;
	RegistrationValidationError.Reset();
	LastValidationError.Reset();

	Super::Deinitialize();
}

bool USWStructureObjectiveSubsystem::RegisterStructure(ASWDefenseStructure& Structure)
{
	if (!IsAuthorityWorld() || !Structure.HasAuthority())
	{
		return false;
	}

	const FName StructureId = Structure.GetStructureId();
	if (StructureId.IsNone())
	{
		RecordRegistrationError(FString::Printf(TEXT("Structure '%s' has no StructureId."), *Structure.GetName()));
		bGraphValid = false;
		ScheduleGraphRebuildAuthority();
		return false;
	}

	if (FRegisteredStructure* const Existing = StructuresById.Find(StructureId))
	{
		if (Existing->Structure.Get() == &Structure)
		{
			return true;
		}

		RecordRegistrationError(FString::Printf(TEXT("StructureId '%s' is assigned to more than one structure."), *StructureId.ToString()));
		bGraphValid = false;
		ScheduleGraphRebuildAuthority();
		return false;
	}

	FRegisteredStructure RegisteredStructure;
	RegisteredStructure.Structure = &Structure;
	RegisteredStructure.DeathDelegateHandle = Structure.GetOnDeathDelegate().AddUObject(
		this,
		&ThisClass::HandleRegisteredStructureDeath,
		TWeakObjectPtr<ASWDefenseStructure>(&Structure));
	StructuresById.Add(StructureId, MoveTemp(RegisteredStructure));

	ScheduleGraphRebuildAuthority();
	return true;
}

void USWStructureObjectiveSubsystem::UnregisterStructure(ASWDefenseStructure& Structure)
{
	if (!IsAuthorityWorld())
	{
		return;
	}

	const FName StructureId = Structure.GetStructureId();
	FRegisteredStructure* const RegisteredStructure = StructuresById.Find(StructureId);
	if (!RegisteredStructure || RegisteredStructure->Structure.Get() != &Structure)
	{
		return;
	}

	Structure.GetOnDeathDelegate().Remove(RegisteredStructure->DeathDelegateHandle);
	StructuresById.Remove(StructureId);
	DependentsByPrerequisiteId.Reset();
	DestroyedStructureIds.Remove(StructureId);
	ScheduleGraphRebuildAuthority();
}

bool USWStructureObjectiveSubsystem::IsAuthorityWorld() const
{
	const UWorld* const World = GetWorld();
	if (!World)
	{
		return false;
	}

	const ENetMode NetMode = World->GetNetMode();
	return NetMode == NM_Standalone || NetMode == NM_ListenServer || NetMode == NM_DedicatedServer;
}

void USWStructureObjectiveSubsystem::LogDiagnosticsAuthority() const
{
	if (!IsAuthorityWorld())
	{
		UE_LOG(LogSWStructureObjective, Warning, TEXT("Structure diagnostics must run in an authority World."));
		return;
	}

	TArray<const ASWDefenseStructure*> Structures;
	Structures.Reserve(StructuresById.Num());
	for (const TPair<FName, FRegisteredStructure>& Pair : StructuresById)
	{
		if (const ASWDefenseStructure* const Structure = Pair.Value.Structure.Get())
		{
			Structures.Add(Structure);
		}
	}

	Structures.Sort([](const ASWDefenseStructure& Left, const ASWDefenseStructure& Right)
	{
		// 仅诊断输出排序；用字符串比较避免依赖 FName 的比较实现细节。
		return Left.GetStructureId().ToString() < Right.GetStructureId().ToString();
	});

	UE_LOG(LogSWStructureObjective, Display,
		TEXT("Structure Diagnostics: GraphValid=%s Registered=%d ValidationError=%s"),
		bGraphValid ? TEXT("true") : TEXT("false"),
		Structures.Num(),
		LastValidationError.IsEmpty() ? TEXT("None") : *LastValidationError);

	for (const ASWDefenseStructure* const Structure : Structures)
	{
		const FString Prerequisites = FString::JoinBy(Structure->GetPrerequisiteStructureIds(), TEXT(", "),
			[](const FName PrerequisiteId) { return PrerequisiteId.ToString(); });
		UE_LOG(LogSWStructureObjective, Display,
			TEXT("  Name=%s Id=%s Kind=%s Team=%s Lane=%d Vulnerable=%s Dead=%s Prerequisites=[%s]"),
			*Structure->GetName(),
			*Structure->GetStructureId().ToString(),
			ToDiagnosticText(Structure->GetStructureKind()),
			ToDiagnosticText(Structure->GetTeamId()),
			static_cast<int32>(Structure->GetLaneId()),
			Structure->IsVulnerable() ? TEXT("true") : TEXT("false"),
			Structure->IsDeadCommitted() ? TEXT("true") : TEXT("false"),
			Prerequisites.IsEmpty() ? TEXT("None") : *Prerequisites);
	}
}

void USWStructureObjectiveSubsystem::ScheduleGraphRebuildAuthority()
{
	if (!IsAuthorityWorld() || bGraphRebuildScheduled)
	{
		return;
	}

	UWorld* const World = GetWorld();
	if (!World)
	{
		return;
	}

	bGraphRebuildScheduled = true;
	GraphRebuildTimer = World->GetTimerManager().SetTimerForNextTick(this, &ThisClass::RebuildGraphAuthority);
}

void USWStructureObjectiveSubsystem::RebuildGraphAuthority()
{
	GraphRebuildTimer.Invalidate();
	bGraphRebuildScheduled = false;
	DependentsByPrerequisiteId.Reset();
	bGraphValid = ValidateGraphAuthority();

	if (!bGraphValid)
	{
		for (const TPair<FName, FRegisteredStructure>& Pair : StructuresById)
		{
			if (ASWDefenseStructure* const Structure = Pair.Value.Structure.Get())
			{
				Structure->SetVulnerableAuthority(false);
			}
		}

		UE_LOG(LogSWStructureObjective, Error, TEXT("Structure objective graph is invalid: %s"), *LastValidationError);
		return;
	}

	// 兼容关卡恢复、测试命令或重建发生在死亡之后：已死亡结构始终视为已满足其后继前置条件。
	for (auto It = DestroyedStructureIds.CreateIterator(); It; ++It)
	{
		if (!StructuresById.Contains(*It))
		{
			It.RemoveCurrent();
		}
	}
	for (const TPair<FName, FRegisteredStructure>& Pair : StructuresById)
	{
		if (const ASWDefenseStructure* const Structure = Pair.Value.Structure.Get(); Structure && Structure->IsDeadCommitted())
		{
			DestroyedStructureIds.Add(Pair.Key);
		}
	}

	ApplyVulnerabilityStatesAuthority();
	UE_LOG(LogSWStructureObjective, Display, TEXT("Validated structure objective graph with %d structures."), StructuresById.Num());
}

bool USWStructureObjectiveSubsystem::ValidateGraphAuthority()
{
	LastValidationError = RegistrationValidationError;
	if (StructuresById.IsEmpty())
	{
		RecordValidationError(TEXT("No defense structures were registered."));
		return false;
	}

	int32 TeamACrystalCount = 0;
	int32 TeamBCrystalCount = 0;
	for (const TPair<FName, FRegisteredStructure>& Pair : StructuresById)
	{
		const FName StructureId = Pair.Key;
		const ASWDefenseStructure* const Structure = Pair.Value.Structure.Get();
		if (!Structure)
		{
			RecordValidationError(FString::Printf(TEXT("StructureId '%s' has an invalid Actor reference."), *StructureId.ToString()));
			continue;
		}

		if (!Structure->GetStructureDefinition())
		{
			RecordValidationError(FString::Printf(TEXT("StructureId '%s' has no StructureDefinition."), *StructureId.ToString()));
		}
		if (Structure->GetTeamId() != ESWTeamId::TeamA && Structure->GetTeamId() != ESWTeamId::TeamB)
		{
			RecordValidationError(FString::Printf(TEXT("StructureId '%s' has an invalid TeamId."), *StructureId.ToString()));
		}

		switch (Structure->GetStructureKind())
		{
		case ESWStructureKind::Tower:
			if (Structure->GetLaneId() == ESWLaneId::None)
			{
				RecordValidationError(FString::Printf(TEXT("Tower '%s' must use Top, Middle, or Bottom LaneId."), *StructureId.ToString()));
			}
			break;
		case ESWStructureKind::Crystal:
			if (Structure->GetLaneId() != ESWLaneId::None)
			{
				RecordValidationError(FString::Printf(TEXT("Crystal '%s' must use LaneId None."), *StructureId.ToString()));
			}
			if (Structure->GetTeamId() == ESWTeamId::TeamA)
			{
				++TeamACrystalCount;
			}
			else if (Structure->GetTeamId() == ESWTeamId::TeamB)
			{
				++TeamBCrystalCount;
			}
			break;
		case ESWStructureKind::None:
		default:
			RecordValidationError(FString::Printf(TEXT("StructureId '%s' has StructureKind None."), *StructureId.ToString()));
			break;
		}

		TSet<FName> UniquePrerequisites;
		for (const FName PrerequisiteId : Structure->GetPrerequisiteStructureIds())
		{
			if (PrerequisiteId.IsNone())
			{
				RecordValidationError(FString::Printf(TEXT("StructureId '%s' has an empty prerequisite ID."), *StructureId.ToString()));
				continue;
			}
			if (PrerequisiteId == StructureId)
			{
				RecordValidationError(FString::Printf(TEXT("StructureId '%s' cannot depend on itself."), *StructureId.ToString()));
				continue;
			}
			if (UniquePrerequisites.Contains(PrerequisiteId))
			{
				RecordValidationError(FString::Printf(TEXT("StructureId '%s' repeats prerequisite '%s'."), *StructureId.ToString(), *PrerequisiteId.ToString()));
				continue;
			}
			UniquePrerequisites.Add(PrerequisiteId);
			if (!StructuresById.Contains(PrerequisiteId))
			{
				RecordValidationError(FString::Printf(TEXT("StructureId '%s' references missing prerequisite '%s'."), *StructureId.ToString(), *PrerequisiteId.ToString()));
				continue;
			}

			DependentsByPrerequisiteId.FindOrAdd(PrerequisiteId).Add(StructureId);
		}
	}

	if (TeamACrystalCount != 1 || TeamBCrystalCount != 1)
	{
		RecordValidationError(FString::Printf(TEXT("The map requires exactly one crystal per team, found TeamA=%d TeamB=%d."), TeamACrystalCount, TeamBCrystalCount));
	}

	TMap<FName, uint8> VisitState;
	TFunction<bool(const FName&)> Visit;
	Visit = [this, &VisitState, &Visit](const FName StructureId)
	{
		const uint8 State = VisitState.FindRef(StructureId);
		if (State == 1)
		{
			RecordValidationError(FString::Printf(TEXT("Structure prerequisite graph contains a cycle at '%s'."), *StructureId.ToString()));
			return false;
		}
		if (State == 2)
		{
			return true;
		}

		VisitState.Add(StructureId, 1);
		const FRegisteredStructure* const RegisteredStructure = StructuresById.Find(StructureId);
		const ASWDefenseStructure* const Structure = RegisteredStructure ? RegisteredStructure->Structure.Get() : nullptr;
		if (Structure)
		{
			for (const FName PrerequisiteId : Structure->GetPrerequisiteStructureIds())
			{
				if (StructuresById.Contains(PrerequisiteId) && !Visit(PrerequisiteId))
				{
					return false;
				}
			}
		}
		VisitState.Add(StructureId, 2);
		return true;
	};

	for (const TPair<FName, FRegisteredStructure>& Pair : StructuresById)
	{
		if (!Visit(Pair.Key))
		{
			break;
		}
	}

	return LastValidationError.IsEmpty();
}

void USWStructureObjectiveSubsystem::ApplyVulnerabilityStatesAuthority()
{
	for (const TPair<FName, FRegisteredStructure>& Pair : StructuresById)
	{
		ASWDefenseStructure* const Structure = Pair.Value.Structure.Get();
		if (!Structure)
		{
			continue;
		}

		const TArray<FName>& PrerequisiteIds = Structure->GetPrerequisiteStructureIds();
		bool bShouldBecomeVulnerable = PrerequisiteIds.IsEmpty();

		if (Structure->GetStructureKind() == ESWStructureKind::Crystal)
		{
			// 水晶采用任一前置被摧毁即可解锁的规则。
			bShouldBecomeVulnerable = PrerequisiteIds.ContainsByPredicate([this](const FName PrerequisiteId)
			{
				return DestroyedStructureIds.Contains(PrerequisiteId);
			});
		}
		else
		{
			// 防御塔保持全部前置均被摧毁才解锁的推进规则。
			bShouldBecomeVulnerable = PrerequisiteIds.ContainsByPredicate([this](const FName PrerequisiteId)
			{
				return !DestroyedStructureIds.Contains(PrerequisiteId);
			}) == false;
		}

		Structure->SetVulnerableAuthority(bShouldBecomeVulnerable);
	}
}

void USWStructureObjectiveSubsystem::HandleRegisteredStructureDeath(
	const FSWDeathContext& DeathContext,
	const TWeakObjectPtr<ASWDefenseStructure> Structure)
{
	static_cast<void>(DeathContext);

	ASWDefenseStructure* const DeadStructure = Structure.Get();
	if (!IsAuthorityWorld() || !bGraphValid || !DeadStructure || !DeadStructure->IsDeadCommitted())
	{
		return;
	}

	const FName StructureId = DeadStructure->GetStructureId();
	if (StructureId.IsNone() || DestroyedStructureIds.Contains(StructureId))
	{
		return;
	}

	DestroyedStructureIds.Add(StructureId);
	ApplyVulnerabilityStatesAuthority();

	// 目标子系统仅报告被毁水晶所属队伍；胜负与同帧平局由服务器 GameMode 统一裁决。
	if (DeadStructure->GetStructureKind() == ESWStructureKind::Crystal)
	{
		if (ASWGameMode* const SWGameMode = Cast<ASWGameMode>(GetWorld()->GetAuthGameMode()))
		{
			SWGameMode->ReportCrystalDestroyed(DeadStructure->GetTeamId());
		}
	}
}

void USWStructureObjectiveSubsystem::RecordValidationError(const FString& Error)
{
	if (!LastValidationError.IsEmpty())
	{
		LastValidationError.Append(TEXT(" | "));
	}
	LastValidationError.Append(Error);
}

void USWStructureObjectiveSubsystem::RecordRegistrationError(const FString& Error)
{
	if (!RegistrationValidationError.IsEmpty())
	{
		RegistrationValidationError.Append(TEXT(" | "));
	}
	RegistrationValidationError.Append(Error);
}
