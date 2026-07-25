// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "AbilitySystem/SWAbilitySystemGlobals.h"

#include "AbilitySystem/SWAbilityTypes.h"

FGameplayEffectContext* USWAbilitySystemGlobals::AllocGameplayEffectContext() const
{
	// 返回项目自定义上下文，确保效果链路全程携带 FSWGameplayEffectContext。
	return new FSWGameplayEffectContext();
}
