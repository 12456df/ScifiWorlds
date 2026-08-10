// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "Weapon/SWWeaponTypes.h"

#include "GameplayTags/SWGameplayTags.h"

bool FSWWeaponDamageConfig::IsValid() const
{
	return DamageType == SWGameplayTags::Damage_Type_Physical
		|| DamageType == SWGameplayTags::Damage_Type_Magical
		|| DamageType == SWGameplayTags::Damage_Type_True;
}

bool FSWWeaponConfig::IsValidForFire() const
{
	return MagazineCapacity > 0
		&& HipSpreadDegrees >= 0.f
		&& AimSpreadMultiplier >= 0.f
		&& AimSpreadMultiplier <= 1.f
		&& ReloadDurationSeconds > 0.f
		&& MaxAimDistance > 0.f
		&& AimFOV > 0.f
		&& AimTransitionSeconds >= 0.f
		&& DamageEffectClass != nullptr
		&& DamageConfig.IsValid()
		&& (ShotResolutionMode != ESWShotResolutionMode::Projectile || ProjectileClass != nullptr)
		&& !MuzzleSocketName.IsNone();
}
