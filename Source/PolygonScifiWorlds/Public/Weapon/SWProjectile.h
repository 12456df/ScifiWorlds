// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/SWAbilityTypes.h"
#include "GameFramework/Actor.h"
#include "Interaction/SWProjectileAbsorptionInterface.h"
#include "Weapon/SWProjectileTypes.h"
#include "SWProjectile.generated.h"

class UProjectileMovementComponent;
class UPrimitiveComponent;
class USphereComponent;
class UStaticMeshComponent;
class APawn;
class USWDamageGameplayEffect;

/** 服务器生成、服务器命中的复制弹丸；M04 只发出命中事件，不结算伤害。 */
UCLASS()
class POLYGONSCIFIWORLDS_API ASWProjectile : public AActor, public ISWProjectileAbsorptionInterface
{
	GENERATED_BODY()

public:
	ASWProjectile();

	/** 由服务器在 SpawnActorDeferred 后调用；成功后启动弹丸移动。 */
	bool InitializeProjectileAuthority(APawn* InInstigatorPawn, const FVector& LaunchDirection,
		TSubclassOf<USWDamageGameplayEffect> InDamageEffectClass, const FSWDamageApplicationParams& InDamageParams);

	UFUNCTION(BlueprintPure, Category = "Projectile")
	FVector GetLaunchVelocity() const;

	virtual bool CanBeAbsorbedByShield_Implementation() const override;
	virtual void AbsorbByShieldAuthority_Implementation(AActor* ShieldActor) override;

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

	UFUNCTION()
	void OnProjectileBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION(BlueprintImplementableEvent, Category = "Projectile")
	void BP_OnProjectileLaunched();

	UFUNCTION(BlueprintImplementableEvent, Category = "Projectile")
	void BP_OnProjectileImpact(const FHitResult& Hit);

private:
	void ConfigureAuthorityCollision();
	void HandleAuthoritativeImpact(AActor* HitActor, const FHitResult& Hit);
	bool ApplyDamageEffectAuthority(AActor* HitActor);

	bool bInitialized = false;
	bool bImpactHandled = false;

	/** 由持有武器在服务器初始化时传入，弹丸蓝图不再拥有独立伤害配置。 */
	TSubclassOf<USWDamageGameplayEffect> DamageEffectClass;

	/** 发射时快照的原始伤害包，避免弹体飞行期间受施法者属性变化影响。 */
	FSWDamageApplicationParams DamageParams;
};
