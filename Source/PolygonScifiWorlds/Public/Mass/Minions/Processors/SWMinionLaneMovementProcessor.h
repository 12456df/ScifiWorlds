// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "SWMinionLaneMovementProcessor.generated.h"

struct FMassEntityManager;
struct FMassExecutionContext;

/**
 * 仅服务器：消费 StateTree 的 Advancing 意图，沿冻结的兵线路线批量推进 Mass Transform。
 * 不访问 Actor、ASC、NavMesh 或 Spline UObject；这不是 Actor Tick，而是 Mass 的分块模拟步骤。
 */
UCLASS()
class POLYGONSCIFIWORLDS_API USWMinionLaneMovementProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	USWMinionLaneMovementProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EntityQuery;
};
