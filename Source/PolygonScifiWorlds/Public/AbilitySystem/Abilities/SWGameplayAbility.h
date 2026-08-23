// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "SWGameplayAbility.generated.h"

/**
 * ScifiWorlds 所有 Gameplay Ability 的 C++ 基础契约类型。
 *
 * 仅保留所有 Ability 共享的生命周期规则、死亡门槛、Avatar 操作与 AttributeSet 底层读取入口。
 * 主动技能的等级、消耗、冷却、充能和 UI 数据属于 USWActiveGameplayAbility；系统输入 Ability
 * 不应看到或配置这些字段。
 */
UCLASS()
class POLYGONSCIFIWORLDS_API USWGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	/**
	 * 为 true 时，只有已复制到本端的比赛状态为 InProgress 才允许激活。
	 * 默认关闭，避免移动、瞄准、换弹等非战斗系统 Ability 被错误限制；伤害与战斗 Ability 应在其 C++ 父类中显式开启。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SW|Ability|Match")
	bool bRequiresMatchInProgress = false;

	/** 所有项目 Ability 的统一死亡门槛；服务器和预测端均拒绝已死亡 Avatar 的新激活。 */
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr,
		OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

protected:
	/** 读取拥有者上某个属性的当前值；无 ASC 或属性缺失时返回 DefaultValue。 */
	float GetOwnerAttributeValue(const FGameplayAttribute& Attribute, float DefaultValue = 0.f) const;
};
