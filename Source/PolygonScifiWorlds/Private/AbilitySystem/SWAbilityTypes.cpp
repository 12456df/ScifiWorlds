// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "AbilitySystem/SWAbilityTypes.h"

bool FSWGameplayEffectContext::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	// M03 未新增需要复制的字段，直接复用基类序列化流程。
	// 后续新增战斗结果字段时，应改为按位打包（RepBits）以避免冗余带宽。
	return FGameplayEffectContext::NetSerialize(Ar, Map, bOutSuccess);
}
