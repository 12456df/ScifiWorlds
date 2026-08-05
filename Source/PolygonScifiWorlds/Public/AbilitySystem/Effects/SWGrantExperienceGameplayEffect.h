// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/SWGameplayEffect.h"
#include "SWGrantExperienceGameplayEffect.generated.h"

/**
 * 经验奖励的统一瞬时 GE。
 * 奖励数值由服务器以 SetByCaller.Experience 写入，AttributeSet 消费 IncomingXP 后交由 PlayerState 结算。
 */
UCLASS()
class POLYGONSCIFIWORLDS_API USWGrantExperienceGameplayEffect : public USWGameplayEffect
{
	GENERATED_BODY()

public:
	USWGrantExperienceGameplayEffect();
};
