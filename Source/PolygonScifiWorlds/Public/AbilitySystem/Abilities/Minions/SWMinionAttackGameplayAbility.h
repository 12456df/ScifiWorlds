// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/SWGameplayAbility.h"
#include "AbilitySystem/SWAbilityTypes.h"
#include "SWMinionAttackGameplayAbility.generated.h"

class USWDamageGameplayEffect;

/**
 * 小兵普通攻击的可复用 GAS 基类。
 * C++ 固定服务器伤害契约；蓝图只编排前摇、Montage、命中特效及何时调用伤害提交函数。
 */
UCLASS(Abstract, Blueprintable)
class POLYGONSCIFIWORLDS_API USWMinionAttackGameplayAbility : public USWGameplayAbility
{
	GENERATED_BODY()

public:
	USWMinionAttackGameplayAbility();

	/** 攻击结束时唤醒该 Entity 的 StateTree，使其回到低频索敌/下一次攻击循环。 */
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/** 仅服务器：在蓝图定义的命中时点重新验证目标并经统一 Damage GE 提交一次伤害。 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Minion|Attack")
	bool ApplyMinionAttackDamageAuthority(AActor* TargetActor);

protected:
	/** 小兵基础物理伤害；最终伤害还会叠加其 AttributeSet 中的攻击力。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Minion|Attack", meta = (ClampMin = "0.0"))
	FScalableFloat BaseDamage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Minion|Attack", meta = (ClampMin = "0.0"))
	float AttackPowerCoefficient = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Minion|Attack")
	TSubclassOf<USWDamageGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Minion|Attack")
	FGameplayTag DamageType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Minion|Attack")
	bool bCanCritical = true;
};
