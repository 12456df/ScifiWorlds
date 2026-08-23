// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "SWBTTask_ActivateStructureAttackAbility.generated.h"

/**
 * 将 Blackboard 中的当前目标转交给结构攻击 Ability。
 * Task 不保存攻击间隔或伤害；成功只表示服务器已接受一次 GA 请求，冷却和命中时序由 GA 管理。
 */
UCLASS()
class POLYGONSCIFIWORLDS_API USWBTTask_ActivateStructureAttackAbility : public UBTTaskNode
{
	GENERATED_BODY()

public:
	USWBTTask_ActivateStructureAttackAbility();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	/** 必须绑定到与 Controller 契约同名的 Actor Key，默认 TargetActor。 */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetActorKey;
};
