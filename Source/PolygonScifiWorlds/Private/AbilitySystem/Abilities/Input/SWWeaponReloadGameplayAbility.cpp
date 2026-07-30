// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "AbilitySystem/Abilities/Input/SWWeaponReloadGameplayAbility.h"

#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Character/SWCharacter_Player.h"
#include "GameplayTags/SWGameplayTags.h"
#include "Weapon/SWWeapon.h"

USWWeaponReloadGameplayAbility::USWWeaponReloadGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(SWGameplayTags::Ability_Weapon_Reload);
	SetAssetTags(AssetTags);
	ActivationOwnedTags.AddTag(SWGameplayTags::State_Weapon_Reloading);
	ActivationBlockedTags.AddTag(SWGameplayTags::State_Weapon_Firing);
	CancelAbilitiesWithTag.AddTag(SWGameplayTags::Ability_Movement_Sprint);
	CancelAbilitiesWithTag.AddTag(SWGameplayTags::Ability_Weapon_Aim);
	CancelAbilitiesWithTag.AddTag(SWGameplayTags::Ability_Weapon_Fire);
}

bool USWWeaponReloadGameplayAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags)
		&& GetCurrentWeapon(ActorInfo) != nullptr
		&& GetCurrentWeapon(ActorInfo)->CanReload();
}

void USWWeaponReloadGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	ASWWeapon* Weapon = GetCurrentWeapon();
	if (!Weapon || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (IsAvatarAuthority())
	{
		Weapon->NotifyReloadStateChangedAuthority(true);
		bReloadPresentationActive = true;
	}
	PlayWeaponMontage(Weapon->GetReloadMontage());

	// 客户端只预测 Montage 和状态 Tag，不能自行结束 Ability；否则其结束同步可能在服务器计时完成前取消权威流程。
	if (!IsAvatarAuthority())
	{
		return;
	}

	// 只有服务器拥有换弹结算时钟，并在回调中填充权威弹匣后结束 Ability。
	UAbilityTask_WaitDelay* WaitTask = UAbilityTask_WaitDelay::WaitDelay(this, Weapon->GetReloadDurationSeconds());
	WaitTask->OnFinish.AddDynamic(this, &ThisClass::OnReloadFinished);
	WaitTask->ReadyForActivation();
}

void USWWeaponReloadGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const bool bReplicateEndAbility, const bool bWasCancelled)
{
	if (IsAvatarAuthority() && bReloadPresentationActive)
	{
		if (ASWWeapon* Weapon = GetCurrentWeapon())
		{
			Weapon->NotifyReloadStateChangedAuthority(false);
		}
		bReloadPresentationActive = false;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void USWWeaponReloadGameplayAbility::OnReloadFinished()
{
	if (IsAvatarAuthority())
	{
		if (ASWWeapon* Weapon = GetCurrentWeapon())
		{
			Weapon->TryCommitReloadAuthority();
		}
	}

	EndAbility(CurrentSpecHandle, GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}
