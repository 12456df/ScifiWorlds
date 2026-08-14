// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "Equipment/SWEquipmentItemDefinition.h"

const FPrimaryAssetType USWEquipmentItemDefinition::EquipmentItemAssetType(TEXT("SWEquipmentItem"));

FPrimaryAssetId USWEquipmentItemDefinition::GetPrimaryAssetId() const
{
	// 固定类型可避免未来重命名 C++ 类时破坏已复制或保存的装备身份。
	return FPrimaryAssetId(EquipmentItemAssetType, GetFName());
}
