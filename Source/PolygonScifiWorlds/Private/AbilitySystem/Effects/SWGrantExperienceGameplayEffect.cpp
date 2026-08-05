// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "AbilitySystem/Effects/SWGrantExperienceGameplayEffect.h"

#include "AbilitySystem/SWAttributeSet.h"
#include "GameplayTags/SWGameplayTags.h"

USWGrantExperienceGameplayEffect::USWGrantExperienceGameplayEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo& ExperienceModifier = Modifiers.AddDefaulted_GetRef();
	ExperienceModifier.Attribute = USWAttributeSet::GetIncomingXPAttribute();
	ExperienceModifier.ModifierOp = EGameplayModOp::Additive;

	FSetByCallerFloat ExperienceMagnitude;
	ExperienceMagnitude.DataTag = SWGameplayTags::SetByCaller_Experience;
	ExperienceModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(ExperienceMagnitude);
}
