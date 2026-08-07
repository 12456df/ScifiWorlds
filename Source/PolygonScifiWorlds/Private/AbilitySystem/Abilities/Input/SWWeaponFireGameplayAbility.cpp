// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "AbilitySystem/Abilities/Input/SWWeaponFireGameplayAbility.h"

#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Character/SWCharacter_Player.h"
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

FSWFireMontageSelection USWWeaponFireGameplayAbility::SelectNextFireMontage()
{
	bFireInputHeld = true;
	SelectedFireMontage = FSWFireMontageSelection();
	if (const ASWWeapon* Weapon = GetCurrentWeapon())
	{
		Weapon->ResolveFireMontageSelection(NextFireMontageVariantIndex, SelectedFireMontage);
		if (SelectedFireMontage.bValid)
		{
			NextFireMontageVariantIndex = SelectedFireMontage.VariantIndex + 1;
		}
	}

	return SelectedFireMontage;
}

FSWFireMontageSelection USWWeaponFireGameplayAbility::GetSelectedFireMontage() const
{
	return SelectedFireMontage;
}

bool USWWeaponFireGameplayAbility::IsCurrentWeaponAutomatic() const
{
	if (const ASWWeapon* Weapon = GetCurrentWeapon())
	{
		return Weapon->IsAutomatic();
	}

	return false;
}

bool USWWeaponFireGameplayAbility::ConfigureActiveFireMontageSections()
{
	const FSWFireMontageSelection& Selection = SelectedFireMontage;
	ASWCharacter_Player* const Character = GetPlayerCharacter();
	USkeletalMeshComponent* const Mesh = Character ? Character->GetMesh() : nullptr;
	UAnimInstance* const AnimInstance = Mesh ? Mesh->GetAnimInstance() : nullptr;
	if (!Selection.bValid || !Selection.Montage || !AnimInstance || !AnimInstance->Montage_IsActive(Selection.Montage))
	{
		return false;
	}

	// 该调用必须发生在 Montage 已播放之后；播放前没有活动实例，设置会被引擎忽略。
	AnimInstance->Montage_SetNextSection(
		TEXT("FireCycle"),
		IsCurrentWeaponAutomatic() && bFireInputHeld ? TEXT("FireCycle") : TEXT("FireRecovery"),
		Selection.Montage);
	return true;
}

void USWWeaponFireGameplayAbility::RequestFireMontageRecovery()
{
	bFireInputHeld = false;

	const FSWFireMontageSelection& Selection = SelectedFireMontage;
	ASWCharacter_Player* const Character = GetPlayerCharacter();
	USkeletalMeshComponent* const Mesh = Character ? Character->GetMesh() : nullptr;
	UAnimInstance* const AnimInstance = Mesh ? Mesh->GetAnimInstance() : nullptr;
	if (!Selection.bValid || !Selection.Montage || !AnimInstance || !AnimInstance->Montage_IsActive(Selection.Montage))
	{
		return;
	}

	AnimInstance->Montage_SetNextSection(TEXT("FireCycle"), TEXT("FireRecovery"), Selection.Montage);
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
