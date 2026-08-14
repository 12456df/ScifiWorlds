// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManagerTypes.h"
#include "SWEquipmentTypes.generated.h"

/**
 * 玩家装备栏的单个持久槽位。
 *
 * 槽位仅保存 Primary Asset Id，而不保存 Data Asset 或 GE 的运行时指针。
 * 因此它可安全复制，并能在 ASC 的派生状态丢失后重建装备效果。
 */
USTRUCT(BlueprintType)
struct FSWEquipmentSlot
{
	GENERATED_BODY()

	/** 空 Id 表示该槽位未装备任何物品。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
	FPrimaryAssetId ItemDefinitionId;

	/** 此槽位购入时的实际成交价；出售退款只依据该值，避免目录改价产生套利。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Economy")
	int32 PurchasePricePaid = 0;

	bool IsEmpty() const
	{
		return !ItemDefinitionId.IsValid();
	}

	void Clear()
	{
		ItemDefinitionId = FPrimaryAssetId();
		PurchasePricePaid = 0;
	}
};
