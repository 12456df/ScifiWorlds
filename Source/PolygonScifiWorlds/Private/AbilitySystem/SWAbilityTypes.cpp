// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "AbilitySystem/SWAbilityTypes.h"

bool FSWGameplayEffectContext::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	if (!FGameplayEffectContext::NetSerialize(Ar, Map, bOutSuccess))
	{
		return false;
	}

	uint8 RepBits = 0;
	if (Ar.IsSaving())
	{
		if (bCriticalHit)
		{
			RepBits |= 1 << 0;
		}
		if (DamageType.IsValid())
		{
			RepBits |= 1 << 1;
		}
		if (PhysicalLifesteal > 0.f && FMath::IsFinite(PhysicalLifesteal))
		{
			RepBits |= 1 << 2;
		}
	}

	Ar.SerializeBits(&RepBits, 3);
	bCriticalHit = (RepBits & (1 << 0)) != 0;

	if (RepBits & (1 << 1))
	{
		DamageType.NetSerialize(Ar, Map, bOutSuccess);
	}
	else if (Ar.IsLoading())
	{
		DamageType = FGameplayTag();
	}

	if (RepBits & (1 << 2))
	{
		Ar << PhysicalLifesteal;
	}
	else if (Ar.IsLoading())
	{
		PhysicalLifesteal = 0.f;
	}

	return bOutSuccess;
}
