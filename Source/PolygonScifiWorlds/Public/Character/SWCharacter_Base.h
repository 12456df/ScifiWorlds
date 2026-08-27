// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "Interaction/SWCombatInterface.h"
#include "Interaction/SWTeamInterface.h"
#include "SWCharacter_Base.generated.h"

class UAbilitySystemComponent;
class USWAttributeSet;
class USWCombatantDefinition;
class USWTargetHealthBarComponent;

/**
 * ScifiWorlds 所有角色的抽象基类。
 *
 * 统一提供 ASC 与 AttributeSet 的查询入口（IAbilitySystemInterface），但不规定它们的归属：
 *   - 玩家角色（ASWCharacter_Player）的 ASC 位于 ASWPlayerState，Character 仅作 Avatar；
 *   - AI 角色（ASWCharacter_Enemy）自身持有 ASC。
 * 具体的 Owner/Avatar 绑定由子类覆写 InitAbilityActorInfo 完成。
 */
UCLASS(Abstract)
class POLYGONSCIFIWORLDS_API ASWCharacter_Base : public ACharacter, public IAbilitySystemInterface, public ISWCombatInterface, public ISWTeamInterface
{
	GENERATED_BODY()

public:
	ASWCharacter_Base(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//~ Begin IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	//~ End IAbilitySystemInterface

	virtual int32 GetCombatLevel_Implementation() const override;
	virtual bool IsDead_Implementation() const override;
	virtual ESWTeamId GetTeamId() const override;
	virtual bool TryCommitDeathAuthority(const FSWDeathContext& DeathContext) override;
	virtual FSWOnDeath& GetOnDeathDelegate() override { return OnDeath; }

	USWAttributeSet* GetAttributeSet() const { return AttributeSet; }

	/** 返回该角色类的静态战斗配置；运行时状态仍由 ASC、PlayerState 或 Character 自身持有。 */
	UFUNCTION(BlueprintPure, Category = "Combat")
	const USWCombatantDefinition* GetCombatantDefinition() const { return CombatantDefinition; }

	/** 死亡是否已由服务器提交；用于动画、UI 等只读表现查询。 */
	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsDeadCommitted() const { return bDead; }

	/**
	 * 所有角色共用的头顶血条锚点。它仅在实际攻击者本地显示，
	 * 不复制可见性，也不属于生命值或战斗规则真值。
	 */
	UFUNCTION(BlueprintPure, Category = "UI|Target Health Bar")
	USWTargetHealthBarComponent* GetTargetHealthBarComponent() const { return TargetHealthBarComponent; }

	/** 仅服务器调用：对新生成的 Pawn 应用其战斗配置指定的重生无敌 GE。 */
	void ApplyRespawnInvulnerabilityEffectAuthority();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 完成 ASC 的 Owner/Avatar 绑定。基类为空，由玩家/AI 子类分别实现。可重复调用。 */
	virtual void InitAbilityActorInfo();

	/** 仅服务器调用：在 ASC 完成 Owner/Avatar 绑定后应用该角色的初始化 GE。 */
	void ApplyCombatantInitializationEffectsAuthority(int32 EffectLevel, bool bRestoreVitalResources);

	/**
	 * 仅服务器调用：将当前资源恢复到 ASC 聚合后的最终上限。
	 * 必须在等级、装备等会修改最大资源的常驻 GE 均已收敛后调用；不重新应用任何 GE。
	 */
	void RestoreVitalResourcesToMaximumAuthority();

	/** 复制到客户端及服务器首次提交时调用；蓝图在此实现布娃娃等纯表现。 */
	void ApplyDeathStatePresentation();

	UFUNCTION()
	void OnRep_Dead();

	/** 死亡状态改变时的纯表现事件；不得在此决定死亡、发奖或重生。 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Combat|Death")
	void BP_OnDeathStateChanged(bool bIsDead);

	/** 由角色蓝图选择的静态战斗配置；蓝图不直接执行其中的权威初始化逻辑。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USWCombatantDefinition> CombatantDefinition;

	/** 唯一死亡真值：仅服务器写入，并复制给所有客户端与晚加入者。 */
	UPROPERTY(ReplicatedUsing = OnRep_Dead, BlueprintReadOnly, Category = "Combat|Death", meta = (AllowPrivateAccess = "true"))
	bool bDead = false;

	// 由子类填充：玩家从 PlayerState 缓存，AI 在构造时自建。
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<USWAttributeSet> AttributeSet;

	/** 头顶血条的世界空间锚点；具体 WidgetClass 与外观由 Character 蓝图配置。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI|Target Health Bar", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USWTargetHealthBarComponent> TargetHealthBarComponent;

	/** 仅服务器广播；死亡后的经验、比分和重生由后续协调层订阅。 */
	FSWOnDeath OnDeath;

private:
	/** 仅服务器调用：死亡唯一提交成功后，按死亡者配置向合法敌方击杀者结算经验与金币。 */
	void GrantDeathRewardsAuthority(const FSWDeathContext& DeathContext);
};
