// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "AbilitySystem/Effects/SWHealGameplayEffect.h"

#include "AbilitySystem/SWAttributeSet.h"
#include "GameplayTags/SWGameplayTags.h"

USWHealGameplayEffect::USWHealGameplayEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo& HealingModifier = Modifiers.AddDefaulted_GetRef();
	HealingModifier.Attribute = USWAttributeSet::GetHealthAttribute();
	HealingModifier.ModifierOp = EGameplayModOp::Additive;

	FSetByCallerFloat HealingMagnitude;
	HealingMagnitude.DataTag = SWGameplayTags::SetByCaller_Healing;
	HealingModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(HealingMagnitude);
}
