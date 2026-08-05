// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "AbilitySystem/SWGameplayEffect.h"

#include "AbilitySystemComponent.h"
#include "Interaction/SWTeamInterface.h"

namespace
{
	ESWTeamId GetTeamIdFromAbilitySystemComponent(const UAbilitySystemComponent* AbilitySystemComponent)
	{
		const AActor* const OwnerActor = AbilitySystemComponent ? AbilitySystemComponent->GetOwnerActor() : nullptr;
		if (const ISWTeamInterface* const TeamOwner = OwnerActor ? Cast<ISWTeamInterface>(OwnerActor) : nullptr)
		{
			return TeamOwner->GetTeamId();
		}

		const AActor* const AvatarActor = AbilitySystemComponent ? AbilitySystemComponent->GetAvatarActor() : nullptr;
		if (const ISWTeamInterface* const TeamAvatar = AvatarActor ? Cast<ISWTeamInterface>(AvatarActor) : nullptr)
		{
			return TeamAvatar->GetTeamId();
		}

		return ESWTeamId::None;
	}
}

bool USWGameplayEffect::AreSourceAndTargetOnSameTeam(const FGameplayEffectSpec& EffectSpec, const UAbilitySystemComponent* TargetAbilitySystemComponent)
{
	return AreAbilitySystemComponentsOnSameTeam(EffectSpec.GetContext().GetInstigatorAbilitySystemComponent(), TargetAbilitySystemComponent);
}

bool USWGameplayEffect::AreAbilitySystemComponentsOnSameTeam(const UAbilitySystemComponent* SourceAbilitySystemComponent, const UAbilitySystemComponent* TargetAbilitySystemComponent)
{
	const ESWTeamId SourceTeamId = GetTeamIdFromAbilitySystemComponent(SourceAbilitySystemComponent);
	const ESWTeamId TargetTeamId = GetTeamIdFromAbilitySystemComponent(TargetAbilitySystemComponent);
	return SourceTeamId != ESWTeamId::None && SourceTeamId == TargetTeamId;
}
