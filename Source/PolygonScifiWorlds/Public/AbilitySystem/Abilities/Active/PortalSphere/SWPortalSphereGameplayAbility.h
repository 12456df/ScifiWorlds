// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Active/SWActiveGameplayAbility.h"
#include "ScalableFloat.h"
#include "SWPortalSphereGameplayAbility.generated.h"

class ASWPortalSphereProjectile;
class USWPortalSphereDamageGameplayEffect;

/**
 * PortalSphere 的专属技能数据契约。
 * 蓝图子类负责 Commit、蒙太奇、生成弹体与表现；本类只保存可调技能数据和只读查询。
 */
UCLASS(Abstract, Blueprintable)
class POLYGONSCIFIWORLDS_API USWPortalSphereGameplayAbility : public USWActiveGameplayAbility
{
	GENERATED_BODY()

public:
	USWPortalSphereGameplayAbility();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SW|PortalSphere|Projectile")
	TSubclassOf<ASWPortalSphereProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SW|PortalSphere|Damage")
	TSubclassOf<USWPortalSphereDamageGameplayEffect> DamageEffectClass;

	/** 每个周期结算前的基础魔法伤害。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SW|PortalSphere|Damage", meta = (ClampMin = "0.0"))
	FScalableFloat BaseMagicDamagePerTick;

	/** 每个周期参与原始伤害计算的法强系数。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SW|PortalSphere|Damage", meta = (ClampMin = "0.0"))
	FScalableFloat SpellPowerCoefficientPerTick;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SW|PortalSphere|Projectile", meta = (ClampMin = "0.0"))
	FScalableFloat BlockingSphereRadius;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SW|PortalSphere|Projectile", meta = (ClampMin = "0.0"))
	FScalableFloat DamageSphereRadius;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SW|PortalSphere|Projectile", meta = (ClampMin = "0.0"))
	FScalableFloat ProjectileSpeed;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SW|PortalSphere|Projectile", meta = (ClampMin = "0.0"))
	FScalableFloat BaseRange;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SW|PortalSphere|Projectile", meta = (ClampMin = "0.0"))
	FScalableFloat BaseDuration;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SW|PortalSphere|Damage")
	bool bCanCritical = false;

	UFUNCTION(BlueprintPure, Category = "SW|PortalSphere")
	float GetBaseMagicDamagePerTick(int32 AbilityLevel) const;

	UFUNCTION(BlueprintPure, Category = "SW|PortalSphere")
	float GetSpellPowerCoefficientPerTick(int32 AbilityLevel) const;

	/** 仅供 PortalSphere 蓝图在服务器生成原始伤害快照时读取当前法强。 */
	UFUNCTION(BlueprintPure, Category = "SW|PortalSphere")
	float GetSpellPowerForPortalDamage() const;

	UFUNCTION(BlueprintPure, Category = "SW|PortalSphere")
	float GetBlockingSphereRadius(int32 AbilityLevel) const;

	UFUNCTION(BlueprintPure, Category = "SW|PortalSphere")
	float GetDamageSphereRadius(int32 AbilityLevel) const;

	UFUNCTION(BlueprintPure, Category = "SW|PortalSphere")
	float GetProjectileSpeed(int32 AbilityLevel) const;

	UFUNCTION(BlueprintPure, Category = "SW|PortalSphere")
	float GetEffectiveProjectileRange(int32 AbilityLevel) const;

	UFUNCTION(BlueprintPure, Category = "SW|PortalSphere")
	float GetEffectiveProjectileDuration(int32 AbilityLevel) const;

	/**
	 * 仅服务器调用。蓝图只提供释放帧确定的生成变换和瞄准方向；伤害快照与延迟生成由 C++ 统一完成，
	 * 避免 Blueprint 无法访问的 Deferred Spawn 节点泄漏到具体技能图中。
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "SW|PortalSphere")
	bool SpawnPortalSphereAuthority(const FTransform& SpawnTransform, FVector LaunchDirection);
};
