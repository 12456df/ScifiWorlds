// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManagerTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SWEquipmentFunctionLibrary.generated.h"

class USWEquipmentItemDefinition;

/** 装备静态定义的只读蓝图查询入口。运行时装备槽位仅保存 Primary Asset Id。 */
UCLASS()
class POLYGONSCIFIWORLDS_API USWEquipmentFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * 由装备主资源 Id 解析其静态定义。
	 * 仅用于 UI 和内容读取；不得通过返回值修改玩家装备槽位或交易状态。
	 */
	UFUNCTION(BlueprintPure, Category = "Equipment|Definition")
	static USWEquipmentItemDefinition* GetEquipmentItemDefinitionById(const FPrimaryAssetId& ItemDefinitionId);
};
