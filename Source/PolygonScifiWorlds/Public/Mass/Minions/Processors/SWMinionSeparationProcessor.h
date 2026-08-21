// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "SWMinionSeparationProcessor.generated.h"

struct FMassEntityManager;
struct FMassExecutionContext;

/**
 * 仅服务器：在 Movement 完成后，以 Capsule 半径为准修正 Mass Transform。
 * 同队同路保留少量个人空间，任何两名重叠小兵都至少保持实际 Capsule 半径之和；
 * 这不是 Chaos 物理，也不使用每个 Actor 的 Sweep/碰撞回调。
 */
UCLASS()
class POLYGONSCIFIWORLDS_API USWMinionSeparationProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	USWMinionSeparationProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EntityQuery;
};
