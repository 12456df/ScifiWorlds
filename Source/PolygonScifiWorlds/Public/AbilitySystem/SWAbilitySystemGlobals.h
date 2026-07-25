// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemGlobals.h"
#include "SWAbilitySystemGlobals.generated.h"

/**
 * ScifiWorlds 的 AbilitySystemGlobals 单例。
 *
 * 通过覆写效果上下文的分配逻辑，使 GAS 在创建 EffectContext 时使用项目自定义的
 * FSWGameplayEffectContext。需在 DefaultGame.ini 中通过 AbilitySystemGlobalsClassName 注册。
 */
UCLASS()
class POLYGONSCIFIWORLDS_API USWAbilitySystemGlobals : public UAbilitySystemGlobals
{
	GENERATED_BODY()

	// 供 ASC->MakeEffectContext() 调用，分配项目自定义的 GameplayEffectContext。
	virtual FGameplayEffectContext* AllocGameplayEffectContext() const override;
};
