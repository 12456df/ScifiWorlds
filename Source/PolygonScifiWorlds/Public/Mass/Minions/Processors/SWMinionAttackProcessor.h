// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "SWMinionAttackProcessor.generated.h"

struct FMassEntityManager;
struct FMassExecutionContext;

/**
 * 仅服务器：消费 StateTree 写入的一次攻击请求，并将其交给对应 Character Actor 的 GAS 攻击桥。
 * 不轮询、不中断 GA，也不直接施加伤害。
 */
UCLASS()
class POLYGONSCIFIWORLDS_API USWMinionAttackProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	USWMinionAttackProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EntityQuery;
};
