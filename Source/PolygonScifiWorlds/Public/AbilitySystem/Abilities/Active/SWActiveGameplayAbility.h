// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/SWGameplayAbility.h"
#include "ScalableFloat.h"
#include "SWActiveGameplayAbility.generated.h"

class UGameplayEffect;
class UAbilitySystemComponent;
class UTexture2D;

/**
 * 所有角色主动技能的蓝图父类。
 *
 * 该类承载角色主动技能共用的身份、升级、消耗、冷却、充能和 UI 数据契约，不承载任何
 * 具体技能的目标、伤害、动画或流程。每个正式技能必须再派生自己的 C++ 类，并由该类创建
 * 对应的 GA 蓝图来编排 Ability Task、Montage 和表现。
 */
UCLASS(Abstract, Blueprintable)
class POLYGONSCIFIWORLDS_API USWActiveGameplayAbility : public USWGameplayAbility
{
	GENERATED_BODY()

public:
	USWActiveGameplayAbility();

	/** 技能唯一身份；升级、UI 与运行时查询使用，不能由输入槽位推导。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SW|Ability|Identity", meta = (Categories = "Ability.Skill"))
	FGameplayTag AbilityIdTag;

	/** 该技能独占的冷却/充能 Effect Tag。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SW|Ability|Identity", meta = (Categories = "Cooldown.Ability"))
	FGameplayTag CooldownTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SW|Ability|Progression", meta = (ClampMin = "1"))
	int32 MaxAbilityLevel = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SW|Ability|Progression")
	bool bUpgradeable = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SW|Ability|UI")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SW|Ability|UI")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SW|Ability|Level")
	FScalableFloat ManaCostByLevel;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SW|Ability|Level")
	FScalableFloat CooldownByLevel;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SW|Ability|Level")
	FScalableFloat MaxChargesByLevel;

	/** 是否允许装备等来源通过 AbilityChargeBonus 修改此技能的最大充能。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SW|Ability|Level")
	bool bUseAbilityChargeBonus = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SW|Ability|Level", meta = (ClampMin = "0.0"))
	float MinimumCooldownSeconds = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SW|Ability|Cooldown")
	TSubclassOf<UGameplayEffect> CooldownEffectClass;

	UFUNCTION(BlueprintPure, Category = "SW|Ability|Identity")
	FGameplayTag GetAbilityIdTag() const { return AbilityIdTag; }

	UFUNCTION(BlueprintPure, Category = "SW|Ability|Identity")
	FGameplayTag GetCooldownTag() const { return CooldownTag; }

	UFUNCTION(BlueprintPure, Category = "SW|Ability|Progression")
	bool IsUpgradeable() const { return bUpgradeable; }

	UFUNCTION(BlueprintPure, Category = "SW|Ability|Progression")
	int32 GetMaxAbilityLevel() const { return FMath::Max(1, MaxAbilityLevel); }

	UFUNCTION(BlueprintPure, Category = "SW|Ability")
	float GetEffectiveRange(float BaseRange) const;

	/** 技能选择性调用；返回作用半径与视觉缩放共用的范围倍率。 */
	UFUNCTION(BlueprintPure, Category = "SW|Ability")
	float GetEffectiveAreaScale() const;

	UFUNCTION(BlueprintPure, Category = "SW|Ability")
	float GetEffectiveArea(float BaseArea) const;

	UFUNCTION(BlueprintPure, Category = "SW|Ability")
	float GetEffectiveDuration(float BaseDuration) const;

	UFUNCTION(BlueprintPure, Category = "SW|Ability")
	float GetEffectiveCooldown(float BaseCooldown) const;

	UFUNCTION(BlueprintPure, Category = "SW|Ability")
	int32 GetEffectiveCharges(float BaseCharges) const;

	/** 返回当前技能的最大充能数；Cooldown GE Stack 只表示已消耗的充能。 */
	UFUNCTION(BlueprintPure, Category = "SW|Ability")
	int32 GetMaxCharges() const;

	/**
	 * 按指定 Spec 等级和 ASC 的当前聚合属性计算最大充能。
	 * UI 读取 Ability CDO 时没有运行中 Ability 实例，因此必须使用此无实例依赖入口。
	 */
	int32 GetMaxChargesForLevel(int32 AbilityLevel, const UAbilitySystemComponent* AbilitySystemComponent) const;

	/** 返回当前可用充能数。没有 Cooldown GE 或冷却为 0 的技能始终视为满充能。 */
	UFUNCTION(BlueprintPure, Category = "SW|Ability")
	int32 GetAvailableCharges() const;

	/** 返回当前技能等级对应的蓝量消耗。 */
	UFUNCTION(BlueprintPure, Category = "SW|Ability")
	float GetEffectiveManaCost() const;

	/** 在提交前验证蓝量与通用 Cost GE 配置。 */
	virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	/** 以 Cooldown GE Stack 取代 GAS 默认的“任意冷却 Tag 即完全阻止”规则。 */
	virtual bool CheckCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	/** 在提交阶段写入蓝量 SetByCaller，并由 GAS 应用 Cost GE。 */
	virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) const override;

	/** 让 GAS 冷却检查读取本项目主动技能配置的冷却 GE。 */
	virtual UGameplayEffect* GetCooldownGameplayEffect() const override;

	/** 在提交时写入已包含冷却缩减的 SetByCaller 秒数，再施加冷却 GE。 */
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) const override;

private:
	int32 GetMaxCharges(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo) const;
	int32 GetSpentChargeCount(const FGameplayAbilityActorInfo* ActorInfo) const;
	bool UsesChargeCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo) const;
};
