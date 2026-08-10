// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "AbilitySystem/Abilities/SWGameplayAbility.h"

#include "AbilitySystemComponent.h"
#include "GameplayTags/SWGameplayTags.h"

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
