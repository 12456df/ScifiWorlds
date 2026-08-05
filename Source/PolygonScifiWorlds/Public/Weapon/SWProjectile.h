// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapon/SWProjectileTypes.h"
#include "SWProjectile.generated.h"

class UProjectileMovementComponent;
class UPrimitiveComponent;
class USphereComponent;
class UStaticMeshComponent;
class APawn;

/** 服务器生成、服务器命中的复制弹丸；M04 只发出命中事件，不结算伤害。 */
UCLASS()
class POLYGONSCIFIWORLDS_API ASWProjectile : public AActor
{
	GENERATED_BODY()

public:
	ASWProjectile();

	/** 由服务器在 SpawnActorDeferred 后调用；成功后启动弹丸移动。 */
	bool InitializeProjectileAuthority(APawn* InInstigatorPawn, const FVector& LaunchDirection);

	UFUNCTION(BlueprintPure, Category = "Projectile")
	FVector GetLaunchVelocity() const;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<USphereComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UStaticMeshComponent> ProjectileMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	FSWProjectileConfig ProjectileConfig;

	UFUNCTION()
	void OnProjectileHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent,
		FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION(BlueprintImplementableEvent, Category = "Projectile")
	void BP_OnProjectileLaunched();

	UFUNCTION(BlueprintImplementableEvent, Category = "Projectile")
	void BP_OnProjectileImpact(const FHitResult& Hit);

private:
	void HandleAuthoritativeImpact(const FHitResult& Hit);
	bool ApplyDamageEffectAuthority(AActor* HitActor);

	bool bInitialized = false;
	bool bImpactHandled = false;
};
