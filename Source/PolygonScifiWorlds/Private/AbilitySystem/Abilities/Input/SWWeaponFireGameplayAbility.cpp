// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "AbilitySystem/Abilities/Input/SWWeaponFireGameplayAbility.h"

#include "GameplayTags/SWGameplayTags.h"
#include "Weapon/SWWeapon.h"

USWWeaponFireGameplayAbility::USWWeaponFireGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(SWGameplayTags::Ability_Weapon_Fire);
	SetAssetTags(AssetTags);
	ActivationOwnedTags.AddTag(SWGameplayTags::State_Weapon_Firing);
	ActivationBlockedTags.AddTag(SWGameplayTags::State_Weapon_Reloading);
	CancelAbilitiesWithTag.AddTag(SWGameplayTags::Ability_Movement_Sprint);
}

bool USWWeaponFireGameplayAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags)
		&& GetCurrentWeapon(ActorInfo) != nullptr
		&& GetCurrentWeapon(ActorInfo)->CanFire();
}

UAnimMontage* USWWeaponFireGameplayAbility::GetCurrentWeaponFireMontage() const
{
	if (const ASWWeapon* Weapon = GetCurrentWeapon())
	{
		return Weapon->GetFireMontage();
	}

	return nullptr;
}

bool USWWeaponFireGameplayAbility::IsCurrentWeaponAutomatic() const
{
	if (const ASWWeapon* Weapon = GetCurrentWeapon())
	{
		return Weapon->IsAutomatic();
	}

	return false;
}

float USWWeaponFireGameplayAbility::GetCurrentWeaponFireIntervalSeconds() const
{
	if (const ASWWeapon* Weapon = GetCurrentWeapon())
	{
		return Weapon->GetEffectiveFireIntervalSeconds();
	}

	return 0.f;
}

bool USWWeaponFireGameplayAbility::CommitFireFromAnimEvent()
{
	if (!IsAvatarAuthority() || !IsActive())
	{
		return false;
	}

	ASWWeapon* Weapon = GetCurrentWeapon();
	return Weapon && Weapon->TryFireAuthority().bFired;
}
