// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SWStructureObjectiveSubsystem.generated.h"

class ASWDefenseStructure;
struct FSWDeathContext;

/**
 * 服务器 World 内结构前置关系的唯一权威拥有者。
 *
 * 它维护关卡预放置结构的依赖 DAG 与已毁 ID 集合，并且是唯一可写 bVulnerable 的对象。
 * 该 Subsystem 不复制：客户端通过结构自身复制的 bVulnerable 和 GAS Tag 获取表现所需状态。
 */
UCLASS()
class POLYGONSCIFIWORLDS_API USWStructureObjectiveSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	/**
	 * 仅服务器结构 BeginPlay 调用。注册成功后在下一服务器 Tick 统一验证全图，避免依赖 Actor BeginPlay 顺序。
	 * 重复注册同一实例幂等；相同 StructureId 的不同实例会使 DAG 无效。
	 */
	bool RegisterStructure(ASWDefenseStructure& Structure);

	/** 仅服务器 EndPlay 调用；移除死亡订阅与本地索引，不主动改写其他结构状态。 */
	void UnregisterStructure(ASWDefenseStructure& Structure);

	/** 仅服务器诊断查询；图无效时所有结构维持锁定且无敌。 */
	bool IsGraphValidAuthority() const { return bGraphValid; }
	const FString& GetLastValidationErrorAuthority() const { return LastValidationError; }

	/**
	 * 仅服务器/Standalone 开发诊断：向日志输出当前地图全部已注册结构的身份与可受击状态。
	 * 不修改结构、属性、目标或推进状态。
	 */
	void LogDiagnosticsAuthority() const;

private:
	struct FRegisteredStructure
	{
		TWeakObjectPtr<ASWDefenseStructure> Structure;
		FDelegateHandle DeathDelegateHandle;
	};

	bool IsAuthorityWorld() const;
	void ScheduleGraphRebuildAuthority();
	void RebuildGraphAuthority();
	bool ValidateGraphAuthority();
	void ApplyVulnerabilityStatesAuthority();
	void HandleRegisteredStructureDeath(const FSWDeathContext& DeathContext, TWeakObjectPtr<ASWDefenseStructure> Structure);
	void RecordValidationError(const FString& Error);
	void RecordRegistrationError(const FString& Error);

	/** 以稳定 StructureId 建立的唯一索引；Actor 保持弱引用，不阻止关卡卸载。 */
	TMap<FName, FRegisteredStructure> StructuresById;

	/** 前置 ID 到其直接后继 ID 的索引；当前用于验证与后续死亡后的局部更新。 */
	TMap<FName, TArray<FName>> DependentsByPrerequisiteId;

	/** 已被服务器首次消费死亡事件的结构 ID；每个 ID 至多触发一次推进更新。 */
	TSet<FName> DestroyedStructureIds;

	FTimerHandle GraphRebuildTimer;
	bool bGraphRebuildScheduled = false;
	bool bGraphValid = false;
	FString RegistrationValidationError;
	FString LastValidationError;
};
