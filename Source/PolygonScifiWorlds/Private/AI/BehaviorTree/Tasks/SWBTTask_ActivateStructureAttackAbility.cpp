// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "AI/BehaviorTree/Tasks/SWBTTask_ActivateStructureAttackAbility.h"

#include "AI/Structures/SWStructureAIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Actor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SWBTTask_ActivateStructureAttackAbility)

USWBTTask_ActivateStructureAttackAbility::USWBTTask_ActivateStructureAttackAbility()
{
	NodeName = TEXT("SW Activate Structure Attack Ability");
	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, TargetActorKey), AActor::StaticClass());
	TargetActorKey.SelectedKeyName = TEXT("TargetActor");
}

EBTNodeResult::Type USWBTTask_ActivateStructureAttackAbility::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	static_cast<void>(NodeMemory);
	ASWStructureAIController* const StructureController = Cast<ASWStructureAIController>(OwnerComp.GetAIOwner());
	UBlackboardComponent* const Blackboard = OwnerComp.GetBlackboardComponent();
	AActor* const TargetActor = Blackboard ? Cast<AActor>(Blackboard->GetValueAsObject(TargetActorKey.SelectedKeyName)) : nullptr;
	return StructureController && TargetActor && StructureController->TryActivateStructureAttackAbilityAuthority(TargetActor)
		? EBTNodeResult::Succeeded
		: EBTNodeResult::Failed;
}
