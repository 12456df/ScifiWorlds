// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/SWAbilityTypes.h"
#include "GameplayEffectTypes.h"
#include "GameFramework/Actor.h"
#include "Interaction/SWProjectileAbsorptionInterface.h"
#include "TimerManager.h"
#include "SWPortalSphereProjectile.generated.h"

class UProjectileMovementComponent;
class UPrimitiveComponent;
class USphereComponent;
class USWPortalSphereDamageGameplayEffect;
class APawn;

/**
 * PortalSphere 的服务器权威弹体。
 * 小球负责阻挡，大球负责对每个目标至多施加一次周期伤害 GE；客户端只接收移动复制和蓝图表现。
 */
UCLASS(Abstract, Blueprintable)
class POLYGONSCIFIWORLDS_API ASWPortalSphereProjectile : public AActor, public ISWProjectileAbsorptionInterface
{
	GENERATED_BODY()

public:
	ASWPortalSphereProjectile();

	/**
	 * 仅服务器在 SpawnActorDeferred 后调用。RawDamage 为每个周期的未减免伤害；Duration 同时限制弹体寿命与 DoT 持续时间。
	 */
	UFUNCTION(BlueprintCallable, Category = "SW|PortalSphere")
	bool InitializePortalSphereAuthority(
		APawn* InInstigatorPawn,
		FVector LaunchDirection,
		TSubclassOf<USWPortalSphereDamageGameplayEffect> InDamageEffectClass,
		const FSWDamageApplicationParams& InDamageParams,
		float InProjectileSpeed,
		float InMaximumRange,
		float InDurationSeconds,
		float InBlockingSphereRadius,
		float InDamageSphereRadius,
		float InVisualScale);

	virtual bool CanBeAbsorbedByShield_Implementation() const override;
	virtual void AbsorbByShieldAuthority_Implementation(AActor* ShieldActor) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SW|PortalSphere")
	TObjectPtr<USphereComponent> BlockingSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SW|PortalSphere")
	TObjectPtr<USphereComponent> DamageSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SW|PortalSphere")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	UFUNCTION(BlueprintImplementableEvent, Category = "SW|PortalSphere")
	void BP_OnPortalSphereLaunched();

	/** 蓝图在此事件中缩放 Niagara 等纯表现组件；碰撞半径始终由 C++ 权威设置。 */
	UFUNCTION(BlueprintImplementableEvent, Category = "SW|PortalSphere")
	void BP_OnPortalSphereVisualScaleChanged(float InVisualScale);

	UFUNCTION(BlueprintImplementableEvent, Category = "SW|PortalSphere")
	void BP_OnPortalSphereBlocked(const FHitResult& Hit);

private:
	UFUNCTION()
	void OnBlockingSphereHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent,
		FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION()
	void OnBlockingSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnBlockingSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex);

	UFUNCTION()
	void OnDamageSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnDamageSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex);

	FActiveGameplayEffectHandle ApplyDamageEffectAuthority(AActor* TargetActor);
	void RemoveDamageEffectAuthority(AActor* TargetActor);
	void ClearDamageEffectsAuthority();
	void ConfigureAuthorityCollision();
	void RefreshProjectileSpeedAuthority();
	void ScheduleRangeStopAuthority();
	void StopAtMaximumRangeAuthority();

	UFUNCTION()
	void OnRep_VisualScale();

	bool bInitialized = false;
	TSubclassOf<USWPortalSphereDamageGameplayEffect> DamageEffectClass;
	FSWDamageApplicationParams DamageParams;
	/** 仅服务器保存：目标离开范围或弹体销毁时，用句柄立即停止其周期伤害。 */
	TMap<TWeakObjectPtr<AActor>, FActiveGameplayEffectHandle> ActiveDamageEffects;
	/** 仅服务器保存：任一重叠组件尚未离开时保持减速，避免多个组件重叠造成过早恢复。 */
	TSet<TWeakObjectPtr<UPrimitiveComponent>> SlowingOverlapComponents;
	FVector LaunchVelocity = FVector::ZeroVector;
	FVector LaunchStartLocation = FVector::ZeroVector;
	FVector InitialLaunchDirection = FVector::ZeroVector;
	float BaseProjectileSpeed = 0.f;
	float MaximumTravelDistance = 0.f;
	bool bReachedMaximumRange = false;
	FTimerHandle RangeStopTimer;

	/** 碰撞小球与 Pawn、屏障或其他 Projectile 重叠时的速度倍率；数值由 PortalSphere 蓝图调节。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SW|PortalSphere|Movement", meta = (ClampMin = "0.01", ClampMax = "1.0", AllowPrivateAccess = "true"))
	float OverlapSpeedMultiplier = 0.2f;

	/** 与作用范围同源的纯表现缩放；复制给所有客户端供 Niagara 使用。 */
	UPROPERTY(ReplicatedUsing = OnRep_VisualScale)
	float VisualScale = 1.f;
};
