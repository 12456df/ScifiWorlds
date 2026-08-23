// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "SWStructureAIController.generated.h"

class AActor;
class ASWDefenseStructure;
class ASWGameState;

/**
 * 仅服务器存在的防御结构 AI 桥接层。
 * 它只把 StructureTargetingComponent 和 GameState 的权威只读状态投影到 Blackboard；
 * 不选择目标、不保存冷却、不结算伤害，也不向客户端复制 AI/Blackboard。
 */
UCLASS()
class POLYGONSCIFIWORLDS_API ASWStructureAIController : public AAIController
{
	GENERATED_BODY()

public:
	ASWStructureAIController();

	/** 仅由结构 BT Task 调用：请求已授予的攻击 GA，不直接造成伤害。 */
	bool TryActivateStructureAttackAbilityAuthority(AActor* TargetActor);

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

private:
	void HandleTargetChanged(TWeakObjectPtr<AActor> NewTarget);
	void HandleMatchStateChanged(FName NewMatchState);
	void HandleStructureDeath(const struct FSWDeathContext& DeathContext);
	void StartBehaviorTreeAuthority();
	void StopBehaviorTreeAuthority(const FString& Reason);
	void UpdateBlackboardTargetAuthority(AActor* NewTarget);
	void SetCombatEnabledAuthority(bool bEnabled);
	void UnbindAuthorityDelegates();

	TWeakObjectPtr<ASWDefenseStructure> StructureOwner;
	TWeakObjectPtr<ASWGameState> ObservedGameState;
	FDelegateHandle TargetChangedDelegateHandle;
	FDelegateHandle MatchStateChangedDelegateHandle;
	FDelegateHandle StructureDeathDelegateHandle;
	bool bBehaviorTreeRunning = false;

	static const FName TargetActorBlackboardKey;
	static const FName CombatEnabledBlackboardKey;
};
