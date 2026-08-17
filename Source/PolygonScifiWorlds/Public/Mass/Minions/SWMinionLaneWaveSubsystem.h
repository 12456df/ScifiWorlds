// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "MassEntityHandle.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimerManager.h"
#include "SWMinionLaneWaveSubsystem.generated.h"

class ASWLaneRoute;
class USWMinionEntityFactory;
class USWMinionWaveData;

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

	/** 当前 World 中仍有效的、由本波次系统创建的小兵 Entity 数量。 */
	int32 GetActiveMinionEntityCount() const;

	/** 已成功生成的最新波次编号；尚未生成任何波次时为 INDEX_NONE。 */
	int32 GetLastSpawnedWaveIndex() const { return LastSpawnedWaveIndex; }

	virtual void Deinitialize() override;

private:
	bool IsAuthorityWorld() const;
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

	/** 仅保存 M10-4 创建的 Handle；未来死亡桥负责销毁 Entity，本系统只在生成前修剪失效项。 */
	TArray<FMassEntityHandle> ActiveMinionEntities;

	/** 关卡中按 LaneId 缓存的三条有效路线；只在比赛开始时建立一次。 */
	TMap<uint8, TWeakObjectPtr<ASWLaneRoute>> LaneRoutes;

	/** 下一次待生成的波次序号；首波为 0。 */
	int32 NextWaveIndex = 0;
	int32 LastSpawnedWaveIndex = INDEX_NONE;
	bool bWavesRunning = false;
	FTimerHandle WaveTimer;
};
