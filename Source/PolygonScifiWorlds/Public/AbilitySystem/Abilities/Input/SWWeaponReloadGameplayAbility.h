// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Input/SWInputGameplayAbility.h"
#include "SWWeaponReloadGameplayAbility.generated.h"

/** 管理换弹状态和权威等待；无限备弹只在等待结束时填满当前弹匣。 */
UCLASS(Blueprintable)
class POLYGONSCIFIWORLDS_API USWWeaponReloadGameplayAbility : public USWInputGameplayAbility
{
	GENERATED_BODY()

public:
	USWWeaponReloadGameplayAbility();

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr,
		OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	bool bReloadPresentationActive = false;

	UFUNCTION()
	void OnReloadFinished();
};
