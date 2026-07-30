// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Input/SWInputGameplayAbility.h"
#include "SWSprintGameplayAbility.generated.h"

/** 只管理疾跑输入意图与生命周期；实际速度和网络预测由 CMC 负责。 */
UCLASS(Blueprintable)
class POLYGONSCIFIWORLDS_API USWSprintGameplayAbility : public USWInputGameplayAbility
{
	GENERATED_BODY()

public:
	USWSprintGameplayAbility();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
};
