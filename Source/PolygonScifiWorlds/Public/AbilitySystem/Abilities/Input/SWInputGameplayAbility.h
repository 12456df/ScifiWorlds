// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/SWGameplayAbility.h"
#include "SWInputGameplayAbility.generated.h"

class ASWCharacter_Player;
class ASWWeapon;
class UAnimMontage;
class USWCharacterMovementComponent;

/**
 * 由 Enhanced Input Tag 激活的 Ability 共用基类。
 *
 * 它只集中 Avatar、固定武器与 CMC 的受控查询，不承载任何具体玩法行为；
 * 疾跑、开火、瞄准和换弹各自在独立子类中实现状态与生命周期。
 */
UCLASS(Abstract)
class POLYGONSCIFIWORLDS_API USWInputGameplayAbility : public USWGameplayAbility
{
	GENERATED_BODY()

protected:
	ASWCharacter_Player* GetPlayerCharacter() const;
	ASWWeapon* GetCurrentWeapon() const;
	static ASWCharacter_Player* GetPlayerCharacter(const FGameplayAbilityActorInfo* ActorInfo);
	static ASWWeapon* GetCurrentWeapon(const FGameplayAbilityActorInfo* ActorInfo);
	USWCharacterMovementComponent* GetCharacterMovement() const;
	bool IsAvatarAuthority() const;
	void PlayWeaponMontage(UAnimMontage* MontageToPlay);
};
