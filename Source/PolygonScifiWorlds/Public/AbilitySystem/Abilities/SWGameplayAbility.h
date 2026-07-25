// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "SWGameplayAbility.generated.h"

/**
 * ScifiWorlds 所有 Gameplay Ability 的 C++ 基础契约类型。
 *
 * M03 为技能提供统一的"装备驱动修正"读取入口（FR-03）：技能不把范围、持续时间和冷却写死在
 * 自身资产中，而是从拥有者的 USWAttributeSet 只读查询修正值，并按统一公式换算有效值。
 *
 * 这些查询是一次性快照：能力应在提交冷却、创建效果 Spec 或计算目标数据时读取。已生效的持续
 * 效果和已启动的冷却不会因属性变化回溯重算，除非某能力明确设计为动态更新。
 */
UCLASS()
class POLYGONSCIFIWORLDS_API USWGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	/** 有效范围 = 基础范围 × (1 + AbilityRangeMultiplier)。无 ASC 时按无修正处理。 */
	UFUNCTION(BlueprintPure, Category = "SW|Ability")
	float GetEffectiveRange(float BaseRange) const;

	/** 有效持续时间 = 基础持续时间 × (1 + AbilityDurationMultiplier)。无 ASC 时按无修正处理。 */
	UFUNCTION(BlueprintPure, Category = "SW|Ability")
	float GetEffectiveDuration(float BaseDuration) const;

	/** 有效冷却 = 基础冷却 × (1 - CooldownReductionMultiplier)。结果钳制为非负。 */
	UFUNCTION(BlueprintPure, Category = "SW|Ability")
	float GetEffectiveCooldown(float BaseCooldown) const;

protected:
	/** 读取拥有者上某个属性的当前值；无 ASC 或属性缺失时返回 DefaultValue。 */
	float GetOwnerAttributeValue(const FGameplayAttribute& Attribute, float DefaultValue = 0.f) const;
};
