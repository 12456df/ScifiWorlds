// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Effects/SWDamageGameplayEffect.h"
#include "AbilitySystem/SWAbilityTypes.h"
#include "AbilitySystemComponent.h"
#include "Team/SWTeamTypes.h"
#include "SWAbilitySystemComponent.generated.h"

/** 已授予主动技能 Spec 的精确变更类型；仅供本地展示层订阅。 */
enum class ESWActivatableAbilitySpecChangeType : uint8
{
	Added,
	Removed
};

/** 已授予主动技能 Spec 增删时广播原始 Spec 与变更类型。 */
DECLARE_MULTICAST_DELEGATE_TwoParams(FSWOnActivatableAbilitySpecChanged, const FGameplayAbilitySpec&, ESWActivatableAbilitySpecChangeType);

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
	/** 主动技能 Spec 增删时广播；服务器授予和客户端复制均会触发。 */
	FSWOnActivatableAbilitySpecChanged OnActivatableAbilitySpecChanged;

	virtual void OnGiveAbility(FGameplayAbilitySpec& AbilitySpec) override;
	virtual void OnRemoveAbility(FGameplayAbilitySpec& AbilitySpec) override;

	/** 记录一个本地技能输入按下事件；实际激活统一延后到本帧输入处理阶段。 */
	void AbilityInputTagPressed(FGameplayTag InputTag);

	/** 记录一个本地技能输入释放事件。 */
	void AbilityInputTagReleased(FGameplayTag InputTag);

	/**
	 * 仅由本地输入路由调用：若有 GAS Task 正在等待通用确认输入，则消费本次输入并通知它。
	 * 不创建自定义 RPC；WaitConfirmCancel 会使用当前 Ability 的预测键同步确认到服务器。
	 */
	bool TryConsumeGenericConfirmInput();

	/**
	 * 仅由本地输入路由调用：若有 GAS Task 正在等待通用取消输入，则消费本次输入并通知它。
	 * 不创建自定义 RPC；WaitConfirmCancel 会使用当前 Ability 的预测键同步取消到服务器。
	 */
	bool TryConsumeGenericCancelInput();

	/** 在 PlayerController::PostProcessInput 中调用，处理按下、持续和释放的 Ability Spec 输入。 */
	void ProcessAbilityInput(float DeltaTime, bool bGamePaused);

	/** 仅服务器调用：授予蓝图默认值配置的启动技能，并用输入 Tag 写入对应 Ability Spec。 */
	void GrantStartupAbilities(const TArray<FSWStartupAbility>& StartupAbilities);

	/**
	 * 仅服务器调用：消耗所属 PlayerState 的一个 AbilityPoint，将指定固定技能槽位的主动技能提升一级。
	 * 服务器会在扣点前完整校验输入槽、技能类型、可升级标记、等级上限与非激活状态。
	 */
	bool TryUpgradeActiveAbilityAuthority(FGameplayTag InputTag);

	/**
	 * 仅服务器调用：将唯一 TeamId 真值镜像为一个可复制的 ASC Loose Tag。
	 * 调用后 State.Team.None/TeamA/TeamB 三者恰好保留一个；蓝图不得直接修改这些 Tag。
	 */
	void SetTeamIdTagAuthority(ESWTeamId TeamId);

	/** 仅服务器调用：写入或清理可复制的死亡状态 Tag，供死亡与重生流程统一使用。 */
	void SetDeadStateTagAuthority(bool bIsDead);

	/** 仅服务器调用：写入或清理可复制的无敌 Tag；结构推进锁定等权威规则通过此入口同步 GAS 最终门槛。 */
	void SetInvulnerableStateTagAuthority(bool bIsInvulnerable);

	bool ApplyDamageEffectToTargetAuthority(
		UAbilitySystemComponent* TargetAbilitySystemComponent,
		TSubclassOf<USWDamageGameplayEffect> DamageEffectClass,
		int32 EffectLevel,
		AActor* EffectCauser,
		const FSWDamageApplicationParams& DamageParams);

	/**
	 * 仅服务器调用：应用伤害 GE 并返回目标上的 Active GE 句柄。
	 * 持续伤害区域需要持有该句柄，才能在目标离开区域或区域销毁时精确移除效果。
	 */
	FActiveGameplayEffectHandle ApplyDamageEffectToTargetWithHandleAuthority(
		UAbilitySystemComponent* TargetAbilitySystemComponent,
		TSubclassOf<USWDamageGameplayEffect> DamageEffectClass,
		int32 EffectLevel,
		AActor* EffectCauser,
		const FSWDamageApplicationParams& DamageParams);

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
