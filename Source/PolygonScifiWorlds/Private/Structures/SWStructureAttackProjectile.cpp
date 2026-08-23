// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "Structures/SWStructureAttackProjectile.h"

#include "Collision/SWCollisionChannels.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SWStructureAttackProjectile)

ASWStructureAttackProjectile::ASWStructureAttackProjectile()
{
	// 防御结构火球经过其他 Pawn 时不应被其物理阻挡；只有锁定目标会消费本次命中。
	CollisionComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionComponent->SetCollisionResponseToChannel(SWCollisionChannels::ShieldBarrier, ECR_Ignore);
}

bool ASWStructureAttackProjectile::InitializeStructureAttackProjectileAuthority(APawn* const InInstigatorPawn,
	AActor* const TargetActor, const FVector& LaunchDirection,
	const TSubclassOf<USWDamageGameplayEffect> InDamageEffectClass,
	const FSWDamageApplicationParams& InDamageParams)
{
	if (!HasAuthority() || !TargetActor || !TargetActor->GetRootComponent()
		|| !Super::InitializeProjectileAuthority(InInstigatorPawn, LaunchDirection, InDamageEffectClass, InDamageParams))
	{
		return false;
	}

	HomingTarget = TargetActor;
	// 基类初始化会恢复通用 Projectile 的碰撞响应；结构火球在此覆盖为穿过非锁定 Pawn、忽略屏障。
	CollisionComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionComponent->SetCollisionResponseToChannel(SWCollisionChannels::ShieldBarrier, ECR_Ignore);
	ProjectileMovement->bIsHomingProjectile = true;
	ProjectileMovement->HomingTargetComponent = TargetActor->GetRootComponent();
	ProjectileMovement->HomingAccelerationMagnitude = FMath::Max(0.f, HomingAccelerationMagnitude);
	return true;
}

bool ASWStructureAttackProjectile::CanBeAbsorbedByShield_Implementation() const
{
	return false;
}

ECollisionChannel ASWStructureAttackProjectile::GetProjectileCollisionChannel() const
{
	return SWCollisionChannels::StructureProjectile;
}

bool ASWStructureAttackProjectile::ShouldHandleImpactAuthority(AActor* const HitActor) const
{
	// 世界阻挡仍应销毁火球；穿过非锁定 Pawn 时不消耗火球，也不产生旁路伤害。
	return !HitActor || !HitActor->IsA<APawn>() || HitActor == HomingTarget.Get();
}
