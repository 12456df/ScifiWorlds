// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Input/SWInputGameplayAbility.h"
#include "SWWeaponFireGameplayAbility.generated.h"

class UAnimMontage;

/**
 * 开火 Ability 的最小 C++ 权威契约。
 * 具体 Montage、Wait Gameplay Event、自动射击循环和结束分支由对应的 Ability 蓝图编排。
 */
UCLASS(Blueprintable)
class POLYGONSCIFIWORLDS_API USWWeaponFireGameplayAbility : public USWInputGameplayAbility
{
	GENERATED_BODY()

public:
	USWWeaponFireGameplayAbility();

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr,
		OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	/** 供 GA 蓝图的 Play Montage and Wait 节点读取当前武器的开火 Montage。 */
	UFUNCTION(BlueprintPure, Category = "Weapon|Fire")
	UAnimMontage* GetCurrentWeaponFireMontage() const;

	/** 供 GA 蓝图区分半自动和自动的内容编排。 */
	UFUNCTION(BlueprintPure, Category = "Weapon|Fire")
	bool IsCurrentWeaponAutomatic() const;

	/**
	 * 返回当前武器已计入 AttributeSet 修正后的射击间隔。
	 * 仅供 GA 蓝图安排自动射击的本地等待；服务器仍在实际发射时独立校验射速。
	 */
	UFUNCTION(BlueprintPure, Category = "Weapon|Fire")
	float GetCurrentWeaponFireIntervalSeconds() const;

	/**
	 * 仅允许服务器在动画 Gameplay Event 到达时调用的权威发射入口。
	 * 函数再次校验 Ability 活动状态、武器、弹药和射速；蓝图无法直接生成弹丸或修改弹药。
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Fire")
	bool CommitFireFromAnimEvent();
};
