// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "AbilitySystem/Abilities/Input/SWWeaponAimGameplayAbility.h"

#include "Character/SWCharacter_Player.h"
#include "GameplayTags/SWGameplayTags.h"
#include "Weapon/SWWeapon.h"

USWWeaponAimGameplayAbility::USWWeaponAimGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(SWGameplayTags::Ability_Weapon_Aim);
	SetAssetTags(AssetTags);
	ActivationOwnedTags.AddTag(SWGameplayTags::State_Weapon_Aiming);
	ActivationBlockedTags.AddTag(SWGameplayTags::State_Weapon_Reloading);
	CancelAbilitiesWithTag.AddTag(SWGameplayTags::Ability_Movement_Sprint);
}

bool USWWeaponAimGameplayAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags)
		&& GetCurrentWeapon(ActorInfo) != nullptr
		&& GetCurrentWeapon(ActorInfo)->SupportsAim();
}

void USWWeaponAimGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	ASWWeapon* Weapon = GetCurrentWeapon();
	ASWCharacter_Player* Character = GetPlayerCharacter();
	float AimFOV = 0.f;
	FVector AimCameraOffset = FVector::ZeroVector;
	float TransitionSeconds = 0.f;
	if (!Weapon || !Character || !Weapon->GetAimCameraSettings(AimFOV, AimCameraOffset, TransitionSeconds)
		|| !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	Character->SetLocalAimCameraSettings(true, AimFOV, AimCameraOffset, TransitionSeconds);
}

void USWWeaponAimGameplayAbility::InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputReleased(Handle, ActorInfo, ActivationInfo);
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void USWWeaponAimGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const bool bReplicateEndAbility, const bool bWasCancelled)
{
	if (ASWCharacter_Player* Character = GetPlayerCharacter())
	{
		Character->ClearLocalAimCameraSettings();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
