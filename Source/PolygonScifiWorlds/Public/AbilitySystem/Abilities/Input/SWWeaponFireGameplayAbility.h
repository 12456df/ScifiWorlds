// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Input/SWInputGameplayAbility.h"
#include "Weapon/SWWeaponTypes.h"
#include "SWWeaponFireGameplayAbility.generated.h"

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

	/** 为本次开火选择并缓存一个 Montage 变体，蓝图使用返回的播放倍率播放即可。 */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Fire")
	FSWFireMontageSelection SelectNextFireMontage();

	/** 返回已缓存的本次射击表现数据；未选择时 bValid 为 false。 */
	UFUNCTION(BlueprintPure, Category = "Weapon|Fire")
	FSWFireMontageSelection GetSelectedFireMontage() const;

	/** 供 GA 蓝图区分半自动和自动的内容编排。 */
	UFUNCTION(BlueprintPure, Category = "Weapon|Fire")
	bool IsCurrentWeaponAutomatic() const;

	/**
	 * 仅在 PlayMontageAndWait 已创建活动 Montage 实例后调用。
	 * 根据本 Ability 已记录的输入状态配置循环，避免服务器先收到输入释放、随后又被播放后配置错误地改回循环。
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Fire")
	bool ConfigureActiveFireMontageSections();

	/**
	 * 由蓝图的输入释放或权威射击失败分支调用。
	 * 先记录停止意图；若 Montage 已激活，立即把 FireCycle 导向 FireRecovery。
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Fire")
	void RequestFireMontageRecovery();

	/**
	 * 仅允许服务器在动画 Gameplay Event 到达时调用的权威发射入口。
	 * 函数再次校验 Ability 活动状态、武器、弹药和射速；蓝图无法直接生成弹丸或修改弹药。
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Fire")
	bool CommitFireFromAnimEvent();

private:
	/** 仅用于本 Ability 实例的内容播放顺序，不复制且不参与服务器权威结算。 */
	FSWFireMontageSelection SelectedFireMontage;

	int32 NextFireMontageVariantIndex = 0;

	/** 本 Ability 实例的输入意图；客户端与服务器实例分别由各自的 GAS 输入事件更新。 */
	bool bFireInputHeld = false;
};
