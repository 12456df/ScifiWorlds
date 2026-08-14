// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "Shop/SWShopCatalogData.h"

#include "Equipment/SWEquipmentItemDefinition.h"

const USWEquipmentItemDefinition* USWShopCatalogData::FindItemDefinition(const FPrimaryAssetId& ItemId) const
{
	if (!ItemId.IsValid())
	{
		return nullptr;
	}

	for (const USWEquipmentItemDefinition* const ItemDefinition : ItemDefinitions)
	{
		if (ItemDefinition && ItemDefinition->GetPrimaryAssetId() == ItemId)
		{
			return ItemDefinition;
		}
	}

	return nullptr;
}

bool USWShopCatalogData::ContainsItemDefinition(const USWEquipmentItemDefinition* const ItemDefinition) const
{
	return ItemDefinition && FindItemDefinition(ItemDefinition->GetPrimaryAssetId()) == ItemDefinition;
}
