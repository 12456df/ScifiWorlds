// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Effects/SWDamageGameplayEffect.h"
#include "AbilitySystem/SWGameplayEffect.h"
#include "SWCommonBuffGameplayEffects.generated.h"

/** 所有通用持续 Buff/Debuff 的基础：时长由 SetByCaller.Ability.Duration 决定。 */
UCLASS(Abstract, Blueprintable)
class POLYGONSCIFIWORLDS_API USWDurationBuffGameplayEffect : public USWGameplayEffect
{
	GENERATED_BODY()

public:
	USWDurationBuffGameplayEffect();
};

/** 加速：蓝图子类只配置时长、Cue 和 SetByCaller.Buff.MovementSpeedDelta。 */
UCLASS(Abstract, Blueprintable)
class POLYGONSCIFIWORLDS_API USWSpeedBuffGameplayEffect : public USWDurationBuffGameplayEffect
{
	GENERATED_BODY()

public:
	USWSpeedBuffGameplayEffect();
};

/** 治疗 Buff：每秒回复量由 SetByCaller.Buff.HealthPerSecond 决定。 */
UCLASS(Abstract, Blueprintable)
class POLYGONSCIFIWORLDS_API USWHealBuffGameplayEffect : public USWDurationBuffGameplayEffect
{
	GENERATED_BODY()

public:
	USWHealBuffGameplayEffect();
};

/**
 * 首版眩晕 Debuff：保留 State.Debuff.Stunned 作为表现/状态协议，但实际效果是减速。
 * 由于当前没有可读的眩晕动作，不阻断移动或 Gameplay Ability；减速量由
 * SetByCaller.Buff.MovementSpeedDelta 提供，必须为负值。
 */
UCLASS(Abstract, Blueprintable)
class POLYGONSCIFIWORLDS_API USWStunDebuffGameplayEffect : public USWDurationBuffGameplayEffect
{
	GENERATED_BODY()

public:
	USWStunDebuffGameplayEffect();
};

/** 中毒：周期伤害仍进入统一 Damage ExecCalc；每跳原始伤害由 SetByCaller.Damage.Raw 决定。 */
UCLASS(Abstract, Blueprintable)
class POLYGONSCIFIWORLDS_API USWPoisonDebuffGameplayEffect : public USWDamageGameplayEffect
{
	GENERATED_BODY()

public:
	USWPoisonDebuffGameplayEffect();
};
