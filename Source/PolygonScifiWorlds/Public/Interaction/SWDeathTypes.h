// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"

class AActor;

/**
 * 一次已由服务器确认的死亡结果。
 * 该结构仅在当前服务器调用链和原生委托内传递，不作为可复制的持久状态；
 * 客户端与晚加入者以 Character 上复制的 bDead 作为死亡真值。
 */
struct FSWDeathContext
{
	/** 被服务器确认死亡的 Avatar；用于重生、奖励等后续协调。 */
	AActor* VictimActor = nullptr;

	/** 造成这次伤害的原始单位；环境伤害时允许为空。 */
	AActor* InstigatorActor = nullptr;

	/** 实际造成伤害的 Actor，例如弹丸或武器；允许为空。 */
	AActor* EffectCauser = nullptr;

	/** 本次结算后实际消耗的生命值，不包含 Overkill。 */
	float AppliedDamage = 0.f;
};

/** 仅服务器使用的死亡提交通知；GameMode、经验奖励等协调者后续可订阅。 */
DECLARE_MULTICAST_DELEGATE_OneParam(FSWOnDeath, const FSWDeathContext&);
