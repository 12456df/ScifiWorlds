// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "SWMinionTargetingProcessor.generated.h"

struct FMassEntityManager;
struct FMassExecutionContext;

/**
 * 仅服务器：按每 Entity 的错峰低频周期查询 Target Registry，并写入当前 Target Fragment。
 * 该 Processor 不执行 World Overlap、不创建 Timer、不决定 StateTree 转换或攻击伤害。
 */
UCLASS()
class POLYGONSCIFIWORLDS_API USWMinionTargetingProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	USWMinionTargetingProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EntityQuery;
};
