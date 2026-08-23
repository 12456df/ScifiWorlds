// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Lane/SWLaneRoute.h"
#include "MassEntityHandle.h"
#include "Mass/Minions/SWMinionMassTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimerManager.h"
#include "SWMinionLaneWaveSubsystem.generated.h"

class USWMinionEntityFactory;
class USWMinionWaveData;
class UMassProcessor;

/** M11-6 运行期诊断快照；仅用于服务器日志，不复制也不作为玩法真值。 */
struct FSWMinionRuntimeDiagnostics
{
	int32 TrackedEntityCount = 0;
	int32 ValidEntityCount = 0;
	int32 ActorBridgeCount = 0;
	int32 DeadActorCount = 0;
	int32 WorldMinionActorCount = 0;
	int32 RegisteredTargetCount = 0;
	/** StateTree 写入的行为意图统计；用于定位行为树是否实际进入首个状态。 */
	int32 IntentNoneCount = 0;
	int32 IntentAdvancingCount = 0;
	int32 IntentEngagingCount = 0;
	int32 IntentAttackingCount = 0;
	int32 IntentReturningCount = 0;
	/** DesiredVelocity 非零代表沿线移动 Processor 至少在本帧以前成功处理过该 Entity。 */
	int32 MovingIntentCount = 0;
	/** 路线身份不完整的 Entity 数量；正常运行时应为零。 */
	int32 InvalidLaneStateCount = 0;
	/** Mass Transform 与桥接 Actor 位置明显不一致的数量，用于定位同步 Processor。 */
	int32 ActorTransformMismatchCount = 0;
	int32 CumulativeSpawnedCount = 0;
	int32 CumulativeCleanedUpCount = 0;
	int32 LastWaveIndex = INDEX_NONE;
	bool bWavesRunning = false;
};

/**
 * 服务器权威的三路小兵波次协调器。
 * 它仅负责比赛阶段、路线发现、波次计时与 Factory 调用；不保存战斗状态、不移动 Entity，也不创建 AIController。
 */
UCLASS()
class POLYGONSCIFIWORLDS_API USWMinionLaneWaveSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** 由 ASWGameMode 在比赛进入 InProgress 时调用；同一局只能使用一份 WaveData 启动一次。 */
	bool StartWavesAuthority(USWMinionWaveData* InWaveData);

	/** 由 ASWGameMode 在比赛结束或 World 退出时调用；仅停止后续波次，不越权销毁 M10-5 尚未桥接的 Entity。 */
	void StopWavesAuthority();

	/**
	 * 由 ASWGameMode 在比赛结束时调用：冻结现存小兵的移动、索敌与攻击意图，并取消已激活的攻击 Ability。
	 * 不销毁 Entity，赛后世界仍保留最终战场画面，后续 Restart Travel 统一重置它们。
	 */
	void StopActiveMinionBehaviorAuthority();

	/** 当前 World 中仍有效的、由本波次系统创建的小兵 Entity 数量。 */
	int32 GetActiveMinionEntityCount() const;

	/** 仅 M11 死亡回收调用：在 Entity 延迟销毁前立即移除活动计数，避免硬上限被尸体占用。 */
	void RemoveActiveMinionEntityAuthority(FMassEntityHandle EntityHandle);

	/** 仅服务器诊断：汇总 Mass/Actor Bridge、死亡待回收与 Target Registry 的当前状态。 */
	FSWMinionRuntimeDiagnostics GetRuntimeDiagnosticsAuthority();

	/** 已成功生成的最新波次编号；尚未生成任何波次时为 INDEX_NONE。 */
	int32 GetLastSpawnedWaveIndex() const { return LastSpawnedWaveIndex; }

	/** M11 Movement Processor 的只读路线查询；不会访问关卡 Spline 或修改波次状态。 */
	bool TrySampleLaneTransform(ESWLaneId LaneId, float DistanceAlongLane, ESWLaneDirection Direction, FTransform& OutTransform) const;
	bool TryProjectLaneDistance(ESWLaneId LaneId, const FVector& WorldLocation, float& OutDistanceAlongLane) const;
	bool TryGetLaneLength(ESWLaneId LaneId, float& OutLength) const;

	/** 路线快照异常按 Lane 聚合一次，避免每个 Entity 每帧刷屏。 */
	void ReportLaneSamplingFailureOnce(ESWLaneId LaneId);

	virtual void Deinitialize() override;

private:
	bool IsAuthorityWorld() const;
	/** 将项目自定义的 M11 Processor 显式注册到当前 World 的 Mass Simulation，避免依赖启动期全局发现。 */
	bool EnsureRuntimeProcessorsRegisteredAuthority();
	/** 仅 World 生命周期结束时撤销本系统创建的动态 Processor。 */
	void UnregisterRuntimeProcessors();
	bool CacheLaneRoutesAuthority();
	bool ValidateWaveData(const USWMinionWaveData& InWaveData, FString& OutFailure) const;
	/** 全局波次 Timer 的无返回值回调；Timer 不直接绑定带返回值的生成函数。 */
	void HandleWaveTimerElapsed();
	bool SpawnNextWaveAuthority();
	bool IsMatchInProgress() const;
	void PruneInvalidActiveEntities();

	/** 一局只使用一份静态波次资产；由服务器 GameMode 提供。 */
	UPROPERTY(Transient)
	TObjectPtr<USWMinionWaveData> WaveData;

	/** Factory 由本 World Subsystem 拥有，其 Outer 保证能解析到正确 World。 */
	UPROPERTY(Transient)
	TObjectPtr<USWMinionEntityFactory> EntityFactory;

	/** 本 World 独占的动态 Processor；由 UMassSimulationSubsystem 持有运行调度，本系统持有 GC 生命周期。 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMassProcessor>> RuntimeProcessors;

	/** 仅保存 M10-4 创建的 Handle；未来死亡桥负责销毁 Entity，本系统只在生成前修剪失效项。 */
	TArray<FMassEntityHandle> ActiveMinionEntities;

	/** 关卡中按 LaneId 缓存的三条有效路线；只在比赛开始时建立一次。 */
	TMap<uint8, TWeakObjectPtr<ASWLaneRoute>> LaneRoutes;

	/** 从 Lane Actor 冻结的纯值快照，运行时移动不读取 Spline UObject。 */
	TMap<uint8, FSWLaneRouteSnapshot> LaneRouteSnapshots;
	TSet<uint8> ReportedLaneSamplingFailures;

	/** 下一次待生成的波次序号；首波为 0。 */
	int32 NextWaveIndex = 0;
	int32 LastSpawnedWaveIndex = INDEX_NONE;
	int32 CumulativeSpawnedMinionCount = 0;
	int32 CumulativeCleanedUpMinionCount = 0;
	bool bWavesRunning = false;
	FTimerHandle WaveTimer;
};
