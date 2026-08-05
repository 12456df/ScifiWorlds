// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/SWGameplayEffect.h"
#include "GameplayTagContainer.h"
#include "ScalableFloat.h"
#include "SWDamageGameplayEffect.generated.h"

class USWDamageCalculationConfig;

/**
 * 单次伤害 Spec 的单一伤害通道配置。
 * 混合伤害应拆分为多个独立 Spec，避免不同防御、暴击和命中结果混入一次结算。
 */
USTRUCT(BlueprintType)
struct FSWDamageChannelSpec
{
	GENERATED_BODY()

	/** 仅允许 Damage.Type.Physical、Damage.Type.Magical 或 Damage.Type.True。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage")
	FGameplayTag DamageType;

	/** 按 Gameplay Effect Level 取值的基础伤害。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage", meta = (ClampMin = "0.0"))
	FScalableFloat BaseMagnitude;

	/** 攻击力参与伤害计算的系数。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage")
	float AttackPowerCoefficient = 0.f;

	/** 法强参与伤害计算的系数。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage")
	float SpellPowerCoefficient = 0.f;

	/** 此伤害通道是否参与服务器唯一的暴击判定。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage")
	bool bCanCritical = false;
};

/**
 * 伤害 Gameplay Effect 的数据契约。
 * 蓝图子类只配置单一伤害通道与全局伤害公式配置；所有属性捕获与最终扣血由 ExecCalc 完成。
 */
UCLASS(BlueprintType)
class POLYGONSCIFIWORLDS_API USWDamageGameplayEffect : public USWGameplayEffect
{
	GENERATED_BODY()

public:
	USWDamageGameplayEffect();

	const FSWDamageChannelSpec& GetDamageChannel() const { return DamageChannel; }
	const USWDamageCalculationConfig* GetDamageCalculationConfig() const { return DamageCalculationConfig; }

private:
	/** 每个伤害 GE 必须显式引用，用于避免伤害公式依赖 GameMode 或全局查找。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USWDamageCalculationConfig> DamageCalculationConfig;

	/** 每个 Damage Effect 仅配置一个物理、魔法或真实伤害通道。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage", meta = (AllowPrivateAccess = "true"))
	FSWDamageChannelSpec DamageChannel;
};
