// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "Weapon/SWWeaponTypes.h"

bool FSWWeaponConfig::IsValidForFire() const
{
	return MagazineCapacity > 0
		&& RoundsPerMinute > 0.f
		&& HipSpreadDegrees >= 0.f
		&& AimSpreadMultiplier >= 0.f
		&& AimSpreadMultiplier <= 1.f
		&& ReloadDurationSeconds > 0.f
		&& MaxAimDistance > 0.f
		&& AimFOV > 0.f
		&& AimTransitionSeconds >= 0.f
		&& ProjectileClass != nullptr
		&& !MuzzleSocketName.IsNone();
}
