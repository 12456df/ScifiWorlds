// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "AbilitySystem/SWAbilitySystemComponent.h"

#include "AbilitySystem/Abilities/SWGameplayAbility.h"
#include "AbilitySystem/Effects/SWGrantExperienceGameplayEffect.h"
#include "AbilitySystem/Effects/SWDamageGameplayEffect.h"
#include "GameplayTags/SWGameplayTags.h"
#include "Interaction/SWPlayerProgressionInterface.h"

namespace
{
	FGameplayTag GetTeamTag(const ESWTeamId TeamId)
	{
		switch (TeamId)
		{
		case ESWTeamId::TeamA:
			return SWGameplayTags::State_Team_TeamA;
		case ESWTeamId::TeamB:
			return SWGameplayTags::State_Team_TeamB;
		case ESWTeamId::None:
		default:
			return SWGameplayTags::State_Team_None;
		}
	}
}

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

	if (HasMatchingGameplayTag(SWGameplayTags::State_Dead))
	{
		// 死亡期间不保留旧输入，避免重生后把死亡前的按键当作新的 Ability 请求。
		InputPressedSpecHandles.Reset();
		InputReleasedSpecHandles.Reset();
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

void USWAbilitySystemComponent::SetTeamIdTagAuthority(const ESWTeamId TeamId)
{
	check(IsOwnerActorAuthoritative());

	// 使用 Set 而不是增减计数，确保重复初始化或重生后不会累积同一 Tag。
	SetLooseGameplayTagCount(SWGameplayTags::State_Team_None, 0, EGameplayTagReplicationState::TagOnly);
	SetLooseGameplayTagCount(SWGameplayTags::State_Team_TeamA, 0, EGameplayTagReplicationState::TagOnly);
	SetLooseGameplayTagCount(SWGameplayTags::State_Team_TeamB, 0, EGameplayTagReplicationState::TagOnly);
	SetLooseGameplayTagCount(GetTeamTag(TeamId), 1, EGameplayTagReplicationState::TagOnly);
}

void USWAbilitySystemComponent::SetDeadStateTagAuthority(const bool bIsDead)
{
	check(IsOwnerActorAuthoritative());

	SetLooseGameplayTagCount(SWGameplayTags::State_Dead, bIsDead ? 1 : 0, EGameplayTagReplicationState::TagOnly);
}

bool USWAbilitySystemComponent::ApplyDamageEffectToTargetAuthority(
	UAbilitySystemComponent* const TargetAbilitySystemComponent,
	const TSubclassOf<USWDamageGameplayEffect> DamageEffectClass,
	const int32 EffectLevel,
	AActor* const EffectCauser)
{
	if (!IsOwnerActorAuthoritative() || !TargetAbilitySystemComponent || !TargetAbilitySystemComponent->IsOwnerActorAuthoritative()
		|| TargetAbilitySystemComponent == this || !DamageEffectClass)
	{
		return false;
	}

	AActor* const SourceAvatar = GetAvatarActor();
	if (!SourceAvatar)
	{
		return false;
	}

	FGameplayEffectContextHandle EffectContext = MakeEffectContext();
	EffectContext.AddInstigator(SourceAvatar, EffectCauser ? EffectCauser : SourceAvatar);
	EffectContext.AddSourceObject(EffectCauser ? EffectCauser : SourceAvatar);
	const FGameplayEffectSpecHandle EffectSpec = MakeOutgoingSpec(DamageEffectClass, FMath::Max(1, EffectLevel), EffectContext);
	if (!EffectSpec.IsValid())
	{
		return false;
	}

	ApplyGameplayEffectSpecToTarget(*EffectSpec.Data.Get(), TargetAbilitySystemComponent);
	return true;
}

bool USWAbilitySystemComponent::ApplyExperienceRewardToSelfAuthority(const int32 ExperienceReward, AActor* const RewardSource)
{
	if (!IsOwnerActorAuthoritative() || ExperienceReward <= 0
		|| !Cast<ISWPlayerProgressionInterface>(GetOwnerActor()))
	{
		return false;
	}

	AActor* const SourceAvatar = GetAvatarActor();
	if (!SourceAvatar)
	{
		return false;
	}

	FGameplayEffectContextHandle EffectContext = MakeEffectContext();
	EffectContext.AddInstigator(SourceAvatar, RewardSource ? RewardSource : SourceAvatar);
	EffectContext.AddSourceObject(RewardSource ? RewardSource : SourceAvatar);

	FGameplayEffectSpecHandle EffectSpec = MakeOutgoingSpec(USWGrantExperienceGameplayEffect::StaticClass(), 1.f, EffectContext);
	if (!EffectSpec.IsValid())
	{
		return false;
	}

	EffectSpec.Data->SetSetByCallerMagnitude(SWGameplayTags::SetByCaller_Experience, static_cast<float>(ExperienceReward));
	ApplyGameplayEffectSpecToSelf(*EffectSpec.Data.Get());
	return true;
}
