// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "MassEntityQuery.h"
#include "MassObserverProcessor.h"
#include "SWMinionActorReadyObserver.generated.h"

struct FMassEntityManager;

/**
 * M10-5 的一次性就绪校验器。
 * Factory 已在添加 Ready Tag 前完成 Entity、Actor、ASC 的权威桥接；此观察器只验证该不变量，
 * 不在运行时 Tick，也不负责生成或销毁 Actor。
 */
UCLASS()
class POLYGONSCIFIWORLDS_API USWMinionActorReadyObserver : public UMassObserverProcessor
{
	GENERATED_BODY()

public:
	USWMinionActorReadyObserver();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery ReadyQuery;
};
