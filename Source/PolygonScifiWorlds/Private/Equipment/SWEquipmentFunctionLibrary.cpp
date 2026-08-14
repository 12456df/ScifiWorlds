// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "Equipment/SWEquipmentFunctionLibrary.h"

#include "Engine/AssetManager.h"
#include "Equipment/SWEquipmentItemDefinition.h"

USWEquipmentItemDefinition* USWEquipmentFunctionLibrary::GetEquipmentItemDefinitionById(const FPrimaryAssetId& ItemDefinitionId)
{
	if (!ItemDefinitionId.IsValid() || ItemDefinitionId.PrimaryAssetType != USWEquipmentItemDefinition::EquipmentItemAssetType)
	{
		return nullptr;
	}

	UAssetManager& AssetManager = UAssetManager::Get();
	if (USWEquipmentItemDefinition* const LoadedDefinition = Cast<USWEquipmentItemDefinition>(AssetManager.GetPrimaryAssetObject(ItemDefinitionId)))
	{
		return LoadedDefinition;
	}

	// 装备定义是 UI 初始化所需的小型数据资产；未预加载时才同步解析该单个定义。
	return Cast<USWEquipmentItemDefinition>(AssetManager.GetPrimaryAssetPath(ItemDefinitionId).TryLoad());
}
