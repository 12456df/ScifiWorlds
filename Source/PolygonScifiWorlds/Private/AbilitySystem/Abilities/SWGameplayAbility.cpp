// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "AbilitySystem/Abilities/SWGameplayAbility.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/SWAttributeSet.h"
#include "GameFramework/Character.h"
#include "GameplayTags/SWGameplayTags.h"
#include "Movement/SWCharacterMovementComponent.h"

bool USWGameplayAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	// 不依赖每个具体 Ability 的蓝图都重复配置 ActivationBlockedTags，死亡是所有项目 Ability 的统一门槛。
	return !ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid()
		|| !ActorInfo->AbilitySystemComponent->HasMatchingGameplayTag(SWGameplayTags::State_Dead);
}

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
	const float BonusPercent = GetOwnerAttributeValue(USWAttributeSet::GetAbilityRangeBonusPercentAttribute());
	return BaseRange * (1.f + BonusPercent);
}

float USWGameplayAbility::GetEffectiveDuration(float BaseDuration) const
{
	const float BonusPercent = GetOwnerAttributeValue(USWAttributeSet::GetAbilityDurationBonusPercentAttribute());
	return BaseDuration * (1.f + BonusPercent);
}

float USWGameplayAbility::GetEffectiveCooldown(float BaseCooldown) const
{
	const float ReductionPercent = GetOwnerAttributeValue(USWAttributeSet::GetCooldownReductionPercentAttribute());
	// 冷却缩减可能被配置得很高，钳制下限避免出现负冷却。
	return FMath::Max(0.f, BaseCooldown * (1.f - ReductionPercent));
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
