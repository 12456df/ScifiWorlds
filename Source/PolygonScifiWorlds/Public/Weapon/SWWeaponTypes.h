// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/SWAbilityTypes.h"
#include "AbilitySystem/Effects/SWDamageGameplayEffect.h"
#include "GameplayTagContainer.h"
#include "ScalableFloat.h"
#include "Weapon/SWProjectile.h"
#include "SWWeaponTypes.generated.h"

class UAnimMontage;
class UTexture2D;

/** 服务器对单次射击采用的权威命中判定方式。 */
UENUM(BlueprintType)
enum class ESWShotResolutionMode : uint8
{
	Projectile UMETA(DisplayName = "Projectile"),
	Hitscan UMETA(DisplayName = "Hitscan")
};

UENUM(BlueprintType)
enum class ESWFireMontageSelectionMode : uint8
{
	FirstValid UMETA(DisplayName = "First Valid"),
	Sequential UMETA(DisplayName = "Sequential")
};

/** 武器自身的原始伤害规则；不得放入通用 Damage GE。 */
USTRUCT(BlueprintType)
struct FSWWeaponDamageConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Damage")
	FGameplayTag DamageType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Damage", meta = (ClampMin = "0.0"))
	FScalableFloat BaseDamage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Damage")
	FScalableFloat AttackPowerCoefficient;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Damage")
	FScalableFloat SpellPowerCoefficient;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Damage")
	bool bCanCritical = true;

	bool IsValid() const;
};

/** 单个武器开火动作的内容配置。 */
USTRUCT(BlueprintType)
struct FSWFireMontageVariant
{
	GENERATED_BODY()

	/** 开火 Montage；其中的 Fire Section 必须放置 Event.Weapon.Fire Notify。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Animation")
	TObjectPtr<UAnimMontage> Montage = nullptr;

	/** 留空时从 Montage 起点播放；填写时从指定 Section 开始播放。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Animation")
	FName StartSection = NAME_None;

	/** 该动作在基础射速下的播放倍率。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Animation", meta = (ClampMin = "0.01"))
	float PlayRate = 1.f;
};

/** Fire Ability 选择一次动作后交给蓝图播放的只读结果。 */
USTRUCT(BlueprintType)
struct FSWFireMontageSelection
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Weapon|Animation")
	TObjectPtr<UAnimMontage> Montage = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon|Animation")
	FName StartSection = NAME_None;

	/** 已计入 FireIntervalReductionPercent 的实际播放倍率。 */
	UPROPERTY(BlueprintReadOnly, Category = "Weapon|Animation")
	float EffectivePlayRate = 1.f;

	/** 供 Ability 内部推进 Sequential 选择，不是权威运行时状态。 */
	int32 VariantIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon|Animation")
	bool bValid = false;
};

/** 固定武器的静态配置；由武器蓝图 Defaults 提供，运行时状态由服务器持有。 */
USTRUCT(BlueprintType)
struct FSWWeaponConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "1"))
	int32 MagazineCapacity = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0.0"))
	float HipSpreadDegrees = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AimSpreadMultiplier = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	bool bAutomatic = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	bool bSupportsAim = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0.0"))
	float ReloadDurationSeconds = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0.0"))
	float MaxAimDistance = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "1.0"))
	float AimFOV = 70.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FVector AimCameraOffset = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0.0"))
	float AimTransitionSeconds = 0.15f;

	/** 武器蓝图选择命中判定模式；两种模式共用 DamageEffectClass。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Shot")
	ESWShotResolutionMode ShotResolutionMode = ESWShotResolutionMode::Projectile;

	/** 服务器命中有效敌方 ASC 后应用的统一伤害 GE。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Shot")
	TSubclassOf<USWDamageGameplayEffect> DamageEffectClass;

	/** 武器生成原始伤害的规则；目标防御、穿透和暴击仍由 Damage GE 的 ExecCalc 结算。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Shot")
	FSWWeaponDamageConfig DamageConfig;

	/** 仅 Projectile 模式需要填写；Hitscan 模式不会生成弹丸 Actor。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<ASWProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName MuzzleSocketName = NAME_None;

	/** 开火表现的候选动作。射速属性会按比例同步缩放其播放倍率。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Animation")
	TArray<FSWFireMontageVariant> FireMontageVariants;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Animation")
	ESWFireMontageSelectionMode FireMontageSelectionMode = ESWFireMontageSelectionMode::FirstValid;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Animation")
	TObjectPtr<UAnimMontage> ReloadMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Cue")
	FGameplayTag FireGameplayCueTag;

	/** 武器 HUD 图标；作为软引用由拥有者 UI 按需加载，不影响服务器权威结算。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|UI")
	TSoftObjectPtr<UTexture2D> WeaponIcon;

	bool IsValidForFire() const;
};

/** 单次服务器权威射击的内部结果；不会复制或暴露为蓝图权威输入。 */
struct FSWResolvedShot
{
	ESWShotResolutionMode Mode = ESWShotResolutionMode::Projectile;
	FTransform MuzzleTransform;
	FVector TraceEnd = FVector::ZeroVector;
	FHitResult HitResult;
	bool bFired = false;
	bool bBlockingHit = false;
	int32 MagazineAmmoAfterShot = 0;
};
