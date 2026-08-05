// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SWDamageNumberTypes.generated.h"

class AActor;

/**
 * 服务器伤害结算后发送给攻击者客户端的纯表现数据。
 * 数值、伤害类型与暴击结果均已由服务器确定；蓝图只能据此决定颜色、文字、动画和生命周期。
 */
USTRUCT(BlueprintType)
struct POLYGONSCIFIWORLDS_API FSWDamageNumberPayload
{
	GENERATED_BODY()

	/** 实际从目标生命值中扣除的伤害，用于避免 Overkill 显示和收益不一致。 */
	UPROPERTY(BlueprintReadOnly, Category = "Damage Number")
	float AppliedDamage = 0.f;

	/** 伤害类型，首版为 Damage.Type.Physical、Damage.Type.Magical 或 Damage.Type.True。 */
	UPROPERTY(BlueprintReadOnly, Category = "Damage Number")
	FGameplayTag DamageType;

	/** 服务器唯一判定的暴击标记；蓝图仅负责对应的视觉样式。 */
	UPROPERTY(BlueprintReadOnly, Category = "Damage Number")
	bool bCritical = false;

	/** 受击目标；网络上暂不可解析时，蓝图应回退使用 WorldLocation。 */
	UPROPERTY(BlueprintReadOnly, Category = "Damage Number")
	TObjectPtr<AActor> TargetActor = nullptr;

	/** 浮动数字的世界坐标回退值。 */
	UPROPERTY(BlueprintReadOnly, Category = "Damage Number")
	FVector WorldLocation = FVector::ZeroVector;
};
