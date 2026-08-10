// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Effects/SWDamageGameplayEffect.h"
#include "SWPortalSphereDamageGameplayEffect.generated.h"

/**
 * PortalSphere 的周期伤害 GE 基类。
 * 伤害原始值由命中时传入的伤害包提供，持续时间由 SetByCaller.Ability.Duration 提供；蓝图子类仅配置周期与表现。
 */
UCLASS(Abstract, Blueprintable)
class POLYGONSCIFIWORLDS_API USWPortalSphereDamageGameplayEffect : public USWDamageGameplayEffect
{
	GENERATED_BODY()

public:
	USWPortalSphereDamageGameplayEffect();
};
