// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "UI/WidgetController/Overlay/SWEquipmentOverlayWidgetController.h"

#include "Engine/AssetManager.h"
#include "Equipment/SWEquipmentItemDefinition.h"
#include "Equipment/SWEquipmentTypes.h"
#include "Player/SWPlayerState.h"

namespace
{
	const USWEquipmentItemDefinition* ResolveEquipmentDefinitionForUI(const FPrimaryAssetId& ItemDefinitionId)
	{
		if (!ItemDefinitionId.IsValid())
		{
			return nullptr;
		}

		UAssetManager& AssetManager = UAssetManager::Get();
		if (const USWEquipmentItemDefinition* const LoadedDefinition = Cast<USWEquipmentItemDefinition>(AssetManager.GetPrimaryAssetObject(ItemDefinitionId)))
		{
			return LoadedDefinition;
		}

		const FSoftObjectPath AssetPath = AssetManager.GetPrimaryAssetPath(ItemDefinitionId);
		return Cast<USWEquipmentItemDefinition>(AssetPath.TryLoad());
	}
}

void USWEquipmentOverlayWidgetController::BroadcastInitialValues()
{
	TArray<FSWEquipmentSlotSnapshot> EquipmentSlots;
	EquipmentSlots.Reserve(ASWPlayerState::EquipmentSlotCount);

	for (int32 SlotIndex = 0; SlotIndex < ASWPlayerState::EquipmentSlotCount; ++SlotIndex)
	{
		EquipmentSlots.Add(BuildEquipmentSlotSnapshot(SlotIndex));
	}

	OnEquipmentBarInitialized.Broadcast(EquipmentSlots);
}

void USWEquipmentOverlayWidgetController::BindCallbacksToDependencies()
{
	UnbindCallbacks();
	if (!PlayerState)
	{
		return;
	}

	EquipmentSlotsChangedHandle = PlayerState->OnEquipmentSlotsChanged.AddUObject(this, &ThisClass::HandleEquipmentSlotsChanged);
	bCallbacksBound = true;
}

void USWEquipmentOverlayWidgetController::BeginDestroy()
{
	UnbindCallbacks();
	Super::BeginDestroy();
}

void USWEquipmentOverlayWidgetController::RefreshEquipmentBar()
{
	BroadcastInitialValues();
}

void USWEquipmentOverlayWidgetController::UnbindCallbacks()
{
	if (bCallbacksBound && PlayerState && EquipmentSlotsChangedHandle.IsValid())
	{
		PlayerState->OnEquipmentSlotsChanged.Remove(EquipmentSlotsChangedHandle);
	}

	EquipmentSlotsChangedHandle.Reset();
	bCallbacksBound = false;
}

void USWEquipmentOverlayWidgetController::HandleEquipmentSlotsChanged()
{
	// 当前 PlayerState 的 RepNotify 只表示六槽快照已更新，未携带差异索引。
	// 六槽规模固定且低频，因此在这里逐槽推送既简单又能为后续交易直接复用。
	for (int32 SlotIndex = 0; SlotIndex < ASWPlayerState::EquipmentSlotCount; ++SlotIndex)
	{
		OnEquipmentSlotChanged.Broadcast(BuildEquipmentSlotSnapshot(SlotIndex));
	}
}

FSWEquipmentSlotSnapshot USWEquipmentOverlayWidgetController::BuildEquipmentSlotSnapshot(const int32 SlotIndex) const
{
	FSWEquipmentSlotSnapshot Snapshot;
	Snapshot.SlotIndex = SlotIndex;

	if (!PlayerState || SlotIndex < 0 || SlotIndex >= PlayerState->GetEquipmentSlots().Num())
	{
		return Snapshot;
	}

	const FSWEquipmentSlot& EquipmentSlot = PlayerState->GetEquipmentSlots()[SlotIndex];
	Snapshot.ItemDefinitionId = EquipmentSlot.ItemDefinitionId;
	Snapshot.bIsOccupied = !EquipmentSlot.IsEmpty();
	if (!Snapshot.bIsOccupied)
	{
		return Snapshot;
	}

	const USWEquipmentItemDefinition* const ItemDefinition = ResolveEquipmentDefinitionForUI(Snapshot.ItemDefinitionId);
	if (!ItemDefinition)
	{
		return Snapshot;
	}

	Snapshot.DisplayName = ItemDefinition->DisplayName;
	Snapshot.Description = ItemDefinition->Description;
	Snapshot.Icon = ItemDefinition->Icon;
	Snapshot.PurchasePrice = ItemDefinition->PurchasePrice;
	return Snapshot;
}
