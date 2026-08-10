// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "Engine/EngineTypes.h"

/**
 * 项目自定义碰撞通道的唯一 C++ 声明位置。
 * `Projectile` 必须与 DefaultEngine.ini 中的 ECC_GameTraceChannel2 保持一致，默认响应为重叠。
 */
namespace SWCollisionChannels
{
	constexpr ECollisionChannel Projectile = ECC_GameTraceChannel2;
	constexpr ECollisionChannel ShieldBarrier = ECC_GameTraceChannel3;
}
