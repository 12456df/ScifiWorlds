// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "Engine/EngineTypes.h"

/**
 * 项目自定义碰撞通道的唯一 C++ 声明位置。
 * 各常量必须与 DefaultEngine.ini 中的 ECC_GameTraceChannel 保持一致。
 * `StructureProjectile` 是防御结构专用对象类型，不受玩家屏障的 Projectile 吸收规则影响。
 */
namespace SWCollisionChannels
{
	constexpr ECollisionChannel Projectile = ECC_GameTraceChannel2;
	constexpr ECollisionChannel ShieldBarrier = ECC_GameTraceChannel3;
	constexpr ECollisionChannel StructureProjectile = ECC_GameTraceChannel4;
	/** 仅本地玩家头顶血条范围探测使用；默认忽略，避免与关卡碰撞产生无关 Overlap。 */
	constexpr ECollisionChannel HealthBarRangeProbe = ECC_GameTraceChannel5;
}
