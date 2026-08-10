// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "AbilitySystem/SWAbilitySystemComponent.h"

#include "AbilitySystem/Abilities/Active/SWActiveGameplayAbility.h"
#include "AbilitySystem/Abilities/SWGameplayAbility.h"
#include "AbilitySystem/Effects/SWGrantExperienceGameplayEffect.h"
#include "AbilitySystem/Effects/SWHealGameplayEffect.h"
#include "AbilitySystem/Effects/SWDamageGameplayEffect.h"
#include "AbilitySystem/SWAttributeSet.h"
#include "GameplayTags/SWGameplayTags.h"
#include "Interaction/SWPlayerProgressionInterface.h"
#include "Player/SWPlayerState.h"

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

void USWAbilitySystemComponent::OnGiveAbility(FGameplayAbilitySpec& AbilitySpec)
{
	Super::OnGiveAbility(AbilitySpec);
	OnActivatableAbilitySpecChanged.Broadcast(AbilitySpec, ESWActivatableAbilitySpecChangeType::Added);
}

void USWAbilitySystemComponent::OnRemoveAbility(FGameplayAbilitySpec& AbilitySpec)
{
	Super::OnRemoveAbility(AbilitySpec);

	// 引擎在此回调返回后才会从数组移除 Spec；直接携带 Spec 广播，订阅方无需读取这个临界状态。
	OnActivatableAbilitySpecChanged.Broadcast(AbilitySpec, ESWActivatableAbilitySpecChangeType::Removed);
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

bool USWAbilitySystemComponent::TryConsumeGenericConfirmInput()
{
	if (!GenericLocalConfirmCallbacks.IsBound())
	{
		return false;
	}

	InputConfirm();
	return true;
}

bool USWAbilitySystemComponent::TryConsumeGenericCancelInput()
{
	if (!GenericLocalCancelCallbacks.IsBound())
	{
		return false;
	}

	InputCancel();
	return true;
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
			UE_LOG(LogTemp, Warning, TEXT("跳过无效的启动技能配置：AbilityClass 或 InputTag 缺失。"));
			continue;
		}

		const USWActiveGameplayAbility* const StartupActiveAbilityCDO = Cast<USWActiveGameplayAbility>(StartupAbility.AbilityClass.GetDefaultObject());
		if (StartupActiveAbilityCDO && !StartupActiveAbilityCDO->GetAbilityIdTag().IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("跳过缺少 AbilityIdTag 的启动技能：%s"), *GetNameSafe(StartupAbility.AbilityClass));
			continue;
		}

		bool bAlreadyGranted = false;
		for (const FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
		{
			const USWActiveGameplayAbility* const ExistingActiveAbilityCDO = Cast<USWActiveGameplayAbility>(AbilitySpec.Ability);
			if (AbilitySpec.Ability && (AbilitySpec.Ability->GetClass() == StartupAbility.AbilityClass.Get()
				|| (StartupActiveAbilityCDO && ExistingActiveAbilityCDO
					&& ExistingActiveAbilityCDO->GetAbilityIdTag() == StartupActiveAbilityCDO->GetAbilityIdTag())))
			{
				bAlreadyGranted = true;
				break;
			}
		}

		if (bAlreadyGranted)
		{
			continue;
		}

		// 仅首次授予使用配置等级；已有 Spec（包括重生后的已升级技能）保持其服务器真值。
		FGameplayAbilitySpec AbilitySpec(StartupAbility.AbilityClass.Get(), FMath::Max(1, StartupAbility.StartingLevel));
		AbilitySpec.GetDynamicSpecSourceTags().AddTag(StartupAbility.InputTag);
		GiveAbility(AbilitySpec);
	}
}

bool USWAbilitySystemComponent::TryUpgradeActiveAbilityAuthority(const FGameplayTag InputTag)
{
	if (!IsOwnerActorAuthoritative()
		|| (InputTag != SWGameplayTags::Ability_Input_Skill1
			&& InputTag != SWGameplayTags::Ability_Input_Skill2
			&& InputTag != SWGameplayTags::Ability_Input_Skill3))
	{
		return false;
	}

	ASWPlayerState* const OwningPlayerState = Cast<ASWPlayerState>(GetOwnerActor());
	if (!OwningPlayerState || OwningPlayerState->GetAbilityPoints() <= 0)
	{
		return false;
	}

	for (FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		if (!AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			continue;
		}

		const USWActiveGameplayAbility* const ActiveAbility = Cast<USWActiveGameplayAbility>(AbilitySpec.Ability);
		if (!ActiveAbility || !ActiveAbility->IsUpgradeable() || AbilitySpec.IsActive()
			|| AbilitySpec.Level < 1 || AbilitySpec.Level >= ActiveAbility->GetMaxAbilityLevel())
		{
			return false;
		}

		// 校验完成后才扣点；失败路径不会改变 PlayerState 或 Ability Spec。
		if (!OwningPlayerState->SpendAbilityPoint())
		{
			return false;
		}

		++AbilitySpec.Level;
		MarkAbilitySpecDirty(AbilitySpec);
		return true;
	}

	return false;
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
	AActor* const EffectCauser,
	const FSWDamageApplicationParams& DamageParams)
{
	return ApplyDamageEffectToTargetWithHandleAuthority(
		TargetAbilitySystemComponent,
		DamageEffectClass,
		EffectLevel,
		EffectCauser,
		DamageParams).WasSuccessfullyApplied();
}

FActiveGameplayEffectHandle USWAbilitySystemComponent::ApplyDamageEffectToTargetWithHandleAuthority(
	UAbilitySystemComponent* const TargetAbilitySystemComponent,
	const TSubclassOf<USWDamageGameplayEffect> DamageEffectClass,
	const int32 EffectLevel,
	AActor* const EffectCauser,
	const FSWDamageApplicationParams& DamageParams)
{
	if (!IsOwnerActorAuthoritative() || !TargetAbilitySystemComponent || !TargetAbilitySystemComponent->IsOwnerActorAuthoritative()
		|| TargetAbilitySystemComponent == this || !DamageEffectClass || !FMath::IsFinite(DamageParams.RawDamage) || DamageParams.RawDamage <= 0.f)
	{
		return FActiveGameplayEffectHandle();
	}

	if (DamageParams.DamageType != SWGameplayTags::Damage_Type_Physical
		&& DamageParams.DamageType != SWGameplayTags::Damage_Type_Magical
		&& DamageParams.DamageType != SWGameplayTags::Damage_Type_True)
	{
		return FActiveGameplayEffectHandle();
	}

	AActor* const SourceAvatar = GetAvatarActor();
	if (!SourceAvatar)
	{
		return FActiveGameplayEffectHandle();
	}

	FGameplayEffectContextHandle EffectContext = MakeEffectContext();
	EffectContext.AddInstigator(SourceAvatar, EffectCauser ? EffectCauser : SourceAvatar);
	EffectContext.AddSourceObject(EffectCauser ? EffectCauser : SourceAvatar);
	FSWGameplayEffectContext* const SWContext = EffectContext.Get() && EffectContext.Get()->GetScriptStruct() == FSWGameplayEffectContext::StaticStruct()
		? static_cast<FSWGameplayEffectContext*>(EffectContext.Get())
		: nullptr;
	if (!SWContext)
	{
		return FActiveGameplayEffectHandle();
	}

	SWContext->SetDamageType(DamageParams.DamageType);
	SWContext->SetCanCritical(DamageParams.bCanCritical);
	const FGameplayEffectSpecHandle EffectSpec = MakeOutgoingSpec(DamageEffectClass, FMath::Max(1, EffectLevel), EffectContext);
	if (!EffectSpec.IsValid())
	{
		return FActiveGameplayEffectHandle();
	}

	EffectSpec.Data->SetSetByCallerMagnitude(SWGameplayTags::SetByCaller_Damage_Raw, DamageParams.RawDamage);
	if (FMath::IsFinite(DamageParams.EffectDurationSeconds) && DamageParams.EffectDurationSeconds > 0.f)
	{
		EffectSpec.Data->SetSetByCallerMagnitude(SWGameplayTags::SetByCaller_Ability_Duration, DamageParams.EffectDurationSeconds);
	}

	return ApplyGameplayEffectSpecToTarget(*EffectSpec.Data.Get(), TargetAbilitySystemComponent);
}

bool USWAbilitySystemComponent::ApplyHealingToSelfAuthority(const float Healing, AActor* const EffectCauser)
{
	if (!IsOwnerActorAuthoritative() || !FMath::IsFinite(Healing) || Healing <= 0.f
		|| HasMatchingGameplayTag(SWGameplayTags::State_Dead))
	{
		return false;
	}

	AActor* const SourceAvatar = GetAvatarActor();
	const USWAttributeSet* const Attributes = GetSet<USWAttributeSet>();
	if (!SourceAvatar || !Attributes || Attributes->GetHealth() <= 0.f)
	{
		return false;
	}

	FGameplayEffectContextHandle EffectContext = MakeEffectContext();
	EffectContext.AddInstigator(SourceAvatar, EffectCauser ? EffectCauser : SourceAvatar);
	EffectContext.AddSourceObject(EffectCauser ? EffectCauser : SourceAvatar);
	FGameplayEffectSpecHandle EffectSpec = MakeOutgoingSpec(USWHealGameplayEffect::StaticClass(), 1.f, EffectContext);
	if (!EffectSpec.IsValid())
	{
		return false;
	}

	EffectSpec.Data->SetSetByCallerMagnitude(SWGameplayTags::SetByCaller_Healing, Healing);
	// Instant GE 不会生成有效的 Active Handle；Spec 有效且已经通过服务器检查即可执行。
	ApplyGameplayEffectSpecToSelf(*EffectSpec.Data.Get());
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
