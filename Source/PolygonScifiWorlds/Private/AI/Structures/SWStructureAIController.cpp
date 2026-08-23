// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "AI/Structures/SWStructureAIController.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BrainComponent.h"
#include "Engine/World.h"
#include "GameFramework/GameMode.h"
#include "GameFramework/Pawn.h"
#include "GameState/SWGameState.h"
#include "Structures/SWDefenseStructure.h"
#include "Structures/SWStructureDefinition.h"
#include "Structures/SWStructureTargetingComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SWStructureAIController)

const FName ASWStructureAIController::TargetActorBlackboardKey(TEXT("TargetActor"));
const FName ASWStructureAIController::CombatEnabledBlackboardKey(TEXT("CombatEnabled"));

ASWStructureAIController::ASWStructureAIController()
{
	PrimaryActorTick.bCanEverTick = false;
	bStartAILogicOnPossess = false;
	bStopAILogicOnUnposses = true;
}

void ASWStructureAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (!HasAuthority())
	{
		return;
	}

	ASWDefenseStructure* const Structure = Cast<ASWDefenseStructure>(InPawn);
	USWStructureTargetingComponent* const Targeting = Structure ? Structure->GetTargetingComponent() : nullptr;
	const USWStructureDefinition* const Definition = Structure ? Structure->GetStructureDefinition() : nullptr;
	if (!Structure || !Targeting || !Definition || !Definition->BehaviorTree)
	{
		UE_LOG(LogTemp, Error, TEXT("Structure AIController 无法初始化：Pawn、TargetingComponent 或 BehaviorTree 缺失。"));
		return;
	}

	StructureOwner = Structure;
	TargetChangedDelegateHandle = Targeting->GetOnTargetChangedDelegate().AddUObject(this, &ThisClass::HandleTargetChanged);
	StructureDeathDelegateHandle = Structure->GetOnDeathDelegate().AddUObject(this, &ThisClass::HandleStructureDeath);

	ASWGameState* const GameState = GetWorld() ? GetWorld()->GetGameState<ASWGameState>() : nullptr;
	if (!GameState)
	{
		UE_LOG(LogTemp, Error, TEXT("Structure AIController 无法初始化：缺少 ASWGameState。"));
		UnbindAuthorityDelegates();
		StructureOwner.Reset();
		return;
	}

	ObservedGameState = GameState;
	MatchStateChangedDelegateHandle = GameState->OnSWMatchStateChanged.AddUObject(this, &ThisClass::HandleMatchStateChanged);
	HandleMatchStateChanged(GameState->GetMatchState());
}

void ASWStructureAIController::OnUnPossess()
{
	UnbindAuthorityDelegates();
	StopBehaviorTreeAuthority(TEXT("Structure unpossessed"));
	StructureOwner.Reset();
	ObservedGameState.Reset();

	Super::OnUnPossess();
}

bool ASWStructureAIController::TryActivateStructureAttackAbilityAuthority(AActor* TargetActor)
{
	ASWDefenseStructure* const Structure = StructureOwner.Get();
	return HasAuthority() && Structure && Structure->TryActivateStructureAttackAbilityAuthority(TargetActor);
}

void ASWStructureAIController::HandleTargetChanged(const TWeakObjectPtr<AActor> NewTarget)
{
	UpdateBlackboardTargetAuthority(NewTarget.Get());
}

void ASWStructureAIController::HandleMatchStateChanged(const FName NewMatchState)
{
	ASWDefenseStructure* const Structure = StructureOwner.Get();
	const bool bShouldEnableCombat = Structure && !Structure->IsDeadCommitted() && NewMatchState == MatchState::InProgress;
	if (!bShouldEnableCombat)
	{
		SetCombatEnabledAuthority(false);
		UpdateBlackboardTargetAuthority(nullptr);
		StopBehaviorTreeAuthority(TEXT("Match not in progress or structure dead"));
		if (Structure)
		{
			Structure->CancelStructureAttackAbilityAuthority();
		}
		return;
	}

	StartBehaviorTreeAuthority();
	SetCombatEnabledAuthority(true);
	UpdateBlackboardTargetAuthority(Structure->GetTargetingComponent()->GetCurrentTargetAuthority());
}

void ASWStructureAIController::HandleStructureDeath(const FSWDeathContext& DeathContext)
{
	static_cast<void>(DeathContext);
	SetCombatEnabledAuthority(false);
	UpdateBlackboardTargetAuthority(nullptr);
	StopBehaviorTreeAuthority(TEXT("Structure dead"));
}

void ASWStructureAIController::StartBehaviorTreeAuthority()
{
	if (bBehaviorTreeRunning)
	{
		return;
	}

	const ASWDefenseStructure* const Structure = StructureOwner.Get();
	const USWStructureDefinition* const Definition = Structure ? Structure->GetStructureDefinition() : nullptr;
	if (!Definition || !Definition->BehaviorTree)
	{
		return;
	}

	bBehaviorTreeRunning = RunBehaviorTree(Definition->BehaviorTree);
	if (!bBehaviorTreeRunning)
	{
		UE_LOG(LogTemp, Error, TEXT("结构 %s 无法运行 BehaviorTree %s。"), *GetNameSafe(Structure), *GetNameSafe(Definition->BehaviorTree));
	}
}

void ASWStructureAIController::StopBehaviorTreeAuthority(const FString& Reason)
{
	if (UBrainComponent* const Brain = GetBrainComponent())
	{
		Brain->StopLogic(Reason);
	}
	bBehaviorTreeRunning = false;
}

void ASWStructureAIController::UpdateBlackboardTargetAuthority(AActor* NewTarget)
{
	if (UBlackboardComponent* const BlackboardComponent = GetBlackboardComponent())
	{
		if (NewTarget)
		{
			BlackboardComponent->SetValueAsObject(TargetActorBlackboardKey, NewTarget);
		}
		else
		{
			BlackboardComponent->ClearValue(TargetActorBlackboardKey);
		}
	}
}

void ASWStructureAIController::SetCombatEnabledAuthority(const bool bEnabled)
{
	if (UBlackboardComponent* const BlackboardComponent = GetBlackboardComponent())
	{
		BlackboardComponent->SetValueAsBool(CombatEnabledBlackboardKey, bEnabled);
	}
}

void ASWStructureAIController::UnbindAuthorityDelegates()
{
	if (USWStructureTargetingComponent* const Targeting = StructureOwner.IsValid() ? StructureOwner->GetTargetingComponent() : nullptr)
	{
		Targeting->GetOnTargetChangedDelegate().Remove(TargetChangedDelegateHandle);
	}
	if (ASWDefenseStructure* const Structure = StructureOwner.Get())
	{
		Structure->GetOnDeathDelegate().Remove(StructureDeathDelegateHandle);
	}
	if (ASWGameState* const GameState = ObservedGameState.Get())
	{
		GameState->OnSWMatchStateChanged.Remove(MatchStateChangedDelegateHandle);
	}

	TargetChangedDelegateHandle.Reset();
	StructureDeathDelegateHandle.Reset();
	MatchStateChangedDelegateHandle.Reset();
}
