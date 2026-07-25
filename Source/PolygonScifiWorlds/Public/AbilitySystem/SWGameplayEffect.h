// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "SWGameplayEffect.generated.h"

/**
 * ScifiWorlds 所有 Gameplay Effect 的 C++ 基础契约类型。
 *
 * M03 仅建立该基类以统一后续效果内容的派生入口，不在此写入任何平衡数值或具体修正；
 * 具体效果（伤害、消耗、冷却、Buff/Debuff 等）由后续内容模块以数据资产或蓝图子类落地。
 */
UCLASS()
class POLYGONSCIFIWORLDS_API USWGameplayEffect : public UGameplayEffect
{
	GENERATED_BODY()
};
