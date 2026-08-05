// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SWDamageCalculationConfig.generated.h"

/** 伤害结算与移动倍率的全局数据契约；具体伤害公式由后续服务器权威 ExecCalc 消费。 */
UCLASS(BlueprintType)
class POLYGONSCIFIWORLDS_API USWDamageCalculationConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	/** 有效物理护甲达到此值时，物理伤害减免恰为 50%。必须大于零。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage|Mitigation", meta = (ClampMin = "0.001"))
	float PhysicalArmorMitigationHalfPoint = 100.f;

	/** 有效魔法护甲达到此值时，魔法伤害减免恰为 50%。必须大于零。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage|Mitigation", meta = (ClampMin = "0.001"))
	float MagicalArmorMitigationHalfPoint = 100.f;

	/** 暴击率的最终安全上限。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage|Critical", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MaxCriticalChance = 1.f;

	/** 移动倍率允许的最小值，防止非法 GE 产生负速度。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0.0"))
	float MinimumMovementSpeedMultiplier = 0.01f;

	/** 移动倍率允许的最大值；实际平衡值由 Data Asset 决定。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0.01"))
	float MaximumMovementSpeedMultiplier = 3.f;
};
