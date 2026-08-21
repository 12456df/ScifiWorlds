// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "MassEntityQuery.h"
#include "MassObserverProcessor.h"
#include "SWMinionDeathObserver.generated.h"

struct FMassEntityManager;

/**
 * 小兵死亡桥的一次性后半段：监听 Dead Tag，按 CombatantDefinition 写入尸体回收时刻。
 * 它不负责扣血、发奖或播放表现；这些都已在 Actor 的统一死亡提交链完成。
 */
UCLASS()
class POLYGONSCIFIWORLDS_API USWMinionDeathObserver : public UMassObserverProcessor
{
	GENERATED_BODY()

public:
	USWMinionDeathObserver();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery DeathQuery;
};
