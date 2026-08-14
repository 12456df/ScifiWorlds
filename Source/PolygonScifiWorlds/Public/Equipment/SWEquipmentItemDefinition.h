// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SWEquipmentItemDefinition.generated.h"

class UGameplayEffect;
class UTexture2D;

/**
 * 装备内容的静态定义。
 *
 * 运行时装备栏只保存本资产的 Primary Asset Id；装备栏状态和实际生效的 GE
 * 分别由后续 M09 装备栏与装备效果应用器持有，避免把可重建的运行时状态序列化进内容资产。
 */
UCLASS(BlueprintType)
class POLYGONSCIFIWORLDS_API USWEquipmentItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** 固定的 Primary Asset 类型；资产名组成可复制、可持久化的物品标识。 */
	static const FPrimaryAssetType EquipmentItemAssetType;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/** 供固定商店格等内容蓝图读取稳定装备标识；仅查询，不暴露任何运行时写入能力。 */
	UFUNCTION(BlueprintPure, Category = "Equipment")
	FPrimaryAssetId GetEquipmentItemId() const { return GetPrimaryAssetId(); }

	/** 装备在商店和装备栏中显示的本地化名称，不参与运行时身份判定。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment|Display")
	FText DisplayName;

	/** 装备在商店和详情面板中显示的本地化描述。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment|Display", meta = (MultiLine = "true"))
	FText Description;

	/** 仅客户端 UI 使用；Dedicated Server 不应为交易或属性效果加载此资源。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment|Display", meta = (AssetBundles = "UI"))
	TSoftObjectPtr<UTexture2D> Icon;

	/** M09 购买时扣除的金币数量。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment|Economy", meta = (ClampMin = "0"))
	int32 PurchasePrice = 0;

	/** 同一玩家可同时持有该装备的最大数量；当前装备栏容量为六。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment|Inventory", meta = (ClampMin = "1", ClampMax = "6"))
	int32 MaxOwnedCount = 1;

	/** 装备进入槽位时由服务器应用的 Infinite Gameplay Effect 列表。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment|Effects")
	TArray<TSubclassOf<UGameplayEffect>> EquippedEffectClasses;
};
