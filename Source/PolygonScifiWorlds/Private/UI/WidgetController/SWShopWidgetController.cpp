// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "UI/WidgetController/SWShopWidgetController.h"

#include "Equipment/SWEquipmentItemDefinition.h"
#include "Equipment/SWEquipmentTypes.h"
#include "GameState/SWGameState.h"
#include "Player/SWPlayerState.h"
#include "Shop/SWShopCatalogData.h"

void USWShopWidgetController::BroadcastInitialValues()
{
	HandleShopStateChanged();
}

void USWShopWidgetController::BindCallbacksToDependencies()
{
	UnbindCallbacks();
	if (!PlayerState)
	{
		return;
	}

	GoldChangedHandle = PlayerState->OnGoldChanged.AddUObject(this, &ThisClass::HandleGoldChanged);
	EquipmentSlotsChangedHandle = PlayerState->OnEquipmentSlotsChanged.AddUObject(this, &ThisClass::HandleShopStateChanged);
	TradeAccessChangedHandle = PlayerState->OnShopTradeAccessChanged.AddUObject(this, &ThisClass::HandleTradeAccessChanged);
	bCallbacksBound = true;
}

void USWShopWidgetController::BeginDestroy()
{
	UnbindCallbacks();
	Super::BeginDestroy();
}

void USWShopWidgetController::RefreshShopState()
{
	HandleShopStateChanged();
}

FSWShopItemSnapshot USWShopWidgetController::GetItemSnapshot(const FPrimaryAssetId& ItemDefinitionId) const
{
	const USWShopCatalogData* const Catalog = GameState ? GameState->GetShopCatalogData() : nullptr;
	return Catalog ? BuildItemSnapshot(Catalog->FindItemDefinition(ItemDefinitionId)) : FSWShopItemSnapshot();
}

void USWShopWidgetController::UnbindCallbacks()
{
	if (bCallbacksBound && PlayerState)
	{
		PlayerState->OnGoldChanged.Remove(GoldChangedHandle);
		PlayerState->OnEquipmentSlotsChanged.Remove(EquipmentSlotsChangedHandle);
		PlayerState->OnShopTradeAccessChanged.Remove(TradeAccessChangedHandle);
	}

	GoldChangedHandle.Reset();
	EquipmentSlotsChangedHandle.Reset();
	TradeAccessChangedHandle.Reset();
	bCallbacksBound = false;
}

void USWShopWidgetController::HandleGoldChanged(const int32 NewGold)
{
	HandleShopStateChanged();
}

void USWShopWidgetController::HandleTradeAccessChanged(const bool bCanTrade)
{
	HandleShopStateChanged();
}

void USWShopWidgetController::HandleShopStateChanged()
{
	const USWShopCatalogData* const Catalog = GameState ? GameState->GetShopCatalogData() : nullptr;
	if (Catalog)
	{
		for (const USWEquipmentItemDefinition* const ItemDefinition : Catalog->ItemDefinitions)
		{
			if (ItemDefinition)
			{
				OnShopItemStateChanged.Broadcast(BuildItemSnapshot(ItemDefinition));
			}
		}
	}

	OnShopTradeAccessChanged.Broadcast(PlayerState && PlayerState->CanTradeAtShop());
}

FSWShopItemSnapshot USWShopWidgetController::BuildItemSnapshot(const USWEquipmentItemDefinition* const ItemDefinition) const
{
	FSWShopItemSnapshot Snapshot;
	if (!ItemDefinition)
	{
		return Snapshot;
	}

	Snapshot.ItemDefinitionId = ItemDefinition->GetPrimaryAssetId();
	Snapshot.DisplayName = ItemDefinition->DisplayName;
	Snapshot.Description = ItemDefinition->Description;
	Snapshot.Icon = ItemDefinition->Icon;
	Snapshot.PurchasePrice = ItemDefinition->PurchasePrice;
	Snapshot.MaxOwnedCount = ItemDefinition->MaxOwnedCount;
	Snapshot.OwnedCount = PlayerState ? PlayerState->GetOwnedEquipmentCount(Snapshot.ItemDefinitionId) : 0;
	Snapshot.bHasEnoughGold = PlayerState && PlayerState->GetGold() >= Snapshot.PurchasePrice;
	const bool bHasInventorySpace = PlayerState && PlayerState->GetEquipmentSlots().ContainsByPredicate([](const FSWEquipmentSlot& Slot)
	{
		return Slot.IsEmpty();
	});
	Snapshot.bCanPurchase = PlayerState && PlayerState->CanTradeAtShop() && Snapshot.bHasEnoughGold
		&& Snapshot.OwnedCount < Snapshot.MaxOwnedCount && bHasInventorySpace;
	return Snapshot;
}
