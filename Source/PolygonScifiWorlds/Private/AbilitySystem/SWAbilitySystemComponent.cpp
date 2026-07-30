// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "AbilitySystem/SWAbilitySystemComponent.h"

#include "AbilitySystem/Abilities/SWGameplayAbility.h"

void USWAbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag InputTag)
{
	if (!InputTag.IsValid())
	{
		return;
	}

	for (FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		if (!AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			continue;
		}

		InputPressedSpecHandles.AddUnique(AbilitySpec.Handle);
	}
}

void USWAbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag InputTag)
{
	if (!InputTag.IsValid())
	{
		return;
	}

	for (FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		if (!AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			continue;
		}

		InputReleasedSpecHandles.AddUnique(AbilitySpec.Handle);
	}
}

void USWAbilitySystemComponent::ProcessAbilityInput(const float DeltaTime, const bool bGamePaused)
{
	(void)DeltaTime;

	if (bGamePaused)
	{
		return;
	}

	for (const FGameplayAbilitySpecHandle AbilitySpecHandle : InputPressedSpecHandles)
	{
		FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(AbilitySpecHandle);
		if (!AbilitySpec)
		{
			continue;
		}

		if (AbilitySpec->IsActive())
		{
			AbilitySpecInputPressed(*AbilitySpec);

			const TArray<UGameplayAbility*> AbilityInstances = AbilitySpec->GetAbilityInstances();
			if (!AbilityInstances.IsEmpty())
			{
				InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, AbilitySpecHandle, AbilityInstances.Last()->GetCurrentActivationInfoRef().GetActivationPredictionKey());
			}
		}
		else
		{
			// 只由按下边沿激活，避免半自动 Ability 结束后因持续按住而被每帧重新激活。
			TryActivateAbility(AbilitySpecHandle);
		}
	}

	for (const FGameplayAbilitySpecHandle AbilitySpecHandle : InputReleasedSpecHandles)
	{
		FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(AbilitySpecHandle);
		if (AbilitySpec && AbilitySpec->IsActive())
		{
			AbilitySpecInputReleased(*AbilitySpec);

			const TArray<UGameplayAbility*> AbilityInstances = AbilitySpec->GetAbilityInstances();
			if (!AbilityInstances.IsEmpty())
			{
				InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, AbilitySpecHandle, AbilityInstances.Last()->GetCurrentActivationInfoRef().GetActivationPredictionKey());
			}
		}
	}

	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
}

void USWAbilitySystemComponent::GrantStartupAbilities(const TArray<FSWStartupAbility>& StartupAbilities)
{
	if (!IsOwnerActorAuthoritative())
	{
		return;
	}

	for (const FSWStartupAbility& StartupAbility : StartupAbilities)
	{
		if (!StartupAbility.AbilityClass || !StartupAbility.InputTag.IsValid())
		{
			continue;
		}

		bool bAlreadyGranted = false;
		for (const FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
		{
			if (AbilitySpec.Ability && AbilitySpec.Ability->GetClass() == StartupAbility.AbilityClass.Get())
			{
				bAlreadyGranted = true;
				break;
			}
		}

		if (bAlreadyGranted)
		{
			continue;
		}

		FGameplayAbilitySpec AbilitySpec(StartupAbility.AbilityClass.Get(), 1);
		AbilitySpec.GetDynamicSpecSourceTags().AddTag(StartupAbility.InputTag);
		GiveAbility(AbilitySpec);
	}
}
