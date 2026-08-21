// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "SWMinionActorSyncProcessor.generated.h"

struct FMassEntityManager;
struct FMassExecutionContext;

/**
 * 仅服务器：将已完成初始化的 Mass Transform 派生写入对应的小兵 Character。
 * Character 不运行自主移动逻辑；客户端通过该 Actor 的 Movement Replication 接收表现位置。
 */
UCLASS()
class POLYGONSCIFIWORLDS_API USWMinionActorSyncProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	USWMinionActorSyncProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EntityQuery;
};
