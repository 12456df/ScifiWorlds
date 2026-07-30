// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "AbilitySystem/Abilities/Input/SWSprintGameplayAbility.h"

#include "GameplayTags/SWGameplayTags.h"

USWSprintGameplayAbility::USWSprintGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(SWGameplayTags::Ability_Movement_Sprint);
	SetAssetTags(AssetTags);
	ActivationOwnedTags.AddTag(SWGameplayTags::State_Movement_Sprinting);
	ActivationBlockedTags.AddTag(SWGameplayTags::State_Weapon_Aiming);
	ActivationBlockedTags.AddTag(SWGameplayTags::State_Weapon_Reloading);
	ActivationBlockedTags.AddTag(SWGameplayTags::State_Weapon_Firing);
}

void USWSprintGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	SetAvatarSprintRequested(true);
}

void USWSprintGameplayAbility::InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputReleased(Handle, ActorInfo, ActivationInfo);
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void USWSprintGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const bool bReplicateEndAbility, const bool bWasCancelled)
{
	SetAvatarSprintRequested(false);
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
