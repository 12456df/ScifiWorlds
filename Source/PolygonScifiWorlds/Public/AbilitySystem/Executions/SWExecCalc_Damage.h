// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "SWExecCalc_Damage.generated.h"

/**
 * 服务器权威的单通道伤害执行计算。
 * 它只输出 IncomingDamage；生命扣减、死亡提交、吸血和伤害数字由后续独立战斗层消费结果。
 */
UCLASS()
class POLYGONSCIFIWORLDS_API USWExecCalc_Damage : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	USWExecCalc_Damage();

	virtual void Execute_Implementation(
		const FGameplayEffectCustomExecutionParameters& ExecutionParams,
		FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
