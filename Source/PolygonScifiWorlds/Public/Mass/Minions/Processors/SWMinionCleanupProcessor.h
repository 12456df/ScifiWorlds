// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "SWMinionCleanupProcessor.generated.h"

struct FMassEntityManager;
struct FMassExecutionContext;

/**
 * 仅服务器：在死亡 Observer 写入的统一尸体截止时间到达后，成对销毁小兵 Actor 与 Mass Entity。
 * 不创建每小兵 Timer；Entity 销毁通过 Context.Defer 延后到本次 Mass 迭代安全结束。
 */
UCLASS()
class POLYGONSCIFIWORLDS_API USWMinionCleanupProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	USWMinionCleanupProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EntityQuery;
};
