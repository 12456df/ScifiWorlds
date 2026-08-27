// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "Player/SWPlayerController.h"

#include "AbilitySystem/SWAbilitySystemComponent.h"
#include "Character/SWCharacter_Base.h"
#include "Character/SWCharacter_Player.h"
#include "Components/MeshComponent.h"
#include "Equipment/SWEquipmentItemDefinition.h"
#include "Equipment/SWEquipmentTypes.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "EngineUtils.h"
#include "Input/SWInputConfig.h"
#include "Interaction/SWCombatInterface.h"
#include "Interaction/SWTeamInterface.h"
#include "Player/SWPlayerState.h"
#include "GameState/SWGameState.h"
#include "Shop/SWShopCatalogData.h"
#include "UI/HUD/SWHUD.h"
#include "UI/World/SWTargetHealthBarComponent.h"
#include "Weapon/SWWeapon.h"

void ASWPlayerController::BeginPlay()
{
	Super::BeginPlay();

	ApplyGameplayMappingContext();
}

void ASWPlayerController::BeginPlayingState()
{
	Super::BeginPlayingState();

	RefreshOverlayWidgetControllers();
}

void ASWPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearEnemyHighlightPresentation();
	RemoveGameplayMappingContext();

	Super::EndPlay(EndPlayReason);
}

void ASWPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	RefreshOverlayWidgetControllers();
}

void ASWPlayerController::PostProcessInput(const float DeltaTime, const bool bGamePaused)
{
	Super::PostProcessInput(DeltaTime, bGamePaused);

	if (ASWPlayerState* SWPlayerState = GetPlayerState<ASWPlayerState>())
	{
		if (USWAbilitySystemComponent* AbilitySystemComponent = Cast<USWAbilitySystemComponent>(SWPlayerState->GetAbilitySystemComponent()))
		{
			AbilitySystemComponent->ProcessAbilityInput(DeltaTime, bGamePaused);
		}
	}
}

void ASWPlayerController::ClientShowDamageNumber_Implementation(const FSWDamageNumberPayload& Payload)
{
	if (!IsLocalController())
	{
		return;
	}

	BP_ShowDamageNumber(Payload);
}

void ASWPlayerController::ClientShowDamagedTargetHealthBar_Implementation(AActor* const TargetActor)
{
	if (!IsLocalController() || !IsValid(TargetActor))
	{
		return;
	}

	if (USWTargetHealthBarComponent* const HealthBarComponent = TargetActor->FindComponentByClass<USWTargetHealthBarComponent>())
	{
		HealthBarComponent->ShowForLocalAttacker();
	}
}

void ASWPlayerController::PlayerTick(const float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	if (!IsLocalController())
	{
		return;
	}

	EnemyHighlightRefreshAccumulator += FMath::Max(0.f, DeltaTime);
	if (EnemyHighlightRefreshAccumulator < FMath::Max(0.016f, EnemyHighlightRefreshIntervalSeconds))
	{
		return;
	}

	EnemyHighlightRefreshAccumulator = 0.f;
	UpdateEnemyHighlightPresentation();
}

void ASWPlayerController::RequestActiveAbilityUpgrade(const FGameplayTag InputTag)
{
	if (!InputTag.IsValid())
	{
		return;
	}

	if (HasAuthority())
	{
		ProcessActiveAbilityUpgradeRequestAuthority(InputTag);
		return;
	}

	ServerRequestActiveAbilityUpgrade(InputTag);
}

void ASWPlayerController::RequestPurchaseItem(const FPrimaryAssetId& ItemDefinitionId)
{
	if (!ItemDefinitionId.IsValid())
	{
		return;
	}

	if (HasAuthority())
	{
		ProcessPurchaseRequestAuthority(ItemDefinitionId);
		return;
	}

	ServerRequestPurchaseItem(ItemDefinitionId);
}

void ASWPlayerController::RequestSellEquipmentSlot(const int32 SlotIndex)
{
	if (HasAuthority())
	{
		ProcessSellRequestAuthority(SlotIndex);
		return;
	}

	ServerRequestSellEquipmentSlot(SlotIndex);
}

void ASWPlayerController::ServerRequestActiveAbilityUpgrade_Implementation(const FGameplayTag InputTag)
{
	ProcessActiveAbilityUpgradeRequestAuthority(InputTag);
}

void ASWPlayerController::ProcessActiveAbilityUpgradeRequestAuthority(const FGameplayTag InputTag)
{
	ASWPlayerState* const SWPlayerState = GetPlayerState<ASWPlayerState>();
	USWAbilitySystemComponent* const AbilitySystemComponent = SWPlayerState
		? Cast<USWAbilitySystemComponent>(SWPlayerState->GetAbilitySystemComponent())
		: nullptr;
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->TryUpgradeActiveAbilityAuthority(InputTag);
	}
}

void ASWPlayerController::ServerRequestPurchaseItem_Implementation(const FPrimaryAssetId ItemDefinitionId)
{
	ProcessPurchaseRequestAuthority(ItemDefinitionId);
}

void ASWPlayerController::ServerRequestSellEquipmentSlot_Implementation(const int32 SlotIndex)
{
	ProcessSellRequestAuthority(SlotIndex);
}

void ASWPlayerController::ClientShopTransactionFailed_Implementation(const ESWShopTransactionFailure Failure)
{
	if (IsLocalController())
	{
		BP_OnShopTransactionFailed(Failure);
	}
}

void ASWPlayerController::ProcessPurchaseRequestAuthority(const FPrimaryAssetId& ItemDefinitionId)
{
	ASWPlayerState* const SWPlayerState = GetPlayerState<ASWPlayerState>();
	const ASWCharacter_Base* const ControlledCharacter = Cast<ASWCharacter_Base>(GetPawn());
	const ASWGameState* const SWGameState = GetWorld() ? GetWorld()->GetGameState<ASWGameState>() : nullptr;
	const USWShopCatalogData* const Catalog = SWGameState ? SWGameState->GetShopCatalogData() : nullptr;
	const USWEquipmentItemDefinition* const ItemDefinition = Catalog ? Catalog->FindItemDefinition(ItemDefinitionId) : nullptr;

	ESWShopTransactionFailure Failure = ESWShopTransactionFailure::None;
	if (!ItemDefinition)
	{
		Failure = ESWShopTransactionFailure::InvalidItem;
	}
	else if (!ControlledCharacter || ControlledCharacter->IsDeadCommitted())
	{
		Failure = ESWShopTransactionFailure::Dead;
	}
	else if (!SWPlayerState || !SWPlayerState->CanTradeAtShop())
	{
		Failure = ESWShopTransactionFailure::NotInShopZone;
	}
	else if (SWPlayerState->GetOwnedEquipmentCount(ItemDefinitionId) >= ItemDefinition->MaxOwnedCount)
	{
		Failure = ESWShopTransactionFailure::OwnershipLimitReached;
	}
	else if (SWPlayerState->GetGold() < ItemDefinition->PurchasePrice)
	{
		Failure = ESWShopTransactionFailure::InsufficientGold;
	}
	else if (SWPlayerState->GetEquipmentSlots().IndexOfByPredicate([](const FSWEquipmentSlot& Slot) { return Slot.IsEmpty(); }) == INDEX_NONE)
	{
		Failure = ESWShopTransactionFailure::InventoryFull;
	}
	else if (!SWPlayerState->TryPurchaseEquipmentAuthority(ItemDefinition))
	{
		Failure = ESWShopTransactionFailure::EffectApplicationFailed;
	}

	if (Failure != ESWShopTransactionFailure::None)
	{
		ClientShopTransactionFailed(Failure);
	}
}

void ASWPlayerController::ProcessSellRequestAuthority(const int32 SlotIndex)
{
	ASWPlayerState* const SWPlayerState = GetPlayerState<ASWPlayerState>();
	const ASWCharacter_Base* const ControlledCharacter = Cast<ASWCharacter_Base>(GetPawn());
	ESWShopTransactionFailure Failure = ESWShopTransactionFailure::None;
	if (!ControlledCharacter || ControlledCharacter->IsDeadCommitted())
	{
		Failure = ESWShopTransactionFailure::Dead;
	}
	else if (!SWPlayerState || !SWPlayerState->CanTradeAtShop())
	{
		Failure = ESWShopTransactionFailure::NotInShopZone;
	}
	else if (!SWPlayerState->GetEquipmentSlots().IsValidIndex(SlotIndex) || SWPlayerState->GetEquipmentSlots()[SlotIndex].IsEmpty())
	{
		Failure = ESWShopTransactionFailure::InvalidSlot;
	}
	else if (!SWPlayerState->TrySellEquipmentSlotAuthority(SlotIndex))
	{
		Failure = ESWShopTransactionFailure::EffectApplicationFailed;
	}

	if (Failure != ESWShopTransactionFailure::None)
	{
		ClientShopTransactionFailed(Failure);
	}
}

void ASWPlayerController::ApplyGameplayMappingContext()
{
	if (!IsLocalController() || !InputConfig || !InputConfig->DefaultMappingContext)
	{
		return;
	}

	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			InputSubsystem->AddMappingContext(InputConfig->DefaultMappingContext, 0);
		}
	}
}

void ASWPlayerController::RemoveGameplayMappingContext()
{
	if (!IsLocalController() || !InputConfig || !InputConfig->DefaultMappingContext)
	{
		return;
	}

	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			InputSubsystem->RemoveMappingContext(InputConfig->DefaultMappingContext);
		}
	}
}

void ASWPlayerController::RefreshOverlayWidgetControllers()
{
	if (!IsLocalController())
	{
		return;
	}

	if (ASWHUD* SWHUD = GetHUD<ASWHUD>())
	{
		SWHUD->RefreshOverlayWidgetControllers();
	}
}

void ASWPlayerController::UpdateEnemyHighlightPresentation()
{
	if (!bEnableEnemyHighlights || !EnemyHighlightOverlayMaterial || !GetPawn() || !GetWorld())
	{
		ClearEnemyHighlightPresentation();
		return;
	}

	TSet<TWeakObjectPtr<UMeshComponent>> TouchedComponents;
	AActor* const FocusedActor = FindCrosshairEnemyHighlightTarget();
	UMaterialInterface* const FocusedMaterial = FocusedEnemyHighlightOverlayMaterial
		? FocusedEnemyHighlightOverlayMaterial.Get()
		: EnemyHighlightOverlayMaterial.Get();
	FVector ViewLocation;
	FRotator ViewRotation;
	GetPlayerViewPoint(ViewLocation, ViewRotation);

	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* const CandidateActor = *It;
		if (!IsHostileHighlightTarget(CandidateActor) || !IsWithinEnemyHighlightDistance(*CandidateActor, ViewLocation))
		{
			continue;
		}

		UMaterialInterface* const DesiredMaterial = CandidateActor == FocusedActor
			? FocusedMaterial
			: EnemyHighlightOverlayMaterial.Get();
		ApplyLocalHighlightToActor(*CandidateActor, *DesiredMaterial, TouchedComponents);
	}

	TArray<TWeakObjectPtr<UMeshComponent>> ComponentsToRestore;
	for (const TPair<TWeakObjectPtr<UMeshComponent>, FSWLocalEnemyHighlightComponentState>& Pair : OriginalEnemyHighlightComponentStates)
	{
		if (!TouchedComponents.Contains(Pair.Key))
		{
			ComponentsToRestore.Add(Pair.Key);
		}
	}

	for (const TWeakObjectPtr<UMeshComponent>& ComponentKey : ComponentsToRestore)
	{
		if (UMeshComponent* const Component = ComponentKey.Get())
		{
			if (const FSWLocalEnemyHighlightComponentState* const OriginalState = OriginalEnemyHighlightComponentStates.Find(ComponentKey))
			{
				Component->SetOverlayMaterial(OriginalState->OverlayMaterial.Get());
				Component->SetOverlayMaterialMaxDrawDistance(OriginalState->OverlayMaterialMaxDrawDistance);
			}
		}

		OriginalEnemyHighlightComponentStates.Remove(ComponentKey);
	}
}

void ASWPlayerController::ClearEnemyHighlightPresentation()
{
	for (const TPair<TWeakObjectPtr<UMeshComponent>, FSWLocalEnemyHighlightComponentState>& Pair : OriginalEnemyHighlightComponentStates)
	{
		if (UMeshComponent* const Component = Pair.Key.Get())
		{
			Component->SetOverlayMaterial(Pair.Value.OverlayMaterial.Get());
			Component->SetOverlayMaterialMaxDrawDistance(Pair.Value.OverlayMaterialMaxDrawDistance);
		}
	}

	OriginalEnemyHighlightComponentStates.Empty();
	EnemyHighlightRefreshAccumulator = 0.f;
}

bool ASWPlayerController::IsHostileHighlightTarget(AActor* const CandidateActor) const
{
	const APawn* const LocalPawn = GetPawn();
	if (!CandidateActor || CandidateActor == LocalPawn
		|| !LocalPawn || !CandidateActor->Implements<USWTeamInterface>()
		|| !CandidateActor->Implements<USWCombatInterface>()
		|| !LocalPawn->Implements<USWTeamInterface>())
	{
		return false;
	}

	const ISWTeamInterface* const LocalTeamProvider = Cast<ISWTeamInterface>(LocalPawn);
	const ISWTeamInterface* const CandidateTeamProvider = Cast<ISWTeamInterface>(CandidateActor);
	const ESWTeamId LocalTeamId = LocalTeamProvider ? LocalTeamProvider->GetTeamId() : ESWTeamId::None;
	const ESWTeamId CandidateTeamId = CandidateTeamProvider ? CandidateTeamProvider->GetTeamId() : ESWTeamId::None;
	return (LocalTeamId == ESWTeamId::TeamA || LocalTeamId == ESWTeamId::TeamB)
		&& (CandidateTeamId == ESWTeamId::TeamA || CandidateTeamId == ESWTeamId::TeamB)
		&& CandidateTeamId != LocalTeamId
		&& !ISWCombatInterface::Execute_IsDead(CandidateActor);
}

AActor* ASWPlayerController::FindCrosshairEnemyHighlightTarget() const
{
	const APawn* const LocalPawn = GetPawn();
	UWorld* const World = GetWorld();
	if (!LocalPawn || !World)
	{
		return nullptr;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	GetPlayerViewPoint(ViewLocation, ViewRotation);

	FCollisionQueryParams QueryParameters(SCENE_QUERY_STAT(SWEnemyHighlightCrosshairTrace), false, LocalPawn);
	QueryParameters.AddIgnoredActor(LocalPawn);
	FHitResult HitResult;
	const FVector TraceEnd = ViewLocation + ViewRotation.Vector() * GetEnemyHighlightTraceDistance();
	if (!World->LineTraceSingleByChannel(HitResult, ViewLocation, TraceEnd, ECC_GameTraceChannel1, QueryParameters))
	{
		return nullptr;
	}

	AActor* const HitActor = HitResult.GetActor();
	return IsHostileHighlightTarget(HitActor) ? HitActor : nullptr;
}

float ASWPlayerController::GetEnemyHighlightTraceDistance() const
{
	if (const ASWCharacter_Player* const LocalCharacter = Cast<ASWCharacter_Player>(GetPawn()))
	{
		if (const ASWWeapon* const CurrentWeapon = LocalCharacter->GetCurrentWeapon())
		{
			return FMath::Max(1.f, CurrentWeapon->GetMaxAimDistance());
		}
	}

	return FMath::Max(1.f, FallbackEnemyHighlightTraceDistance);
}

bool ASWPlayerController::IsWithinEnemyHighlightDistance(const AActor& CandidateActor, const FVector& ViewLocation) const
{
	const float MaxDistance = FMath::Max(1.f, EnemyHighlightMaxDisplayDistance);
	return FVector::DistSquared(ViewLocation, CandidateActor.GetActorLocation()) <= FMath::Square(MaxDistance);
}

void ASWPlayerController::ApplyLocalHighlightToActor(AActor& TargetActor, UMaterialInterface& OverlayMaterial,
	TSet<TWeakObjectPtr<UMeshComponent>>& OutTouchedComponents)
{
	TInlineComponentArray<UMeshComponent*> MeshComponents;
	TargetActor.GetComponents(MeshComponents);

	for (UMeshComponent* const MeshComponent : MeshComponents)
	{
		if (!IsValid(MeshComponent))
		{
			continue;
		}

		const TWeakObjectPtr<UMeshComponent> ComponentKey(MeshComponent);
		OutTouchedComponents.Add(ComponentKey);
		if (!OriginalEnemyHighlightComponentStates.Contains(ComponentKey))
		{
			FSWLocalEnemyHighlightComponentState& OriginalState = OriginalEnemyHighlightComponentStates.Add(ComponentKey);
			OriginalState.OverlayMaterial = MeshComponent->OverlayMaterial;
			OriginalState.OverlayMaterialMaxDrawDistance = MeshComponent->OverlayMaterialMaxDrawDistance;
		}

		if (MeshComponent->OverlayMaterial != &OverlayMaterial)
		{
			MeshComponent->SetOverlayMaterial(&OverlayMaterial);
		}

		if (!FMath::IsNearlyEqual(MeshComponent->OverlayMaterialMaxDrawDistance, EnemyHighlightMaxDisplayDistance))
		{
			MeshComponent->SetOverlayMaterialMaxDrawDistance(EnemyHighlightMaxDisplayDistance);
		}
	}
}
