// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Effects/SWDamageGameplayEffect.h"
#include "AbilitySystem/SWAbilityTypes.h"
#include "AbilitySystemComponent.h"
#include "Team/SWTeamTypes.h"
#include "SWAbilitySystemComponent.generated.h"

/**
 * Project ability system component for ScifiWorlds.
 *
 * Owns ability, effect, tag and attribute aggregation as described in
 * Docs/Systems/M03_GASCoreFramework.md. This module establishes the type; ability
 * granting, input binding and effect helpers land with later delivery steps.
 */
UCLASS()
class POLYGONSCIFIWORLDS_API USWAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	/** 记录一个本地技能输入按下事件；实际激活统一延后到本帧输入处理阶段。 */
	void AbilityInputTagPressed(FGameplayTag InputTag);

	/** 记录一个本地技能输入释放事件。 */
	void AbilityInputTagReleased(FGameplayTag InputTag);

	/** 在 PlayerController::PostProcessInput 中调用，处理按下、持续和释放的 Ability Spec 输入。 */
	void ProcessAbilityInput(float DeltaTime, bool bGamePaused);

	/** 仅服务器调用：授予蓝图默认值配置的启动技能，并用输入 Tag 写入对应 Ability Spec。 */
	void GrantStartupAbilities(const TArray<FSWStartupAbility>& StartupAbilities);

	/**
	 * 仅服务器调用：将唯一 TeamId 真值镜像为一个可复制的 ASC Loose Tag。
	 * 调用后 State.Team.None/TeamA/TeamB 三者恰好保留一个；蓝图不得直接修改这些 Tag。
	 */
	void SetTeamIdTagAuthority(ESWTeamId TeamId);

	/** 仅服务器调用：写入或清理可复制的死亡状态 Tag，供死亡与重生流程统一使用。 */
	void SetDeadStateTagAuthority(bool bIsDead);

	bool ApplyDamageEffectToTargetAuthority(
		UAbilitySystemComponent* TargetAbilitySystemComponent,
		TSubclassOf<USWDamageGameplayEffect> DamageEffectClass,
		int32 EffectLevel,
		AActor* EffectCauser);

	/**
	 * 仅服务器调用：对仍存活的 ASC 自身应用一次瞬时治疗。
	 * 这是物理吸血等服务器结算治疗的唯一写入口；蓝图不得直接写入 Health。
	 */
	bool ApplyHealingToSelfAuthority(float Healing, AActor* EffectCauser);

	/**
	 * 仅服务器调用：向本 ASC 所属玩家结算一次经验奖励。
	 * 该入口只创建 GE Spec；实际经验、等级和技能点仍由目标 PlayerState 的 AttributeSet/Progression 接口唯一写入。
	 */
	bool ApplyExperienceRewardToSelfAuthority(int32 ExperienceReward, AActor* RewardSource);

private:
	/** 本帧按下和释放的 Ability Spec；仅本地输入状态，不复制。 */
	TArray<FGameplayAbilitySpecHandle> InputPressedSpecHandles;
	TArray<FGameplayAbilitySpecHandle> InputReleasedSpecHandles;
};
