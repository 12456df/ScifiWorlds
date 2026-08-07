// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "Weapon/SWProjectileTypes.h"

bool FSWProjectileConfig::IsValid() const
{
	return InitialSpeed > 0.f
		&& (MaxSpeed <= 0.f || MaxSpeed >= InitialSpeed)
		&& FMath::IsFinite(GravityScale)
		&& CollisionRadius > 0.f
		&& LifeSeconds > 0.f
		&& Bounciness >= 0.f
		&& Bounciness <= 1.f;
}
