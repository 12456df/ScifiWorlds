// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ScalableFloat.h"
#include "SWEconomyData.generated.h"

/**
 * 单局经济的静态配置。GameMode 在服务器设置，GameState 复制只读引用；
 * PlayerState 只保存玩家实际金币，不保存第二份经济规则。
 */
UCLASS(BlueprintType)
class POLYGONSCIFIWORLDS_API USWEconomyData : public UDataAsset
{
	GENERATED_BODY()

public:
	/** 玩家首次加入本局时授予的金币；死亡和重生不会再次触发。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Economy", meta = (ClampMin = "0"))
	int32 StartingGold = 0;

	/** 每秒被动金币，按玩家当前等级从 ScalableFloat/CurveTable 求值；允许配置为 0。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Economy")
	FScalableFloat PassiveGoldPerSecondByLevel;

	/** 单局金币上限，所有奖励均在 PlayerState 中饱和处理。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Economy", meta = (ClampMin = "1"))
	int32 MaxGold = MAX_int32;

	/** 出售装备时返还成交价的比例。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Economy", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SellRefundRate = 0.70f;

	/** 返回数据驱动的每秒收入；负数配置按 0 处理，整数结算由 PlayerState 累积小数后完成。 */
	float GetPassiveGoldPerSecondAtLevel(int32 PlayerLevel) const
	{
		return FMath::Max(0.f, PassiveGoldPerSecondByLevel.GetValueAtLevel(FMath::Max(1, PlayerLevel)));
	}
};
