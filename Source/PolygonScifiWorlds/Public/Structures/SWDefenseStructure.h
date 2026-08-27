// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GameFramework/Pawn.h"
#include "Interaction/SWCombatInterface.h"
#include "Interaction/SWDamageReceiverPolicyInterface.h"
#include "Interaction/SWTargetableInterface.h"
#include "Interaction/SWTeamInterface.h"
#include "Mass/Minions/SWMinionMassTypes.h"
#include "Structures/SWStructureTypes.h"
#include "Team/SWTeamTypes.h"
#include "SWDefenseStructure.generated.h"

class USceneComponent;
class USphereComponent;
class UStaticMeshComponent;
class USWAbilitySystemComponent;
class USWAttributeSet;
class USWStructureDefinition;
class USWStructureTargetingComponent;
class USWTargetHealthBarComponent;

/**
 * 防御塔与水晶共用的静态、可复制战斗实体。
 *
 * 结构自身拥有 ASC：它不经由 PlayerState 存活，也不在死亡后重生。目标选择、AI、攻击与推进规则
 * 由后续 M12 步骤接入；本类只提供稳定的生命、队伍、死亡和 GAS 生命周期边界。
 */
UCLASS(Abstract, Blueprintable)
class POLYGONSCIFIWORLDS_API ASWDefenseStructure : public APawn, public IAbilitySystemInterface, public ISWCombatInterface, public ISWDamageReceiverPolicyInterface, public ISWTargetableInterface, public ISWTeamInterface
{
	GENERATED_BODY()

public:
	ASWDefenseStructure();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//~ Begin IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	//~ End IAbilitySystemInterface

	//~ Begin ISWCombatInterface
	virtual int32 GetCombatLevel_Implementation() const override;
	virtual bool IsDead_Implementation() const override;
	virtual bool TryCommitDeathAuthority(const FSWDeathContext& DeathContext) override;
	virtual FSWOnDeath& GetOnDeathDelegate() override { return OnDeath; }
	//~ End ISWCombatInterface

	//~ Begin ISWTargetableInterface
	virtual bool IsTargetableBy(const AActor* Requestor) const override;
	//~ End ISWTargetableInterface

	//~ Begin ISWDamageReceiverPolicyInterface
	/**
	 * 仅由服务器伤害 ExecCalc 查询的结构受击规则。
	 * 本函数只返回接受/拒绝及减伤倍率，IncomingDamage 仍由 ExecCalc 唯一写入。
	 */
	virtual FSWDamageReceptionResult EvaluateDamageReceptionAuthority(const FSWDamageReceptionQuery& Query) const override;
	//~ End ISWDamageReceiverPolicyInterface

	//~ Begin ISWTeamInterface
	virtual ESWTeamId GetTeamId() const override;
	//~ End ISWTeamInterface

	/** 供表现层读取的静态结构配置；不得据此直接写入运行时生命或队伍。 */
	UFUNCTION(BlueprintPure, Category = "Structure")
	const USWStructureDefinition* GetStructureDefinition() const { return StructureDefinition; }

	/** 结构当前 ASC 使用的 AttributeSet。 */
	UFUNCTION(BlueprintPure, Category = "Structure|GAS")
	USWAttributeSet* GetAttributeSet() const { return AttributeSet; }

	/** 死亡状态的只读表现查询；服务器是唯一写入者。 */
	UFUNCTION(BlueprintPure, Category = "Structure|Combat")
	bool IsDeadCommitted() const { return bDead; }

	/** 由 Objective Subsystem 复制的推进解锁投影；只读供表现、伤害与索敌门槛使用。 */
	UFUNCTION(BlueprintPure, Category = "Structure|Objective")
	bool IsVulnerable() const { return bVulnerable; }

	/** 关卡内稳定结构身份；只用于依赖、诊断和目标事件，不使用运行时 Actor 名称。 */
	FName GetStructureId() const { return StructureId; }
	ESWStructureKind GetStructureKind() const { return StructureKind; }
	ESWLaneId GetLaneId() const { return LaneId; }
	const TArray<FName>& GetPrerequisiteStructureIds() const { return PrerequisiteStructureIds; }

	/** 供后续攻击 Ability 读取的攻击原点。 */
	UFUNCTION(BlueprintPure, Category = "Structure|Combat")
	USceneComponent* GetAttackOrigin() const { return AttackOrigin; }

	/** 结构的可见与受击 Primitive；具体 Static Mesh、材质和局部变换由蓝图子类配置。 */
	UFUNCTION(BlueprintPure, Category = "Structure|Components")
	UStaticMeshComponent* GetStructureMesh() const { return StructureMesh; }

	/**
	 * 仅本地表现的目标头顶血条锚点。它复用角色的同一组件和同一个 WBP，
	 * 可见性只在造成实际伤害的玩家客户端短暂显示。
	 */
	UFUNCTION(BlueprintPure, Category = "UI|Target Health Bar")
	USWTargetHealthBarComponent* GetTargetHealthBarComponent() const { return TargetHealthBarComponent; }

	/** 供后续 TargetingComponent 使用的服务器战斗范围组件。 */
	UFUNCTION(BlueprintPure, Category = "Structure|Combat")
	USphereComponent* GetCombatRange() const { return CombatRange; }

	/** 服务器目标选择组件；客户端不持有候选或当前目标真值。 */
	USWStructureTargetingComponent* GetTargetingComponent() const { return TargetingComponent; }

	/** 仅服务器 BT Task 调用：重验当前目标后，通过 Gameplay Event 请求结构攻击 GA。 */
	bool TryActivateStructureAttackAbilityAuthority(AActor* TargetActor);

	/** 仅服务器 Controller 在比赛停止或结构死亡时调用；只取消本结构攻击 Ability。 */
	void CancelStructureAttackAbilityAuthority();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Owner 与 Avatar 都是本结构；可在所有端安全重复调用。 */
	void InitAbilityActorInfo();

	/** 仅服务器调用：按 StructureDefinition 的 CombatantDefinition 应用启动 GE。 */
	void ApplyInitializationEffectsAuthority();

	/** 仅服务器首个塔死亡提交调用：向最后一击的敌方玩家结算此塔配置的经验与金币。 */
	void GrantDeathRewardsAuthority(const FSWDeathContext& DeathContext);

	/** 仅服务器调用：授予一次由 Definition 指定的结构攻击 Ability。 */
	void GrantAttackAbilityAuthority();

	/** 根据 Definition 更新范围预览，不注册 Overlap，也不产生权威局内状态。 */
	void RefreshCombatRangeFromDefinition();

	/** 服务器首次死亡与客户端 OnRep 均进入此处；蓝图只能实现表现。 */
	void ApplyDeathStatePresentation();

	UFUNCTION()
	void OnRep_Dead();

	UFUNCTION()
	void OnRep_Vulnerable();

	/** 死亡状态变化的纯表现入口，例如残骸、材质、音效和 Niagara。 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Structure|Combat")
	void BP_OnDeathStateChanged(bool bIsDead);

	/** 易伤状态变化的纯表现入口，例如护盾、材质和地图提示。 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Structure|Objective")
	void BP_OnVulnerableStateChanged(bool bIsNowVulnerable);

	/** 蓝图子类选择的静态结构配置；局内状态不放入 Data Asset。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Structure", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USWStructureDefinition> StructureDefinition;

	/** 关卡内稳定唯一 ID；不得使用 Actor 名称替代。 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Structure|Objective", meta = (AllowPrivateAccess = "true"))
	FName StructureId;

	/** 塔与水晶共用一个类，但死亡后的目标后果由该身份在后续步骤路由。 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Structure|Objective", meta = (AllowPrivateAccess = "true"))
	ESWStructureKind StructureKind = ESWStructureKind::None;

	/** Tower 必须属于 Top/Middle/Bottom；Crystal 必须为 None。 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Structure|Objective", meta = (AllowPrivateAccess = "true"))
	ESWLaneId LaneId = ESWLaneId::None;

	/** 防御塔须全部前置被摧毁；水晶任一前置被摧毁即可进入 Vulnerable。 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Structure|Objective", meta = (AllowPrivateAccess = "true"))
	TArray<FName> PrerequisiteStructureIds;

	/** 关卡中结构实例的队伍真值；后续 Objective Subsystem 只读取，不改写。 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Replicated, Category = "Structure|Team", meta = (AllowPrivateAccess = "true"))
	ESWTeamId TeamId = ESWTeamId::None;

	/** 唯一死亡真值；结构死亡后保留 Actor 供晚加入与残骸表现同步。 */
	UPROPERTY(ReplicatedUsing = OnRep_Dead, BlueprintReadOnly, Category = "Structure|Combat", meta = (AllowPrivateAccess = "true"))
	bool bDead = false;

	/** 由 Objective Subsystem 唯一写入的推进解锁投影；初始默认锁定。 */
	UPROPERTY(ReplicatedUsing = OnRep_Vulnerable, BlueprintReadOnly, Category = "Structure|Objective", meta = (AllowPrivateAccess = "true"))
	bool bVulnerable = false;

	/** 静态结构的攻击参考点；蓝图可将其调整至炮口或水晶发射位置。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Structure|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> AttackOrigin;

	/**
	 * 所有结构共用的视觉与受击 Primitive。C++ 固定组件身份和默认阻挡契约；
	 * 蓝图子类只配置实际 Mesh、材质、缩放与需要的 Socket。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Structure|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> StructureMesh;

	/**
	 * 目标头顶血条的世界锚点。WidgetClass、尺寸和相对于每种塔模型的高度由蓝图子类配置；
	 * 它不复制任何 UI 可见性或生命真值。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI|Target Health Bar", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USWTargetHealthBarComponent> TargetHealthBarComponent;

	/** 仅服务器 QueryOnly 的候选范围；具体 Profile 在蓝图/项目碰撞配置阶段设置。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Structure|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> CombatRange;

	/** 仅服务器维护的范围候选与稳定目标选择；不复制给客户端。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Structure|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USWStructureTargetingComponent> TargetingComponent;

	/** 结构自行拥有 ASC，使用 Minimal 复制模式。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Structure|GAS", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USWAbilitySystemComponent> AbilitySystemComponent;

	/** 结构的生命、护甲等运行时属性，由 ASC 聚合并复制。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Structure|GAS", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USWAttributeSet> AttributeSet;

	/** 仅服务器广播；M12 Objective Subsystem 将在后续订阅并消费。 */
	FSWOnDeath OnDeath;

private:
	friend class USWStructureObjectiveSubsystem;

	/** 仅供 Objective Subsystem 调用：同步复制投影与 ASC 的 State.Invulnerable 最终伤害门槛。 */
	void SetVulnerableAuthority(bool bNewVulnerable);

	/** 防止服务器生命周期重入时重复施加启动 GE。 */
	bool bInitializationEffectsApplied = false;

	/** 结构攻击 Spec 只存在服务器；BT 不保存冷却或攻击次数。 */
	FGameplayAbilitySpecHandle StructureAttackAbilityHandle;
};
