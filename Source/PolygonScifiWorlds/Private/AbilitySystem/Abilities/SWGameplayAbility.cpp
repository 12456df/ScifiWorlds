// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "AbilitySystem/Abilities/SWGameplayAbility.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/SWAttributeSet.h"
#include "GameFramework/Character.h"
#include "Movement/SWCharacterMovementComponent.h"

void USWGameplayAbility::SetAvatarSprintRequested(const bool bRequested) const
{
	if (ACharacter* AvatarCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		if (USWCharacterMovementComponent* MovementComponent = Cast<USWCharacterMovementComponent>(AvatarCharacter->GetCharacterMovement()))
		{
			MovementComponent->SetSprintRequested(bRequested);
		}
	}
}

float USWGameplayAbility::GetEffectiveRange(float BaseRange) const
{
	const float Multiplier = GetOwnerAttributeValue(USWAttributeSet::GetAbilityRangeMultiplierAttribute());
	return BaseRange * (1.f + Multiplier);
}

float USWGameplayAbility::GetEffectiveDuration(float BaseDuration) const
{
	const float Multiplier = GetOwnerAttributeValue(USWAttributeSet::GetAbilityDurationMultiplierAttribute());
	return BaseDuration * (1.f + Multiplier);
}

float USWGameplayAbility::GetEffectiveCooldown(float BaseCooldown) const
{
	const float Multiplier = GetOwnerAttributeValue(USWAttributeSet::GetCooldownReductionMultiplierAttribute());
	// 冷却缩减可能被配置得很高，钳制下限避免出现负冷却。
	return FMath::Max(0.f, BaseCooldown * (1.f - Multiplier));
}

float USWGameplayAbility::GetOwnerAttributeValue(const FGameplayAttribute& Attribute, float DefaultValue) const
{
	if (const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		bool bFound = false;
		const float Value = ASC->GetGameplayAttributeValue(Attribute, bFound);
		if (bFound)
		{
			return Value;
		}
	}
	return DefaultValue;
}
