// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/SWGameplayEffect.h"
#include "SWDamageGameplayEffect.generated.h"

class USWDamageCalculationConfig;

/**
 * 通用伤害 Gameplay Effect 的数据契约。
 * 伤害生产者通过 FSWDamageApplicationParams 写入原始伤害与伤害类型；该 GE 只绑定全局结算配置与 ExecCalc。
 */
UCLASS(BlueprintType)
class POLYGONSCIFIWORLDS_API USWDamageGameplayEffect : public USWGameplayEffect
{
	GENERATED_BODY()

public:
	USWDamageGameplayEffect();

	const USWDamageCalculationConfig* GetDamageCalculationConfig() const { return DamageCalculationConfig; }

private:
	/** 每个伤害 GE 必须显式引用，用于避免伤害公式依赖 GameMode 或全局查找。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USWDamageCalculationConfig> DamageCalculationConfig;

};
