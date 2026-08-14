// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManagerTypes.h"
#include "Engine/DataAsset.h"
#include "SWShopCatalogData.generated.h"

class USWEquipmentItemDefinition;

/**
 * 本局允许交易的商品白名单。
 * 页面分类、价格层和具体摆位属于 UMG 内容配置；它们不进入服务器交易规则。
 */
UCLASS(BlueprintType)
class POLYGONSCIFIWORLDS_API USWShopCatalogData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shop")
	TArray<TObjectPtr<USWEquipmentItemDefinition>> ItemDefinitions;

	const USWEquipmentItemDefinition* FindItemDefinition(const FPrimaryAssetId& ItemId) const;
	bool ContainsItemDefinition(const USWEquipmentItemDefinition* ItemDefinition) const;
};
