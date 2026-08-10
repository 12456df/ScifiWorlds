// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "AbilitySystem/Effects/SWPortalSphereDamageGameplayEffect.h"

#include "GameplayTags/SWGameplayTags.h"

USWPortalSphereDamageGameplayEffect::USWPortalSphereDamageGameplayEffect()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	FSetByCallerFloat DurationMagnitudeByCaller;
	DurationMagnitudeByCaller.DataTag = SWGameplayTags::SetByCaller_Ability_Duration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(DurationMagnitudeByCaller);

	// 默认每秒结算一次；具体技能蓝图可按内容需要修改该周期。
	Period.Value = 1.f;
	bExecutePeriodicEffectOnApplication = false;
}
