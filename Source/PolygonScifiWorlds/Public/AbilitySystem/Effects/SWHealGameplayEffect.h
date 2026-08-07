// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/SWGameplayEffect.h"
#include "SWHealGameplayEffect.generated.h"

/**
 * 服务器权威的统一瞬时治疗 GE。
 * 治疗量由 C++ 通过 SetByCaller.Healing 写入，AttributeSet 负责最终的生命值 Clamp。
 */
UCLASS()
class POLYGONSCIFIWORLDS_API USWHealGameplayEffect : public USWGameplayEffect
{
	GENERATED_BODY()

public:
	USWHealGameplayEffect();
};
