// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "SWGameplayEffect.generated.h"

class UAbilitySystemComponent;

/**
 * ScifiWorlds 所有 Gameplay Effect 的 C++ 基础契约类型。
 *
 * M03 仅建立该基类以统一后续效果内容的派生入口，不在此写入任何平衡数值或具体修正；
 * 具体效果（伤害、消耗、冷却、Buff/Debuff 等）由后续内容模块以数据资产或蓝图子类落地。
 */
UCLASS()
class POLYGONSCIFIWORLDS_API USWGameplayEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	/**
	 * 判断 Effect Spec 的源与目标是否属于同一有效队伍。
	 * 依次从 ASC Owner 与 Avatar 上的 Team 接口读取 ESWTeamId；None、缺失或未实现队伍真值的对象均不视为友军。
	 */
	static bool AreSourceAndTargetOnSameTeam(const FGameplayEffectSpec& EffectSpec, const UAbilitySystemComponent* TargetAbilitySystemComponent);

	/** 供不依赖 Effect Spec 的通用 Buff、Debuff 与 Damage 逻辑复用的同队查询。 */
	static bool AreAbilitySystemComponentsOnSameTeam(const UAbilitySystemComponent* SourceAbilitySystemComponent, const UAbilitySystemComponent* TargetAbilitySystemComponent);
};
