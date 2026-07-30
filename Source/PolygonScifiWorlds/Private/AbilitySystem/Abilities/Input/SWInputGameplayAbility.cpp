// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "AbilitySystem/Abilities/Input/SWInputGameplayAbility.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Character/SWCharacter_Player.h"
#include "Movement/SWCharacterMovementComponent.h"
#include "Weapon/SWWeapon.h"

ASWCharacter_Player* USWInputGameplayAbility::GetPlayerCharacter() const
{
	return GetPlayerCharacter(GetCurrentActorInfo());
}

ASWWeapon* USWInputGameplayAbility::GetCurrentWeapon() const
{
	return GetCurrentWeapon(GetCurrentActorInfo());
}

ASWCharacter_Player* USWInputGameplayAbility::GetPlayerCharacter(const FGameplayAbilityActorInfo* ActorInfo)
{
	return ActorInfo ? Cast<ASWCharacter_Player>(ActorInfo->AvatarActor.Get()) : nullptr;
}

ASWWeapon* USWInputGameplayAbility::GetCurrentWeapon(const FGameplayAbilityActorInfo* ActorInfo)
{
	if (ASWCharacter_Player* Character = GetPlayerCharacter(ActorInfo))
	{
		return Character->GetCurrentWeapon();
	}

	return nullptr;
}

USWCharacterMovementComponent* USWInputGameplayAbility::GetCharacterMovement() const
{
	if (ASWCharacter_Player* Character = GetPlayerCharacter())
	{
		return Character->GetCharacterMovement<USWCharacterMovementComponent>();
	}

	return nullptr;
}

bool USWInputGameplayAbility::IsAvatarAuthority() const
{
	if (const AActor* AvatarActor = GetAvatarActorFromActorInfo())
	{
		return AvatarActor->HasAuthority();
	}

	return false;
}

void USWInputGameplayAbility::PlayWeaponMontage(UAnimMontage* MontageToPlay)
{
	if (!MontageToPlay)
	{
		return;
	}

	// Ability 结束时不强制停止 Montage，避免瞬时半自动开火 Ability 立即截断表现。
	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, MontageToPlay, 1.f, NAME_None, false);
	MontageTask->ReadyForActivation();
}
