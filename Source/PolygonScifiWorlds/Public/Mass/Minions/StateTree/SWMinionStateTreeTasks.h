// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Mass/Minions/SWMinionMassTypes.h"
#include "MassStateTreeTypes.h"
#include "SWMinionStateTreeTasks.generated.h"

struct FMassExecutionContext;
struct FMassEntityHandle;

/** Set Behavior Task 的每实例可编辑数据；StateTree 5.7 要求 Task 显式声明实例数据类型。 */
USTRUCT()
struct FSWMinionSetBehaviorTaskInstanceData
{
	GENERATED_BODY()

	/** StateTree 中选择的行为；首个资产默认 Advancing。 */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	ESWMinionBehaviorIntent Behavior = ESWMinionBehaviorIntent::Advancing;
};

/** 只读 Condition 的空实例数据；保留该类型以满足 StateTree 节点实例化契约。 */
USTRUCT()
struct FSWMinionTargetConditionInstanceData
{
	GENERATED_BODY()
};

/**
 * StateTree 进入一个小兵行为状态时，原子地发布行为意图。
 * 本 Task 不执行移动、索敌、攻击或伤害；这些工作必须由后续服务器 Processor 完成。
 */
USTRUCT(meta = (DisplayName = "SW Set Minion Behavior"))
struct POLYGONSCIFIWORLDS_API FSWMinionSetBehaviorTask : public FMassStateTreeTaskBase
{
	GENERATED_BODY()
	using FInstanceDataType = FSWMinionSetBehaviorTaskInstanceData;

	FSWMinionSetBehaviorTask()
	{
		// 行为切换只由 Signal 驱动的状态进入处理，不注册任何逐帧 Task Tick。
		bShouldStateChangeOnReselect = false;
		bShouldCallTick = false;
		bShouldCallTickOnlyOnEvents = false;
		bConsideredForScheduling = false;
	}

	// 与引擎基类保持相同的 public 可见性，避免缩窄 StateTree 的虚函数接口。
	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual void GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const override;
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;


	TStateTreeExternalDataHandle<FSWMinionIntentFragment> IntentHandle;
	TStateTreeExternalDataHandle<FSWMinionLeashFragment> LeashHandle;

};

/** 只读检查 Targeting Processor 的结果，供 StateTree 在 Target Signal 到达时选择 Engaging/Returning。 */
USTRUCT(meta = (DisplayName = "SW Minion Has Valid Target"))
struct POLYGONSCIFIWORLDS_API FSWMinionHasValidTargetCondition : public FMassStateTreeConditionBase
{
	GENERATED_BODY()
	using FInstanceDataType = FSWMinionTargetConditionInstanceData;

	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual void GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const override;
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

private:
	TStateTreeExternalDataHandle<FSWMinionTargetFragment> TargetHandle;
};

/** 只读检查没有有效目标，避免依赖编辑器版本差异较大的反转条件界面。 */
USTRUCT(meta = (DisplayName = "SW Minion Has No Valid Target"))
struct POLYGONSCIFIWORLDS_API FSWMinionHasNoValidTargetCondition : public FMassStateTreeConditionBase
{
	GENERATED_BODY()
	using FInstanceDataType = FSWMinionTargetConditionInstanceData;

	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual void GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const override;
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

private:
	TStateTreeExternalDataHandle<FSWMinionTargetFragment> TargetHandle;
};

/** 只读检查当前有效目标是否在同一攻击距离快照内，供 StateTree 进入 Attacking。 */
USTRUCT(meta = (DisplayName = "SW Minion Target In Attack Range"))
struct POLYGONSCIFIWORLDS_API FSWMinionTargetInAttackRangeCondition : public FMassStateTreeConditionBase
{
	GENERATED_BODY()
	using FInstanceDataType = FSWMinionTargetConditionInstanceData;

	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual void GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const override;
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

private:
	TStateTreeExternalDataHandle<FSWMinionTargetFragment> TargetHandle;
};

/** 只读检查当前仍有合法目标但已离开攻击距离，供 Attacking 返回 Engaging。 */
USTRUCT(meta = (DisplayName = "SW Minion Target Outside Attack Range"))
struct POLYGONSCIFIWORLDS_API FSWMinionTargetOutsideAttackRangeCondition : public FMassStateTreeConditionBase
{
	GENERATED_BODY()
	using FInstanceDataType = FSWMinionTargetConditionInstanceData;

	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual void GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const override;
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

private:
	TStateTreeExternalDataHandle<FSWMinionTargetFragment> TargetHandle;
};

/** 只读检查 Returning 是否已回到保存的路线锚点，避免以“目标无效”作为错误的回线完成条件。 */
USTRUCT(meta = (DisplayName = "SW Minion Reached Leash Anchor"))
struct POLYGONSCIFIWORLDS_API FSWMinionReachedLeashAnchorCondition : public FMassStateTreeConditionBase
{
	GENERATED_BODY()
	using FInstanceDataType = FSWMinionTargetConditionInstanceData;

	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual void GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const override;
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

private:
	TStateTreeExternalDataHandle<FSWMinionIntentFragment> IntentHandle;
};
