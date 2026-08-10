// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Input/SWInputGameplayAbility.h"
#include "GameplayEffectTypes.h"
#include "SWSprintGameplayAbility.generated.h"

class UGameplayEffect;

/** 只管理疾跑输入意图与生命周期；实际速度和网络预测由 CMC 负责。 */
UCLASS(Blueprintable)
class POLYGONSCIFIWORLDS_API USWSprintGameplayAbility : public USWInputGameplayAbility
{
	GENERATED_BODY()

public:
	USWSprintGameplayAbility();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	/** 疾跑激活期间由服务器施加的无限周期 GE；幅度与周期均由蓝图资产配置。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SW|Sprint", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayEffect> StaminaDrainEffect;

	/** 仅保存服务器实际施加的 GE 句柄，用于 Ability 结束时精确移除。 */
	FActiveGameplayEffectHandle StaminaDrainEffectHandle;

	bool ApplyStaminaDrainAuthority(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo);
	void RemoveStaminaDrainAuthority(const FGameplayAbilityActorInfo* ActorInfo);

	/** 只由疾跑 Ability 调用，向当前 Avatar 的 CMC 写入本地预测与服务器权威共用的疾跑意图。 */
	void SetSprintRequested(bool bRequested) const;
};
