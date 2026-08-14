// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "Player/SWPlayerController.h"

#include "AbilitySystem/SWAbilitySystemComponent.h"
#include "Character/SWCharacter_Base.h"
#include "Equipment/SWEquipmentItemDefinition.h"
#include "Equipment/SWEquipmentTypes.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Input/SWInputConfig.h"
#include "Player/SWPlayerState.h"
#include "GameState/SWGameState.h"
#include "Shop/SWShopCatalogData.h"
#include "UI/HUD/SWHUD.h"

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
