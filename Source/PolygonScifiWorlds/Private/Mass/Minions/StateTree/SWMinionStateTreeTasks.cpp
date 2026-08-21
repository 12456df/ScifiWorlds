// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "Mass/Minions/StateTree/SWMinionStateTreeTasks.h"

#include "MassStateTreeDependency.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SWMinionStateTreeTasks)

bool FSWMinionSetBehaviorTask::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(IntentHandle);
	Linker.LinkExternalData(LeashHandle);
	return true;
}

void FSWMinionSetBehaviorTask::GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const
{
	Builder.AddReadWrite<FSWMinionIntentFragment>();
	Builder.AddReadWrite<FSWMinionLeashFragment>();
}

EStateTreeRunStatus FSWMinionSetBehaviorTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// 行为意图是 StateTree 的唯一同步输出，供纯数据 Processor 直接消费。
	// 不为每次行为切换增删 Tag，避免无收益的 Archetype 迁移和延迟组成变更。
	const FSWMinionSetBehaviorTaskInstanceData& InstanceData = Context.GetInstanceData(*this);
	FSWMinionIntentFragment& Intent = Context.GetExternalData(IntentHandle);
	FSWMinionLeashFragment& Leash = Context.GetExternalData(LeashHandle);
	Intent.Behavior = InstanceData.Behavior;
	Intent.DesiredVelocity = FVector::ZeroVector;
	Intent.bReachedReturnAnchor = false;
	// 每次进入 Attacking 仅发出一次请求，Attack Processor 消费后清零；不会变成每帧激活 GA。
	Intent.bAttackRequested = InstanceData.Behavior == ESWMinionBehaviorIntent::Attacking;
	if (InstanceData.Behavior == ESWMinionBehaviorIntent::Returning)
	{
		// 无论是超出 Leash、Target 死亡还是失效，均从当前世界位置向前归线。
		Leash.bNeedsForwardLaneRejoinProjection = true;
	}

	return EStateTreeRunStatus::Running;
}

bool FSWMinionHasValidTargetCondition::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(TargetHandle);
	return true;
}

void FSWMinionHasValidTargetCondition::GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const
{
	Builder.AddReadOnly<FSWMinionTargetFragment>();
}

bool FSWMinionHasValidTargetCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FSWMinionTargetFragment& Target = Context.GetExternalData(TargetHandle);
	return Target.TargetId != 0 && Target.TargetActor.IsValid();
}

bool FSWMinionHasNoValidTargetCondition::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(TargetHandle);
	return true;
}

void FSWMinionHasNoValidTargetCondition::GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const
{
	Builder.AddReadOnly<FSWMinionTargetFragment>();
}

bool FSWMinionHasNoValidTargetCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FSWMinionTargetFragment& Target = Context.GetExternalData(TargetHandle);
	return Target.TargetId == 0 || !Target.TargetActor.IsValid();
}

bool FSWMinionTargetInAttackRangeCondition::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(TargetHandle);
	return true;
}

void FSWMinionTargetInAttackRangeCondition::GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const
{
	Builder.AddReadOnly<FSWMinionTargetFragment>();
}

bool FSWMinionTargetInAttackRangeCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FSWMinionTargetFragment& Target = Context.GetExternalData(TargetHandle);
	return Target.TargetId != 0 && Target.TargetActor.IsValid() && Target.bIsWithinAttackRange;
}

bool FSWMinionTargetOutsideAttackRangeCondition::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(TargetHandle);
	return true;
}

void FSWMinionTargetOutsideAttackRangeCondition::GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const
{
	Builder.AddReadOnly<FSWMinionTargetFragment>();
}

bool FSWMinionTargetOutsideAttackRangeCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FSWMinionTargetFragment& Target = Context.GetExternalData(TargetHandle);
	return Target.TargetId != 0 && Target.TargetActor.IsValid() && !Target.bIsWithinAttackRange;
}

bool FSWMinionReachedLeashAnchorCondition::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(IntentHandle);
	return true;
}

void FSWMinionReachedLeashAnchorCondition::GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const
{
	Builder.AddReadOnly<FSWMinionIntentFragment>();
}

bool FSWMinionReachedLeashAnchorCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
	return Context.GetExternalData(IntentHandle).bReachedReturnAnchor;
}
