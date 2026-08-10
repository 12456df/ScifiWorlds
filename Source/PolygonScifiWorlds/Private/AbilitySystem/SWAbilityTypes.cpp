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
		if (bCanCritical)
		{
			RepBits |= 1 << 0;
		}
		if (bCriticalHit)
		{
			RepBits |= 1 << 1;
		}
		if (DamageType.IsValid())
		{
			RepBits |= 1 << 2;
		}
		if (PhysicalLifesteal > 0.f && FMath::IsFinite(PhysicalLifesteal))
		{
			RepBits |= 1 << 3;
		}
	}

	Ar.SerializeBits(&RepBits, 4);
	bCanCritical = (RepBits & (1 << 0)) != 0;
	bCriticalHit = (RepBits & (1 << 1)) != 0;

	if (RepBits & (1 << 2))
	{
		DamageType.NetSerialize(Ar, Map, bOutSuccess);
	}
	else if (Ar.IsLoading())
	{
		DamageType = FGameplayTag();
	}

	if (RepBits & (1 << 3))
	{
		Ar << PhysicalLifesteal;
	}
	else if (Ar.IsLoading())
	{
		PhysicalLifesteal = 0.f;
	}

	return bOutSuccess;
}
