// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "Mass/Minions/SWMinionMassTypes.h"
#include "SWMinionSpawnInitializerProcessor.generated.h"

struct FMassEntityManager;
struct FMassExecutionContext;

/** Factory 提交给 MassSpawner 的单个小兵出生数据。 */
USTRUCT()
struct FSWMinionSpawnEntry
{
	GENERATED_BODY()

	FTransform SpawnTransform = FTransform::Identity;
	FName UnitId = NAME_None;
	ESWTeamId TeamId = ESWTeamId::None;
	ESWLaneId LaneId = ESWLaneId::None;
	ESWLaneDirection LaneDirection = ESWLaneDirection::None;
	float DistanceAlongLane = 0.f;
	float LateralOffset = 0.f;
	float VerticalOffset = 0.f;
	int32 WaveIndex = INDEX_NONE;
	int32 SpawnOrdinal = INDEX_NONE;
	float AttackRange = 0.f;
};

/** Factory 一次批量生成所携带的按 Entity 顺序排列的初始化数据。 */
USTRUCT()
struct FSWMinionSpawnData
{
	GENERATED_BODY()

	TArray<FSWMinionSpawnEntry> Entries;
};

/**
 * MassSpawner 专用初始化 Processor。
 * 它只在 SpawnEntities 调用期间执行一次，将 Factory 的出生快照写入已有 Fragment；不会自动注册到每帧处理阶段。
 */
UCLASS()
class POLYGONSCIFIWORLDS_API USWMinionSpawnInitializerProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	USWMinionSpawnInitializerProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EntityQuery;
};
