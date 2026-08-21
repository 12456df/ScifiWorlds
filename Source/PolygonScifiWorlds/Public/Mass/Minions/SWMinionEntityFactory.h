// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "MassEntityHandle.h"
#include "UObject/Object.h"
#include "Mass/Minions/SWMinionMassTypes.h"
#include "SWMinionEntityFactory.generated.h"

class ASWLaneRoute;
class USWMinionDefinition;

/** Wave Subsystem 向 Factory 提交的一次同类型、同队伍、同路线小兵批次请求。 */
USTRUCT()
struct FSWMinionSpawnBatchRequest
{
	GENERATED_BODY()

	const ASWLaneRoute* LaneRoute = nullptr;
	const USWMinionDefinition* MinionDefinition = nullptr;
	ESWTeamId TeamId = ESWTeamId::None;
	int32 WaveIndex = INDEX_NONE;
	int32 FirstSpawnOrdinal = 0;
	TArray<FTransform> SpawnTransforms;
	/** 与 SpawnTransforms 一一对应的、相对队伍出生 Transform 的局部编队偏移。 */
	TArray<FVector> FormationOffsets;
};

/** Factory 成功创建并完成 Actor/ASC Bridge 的 Entity Handle。 */
struct FSWMinionSpawnBatchResult
{
	TArray<FMassEntityHandle> SpawnedEntities;
	FString FailureReason;

	bool IsSuccess() const { return FailureReason.IsEmpty() && !SpawnedEntities.IsEmpty(); }
};

/**
 * 服务器权威的小兵 Entity 创建入口。
 * 本对象不保存波次、路线或活动 Entity；M10-4 Wave Subsystem 持有它，M10-5 扩展 Actor/ASC Bridge。
 */
UCLASS()
class POLYGONSCIFIWORLDS_API USWMinionEntityFactory : public UObject
{
	GENERATED_BODY()

public:
	/** 仅服务器/Standalone：原子创建一批同 Archetype 的小兵 Entity、Actor 与 ASC Bridge。 */
	bool SpawnBatchAuthority(const FSWMinionSpawnBatchRequest& Request, FSWMinionSpawnBatchResult& OutResult);

	/**
	 * 仅服务器/Standalone：销毁一批已桥接的 Entity 与其对应 Actor。
	 * 用于波次创建失败回滚；正常死亡由 M11-5 的死亡桥与 Cleanup Processor 处理。
	 */
	void DestroyBatchAuthority(TConstArrayView<FMassEntityHandle> EntityHandles);

private:
	bool IsAuthorityWorld() const;
	bool ValidateRequest(const FSWMinionSpawnBatchRequest& Request, FString& OutFailure) const;
	bool CreateActorBridgeAuthority(const FSWMinionSpawnBatchRequest& Request, TConstArrayView<FMassEntityHandle> EntityHandles, FString& OutFailure);
};
