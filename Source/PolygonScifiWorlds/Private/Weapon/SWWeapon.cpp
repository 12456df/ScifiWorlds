// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "Weapon/SWWeapon.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/SWAttributeSet.h"
#include "Components/MeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameplayTags/SWGameplayTags.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Weapon/SWProjectile.h"

ASWWeapon::ASWWeapon()
{
	bReplicates = true;
	SetReplicateMovement(false);
	SetCanBeDamaged(false);
	PrimaryActorTick.bCanEverTick = false;

	SkeletalWeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalWeaponMesh"));
	SkeletalWeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RootComponent = SkeletalWeaponMesh;

	StaticWeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticWeaponMesh"));
	StaticWeaponMesh->SetupAttachment(SkeletalWeaponMesh);
	StaticWeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ASWWeapon::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		return;
	}

	if (!WeaponConfig.IsValidForFire())
	{
		UE_LOG(LogTemp, Error, TEXT("武器 %s 的配置无效，服务器拒绝初始化弹药。"), *GetName());
		return;
	}

	CurrentMagazineAmmo = GetEffectiveMagazineCapacity();
	BroadcastAmmoChanged();
}

void ASWWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(ASWWeapon, CurrentMagazineAmmo, COND_OwnerOnly, REPNOTIFY_Always);
}

bool ASWWeapon::CanFire() const
{
	if (!WeaponConfig.IsValidForFire() || CurrentMagazineAmmo <= 0)
	{
		return false;
	}

	// 非权威端只用于 UI 预览，实际射速校验始终由服务器在 TryFireAuthority 中完成。
	if (!HasAuthority())
	{
		return true;
	}

	const UWorld* World = GetWorld();
	return World != nullptr && World->GetTimeSeconds() >= NextAllowedFireServerTime;
}

bool ASWWeapon::CanReload() const
{
	return WeaponConfig.IsValidForFire()
		&& CurrentMagazineAmmo < GetEffectiveMagazineCapacity();
}

bool ASWWeapon::GetMuzzleTransform(FTransform& OutMuzzleTransform) const
{
	if (UMeshComponent* WeaponMesh = GetActiveWeaponMesh())
	{
		if (WeaponMesh->DoesSocketExist(WeaponConfig.MuzzleSocketName))
		{
			OutMuzzleTransform = WeaponMesh->GetSocketTransform(WeaponConfig.MuzzleSocketName, RTS_World);
			return true;
		}
	}

	return false;
}

bool ASWWeapon::GetAimCameraSettings(float& OutAimFOV, FVector& OutAimCameraOffset, float& OutTransitionSeconds) const
{
	if (!WeaponConfig.bSupportsAim)
	{
		return false;
	}

	OutAimFOV = WeaponConfig.AimFOV;
	OutAimCameraOffset = WeaponConfig.AimCameraOffset;
	OutTransitionSeconds = WeaponConfig.AimTransitionSeconds;
	return true;
}

float ASWWeapon::GetEffectiveFireIntervalSeconds() const
{
	return GetEffectiveFireInterval();
}

FSWShotResult ASWWeapon::TryFireAuthority()
{
	FSWShotResult Result;
	Result.MagazineAmmoAfterShot = CurrentMagazineAmmo;

	if (!CanFire())
	{
		return Result;
	}

	FTransform MuzzleTransform;
	FVector ShotDirection;
	if (!GetMuzzleTransform(MuzzleTransform) || !BuildAuthoritativeShotDirection(MuzzleTransform, ShotDirection))
	{
		UE_LOG(LogTemp, Warning, TEXT("武器 %s 缺少有效枪口或瞄准上下文，本次射击被拒绝。"), *GetName());
		return Result;
	}

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	ASWProjectile* Projectile = GetWorld()->SpawnActorDeferred<ASWProjectile>(WeaponConfig.ProjectileClass, MuzzleTransform, OwnerPawn, OwnerPawn,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Projectile)
	{
		UE_LOG(LogTemp, Warning, TEXT("武器 %s 未能生成权威弹丸，本次射击未扣弹。"), *GetName());
		return Result;
	}

	UGameplayStatics::FinishSpawningActor(Projectile, MuzzleTransform);
	if (!Projectile->InitializeProjectileAuthority(OwnerPawn, ShotDirection))
	{
		Projectile->Destroy();
		return Result;
	}
	CurrentMagazineAmmo = FMath::Max(0, CurrentMagazineAmmo - 1);
	NextAllowedFireServerTime = GetWorld()->GetTimeSeconds() + GetEffectiveFireInterval();
	BroadcastAmmoChanged();
	BP_OnFireCosmetic();

	if (WeaponConfig.FireGameplayCueTag.IsValid())
	{
		ExecuteOwnerGameplayCue(WeaponConfig.FireGameplayCueTag);
	}

	Result.bFired = true;
	Result.MagazineAmmoAfterShot = CurrentMagazineAmmo;
	return Result;
}

int32 ASWWeapon::TryCommitReloadAuthority()
{
	if (!CanReload())
	{
		return 0;
	}

	const int32 AmmoToLoad = GetEffectiveMagazineCapacity() - CurrentMagazineAmmo;
	CurrentMagazineAmmo += AmmoToLoad;
	BroadcastAmmoChanged();
	return AmmoToLoad;
}

void ASWWeapon::OnRep_CurrentMagazineAmmo(const int32 OldAmmo)
{
	BroadcastAmmoChanged();
}

void ASWWeapon::NotifyReloadStateChangedAuthority(const bool bReloading)
{
	if (HasAuthority())
	{
		BP_OnReloadStateChanged(bReloading);
		if (bReloading)
		{
			ExecuteOwnerGameplayCue(SWGameplayTags::GameplayCue_Weapon_Reload);
		}
	}
}

UMeshComponent* ASWWeapon::GetActiveWeaponMesh() const
{
	if (SkeletalWeaponMesh && SkeletalWeaponMesh->GetSkeletalMeshAsset())
	{
		return SkeletalWeaponMesh;
	}

	if (StaticWeaponMesh && StaticWeaponMesh->GetStaticMesh())
	{
		return StaticWeaponMesh;
	}

	return nullptr;
}

int32 ASWWeapon::GetEffectiveMagazineCapacity() const
{
	float CapacityBonusPercent = 0.f;
	if (const IAbilitySystemInterface* AbilitySystemOwner = Cast<IAbilitySystemInterface>(GetOwner()))
	{
		if (const UAbilitySystemComponent* AbilitySystemComponent = AbilitySystemOwner->GetAbilitySystemComponent())
		{
			CapacityBonusPercent = AbilitySystemComponent->GetNumericAttribute(USWAttributeSet::GetMagazineCapacityBonusPercentAttribute());
		}
	}

	return FMath::Max(1, FMath::FloorToInt(static_cast<float>(WeaponConfig.MagazineCapacity) * (1.f + CapacityBonusPercent)));
}

float ASWWeapon::GetEffectiveFireInterval() const
{
	float FireIntervalReductionPercent = 0.f;
	if (const IAbilitySystemInterface* AbilitySystemOwner = Cast<IAbilitySystemInterface>(GetOwner()))
	{
		if (const UAbilitySystemComponent* AbilitySystemComponent = AbilitySystemOwner->GetAbilitySystemComponent())
		{
			FireIntervalReductionPercent = AbilitySystemComponent->GetNumericAttribute(USWAttributeSet::GetFireIntervalReductionPercentAttribute());
		}
	}

	const float BaseInterval = 60.f / WeaponConfig.RoundsPerMinute;
	return BaseInterval * FMath::Clamp(1.f - FireIntervalReductionPercent, 0.01f, 1.f);
}

bool ASWWeapon::IsAiming() const
{
	if (const IAbilitySystemInterface* AbilitySystemOwner = Cast<IAbilitySystemInterface>(GetOwner()))
	{
		if (const UAbilitySystemComponent* AbilitySystemComponent = AbilitySystemOwner->GetAbilitySystemComponent())
		{
			return AbilitySystemComponent->HasMatchingGameplayTag(SWGameplayTags::State_Weapon_Aiming);
		}
	}

	return false;
}

bool ASWWeapon::BuildAuthoritativeShotDirection(const FTransform& MuzzleTransform, FVector& OutDirection) const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	const AController* Controller = OwnerPawn ? OwnerPawn->GetController() : nullptr;
	if (!OwnerPawn || !Controller || !GetWorld())
	{
		return false;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);
	const FVector ViewDirection = ViewRotation.Vector();

	FCollisionQueryParams QueryParameters(SCENE_QUERY_STAT(SWWeaponAimTrace), false, OwnerPawn);
	QueryParameters.AddIgnoredActor(this);
	// 第三人称相机射线从准星中心出发，必须忽略持枪 Pawn，避免命中自己的 Capsule 或 Mesh。
	QueryParameters.AddIgnoredActor(OwnerPawn);
	FHitResult AimHit;
	const FVector TraceEnd = ViewLocation + ViewDirection * WeaponConfig.MaxAimDistance;
	const FVector AimPoint = GetWorld()->LineTraceSingleByChannel(AimHit, ViewLocation, TraceEnd, ECC_Visibility, QueryParameters)
		? AimHit.ImpactPoint
		: TraceEnd;

	OutDirection = (AimPoint - MuzzleTransform.GetLocation()).GetSafeNormal();
	if (OutDirection.IsNearlyZero())
	{
		return false;
	}

	const float SpreadMultiplier = IsAiming() ? WeaponConfig.AimSpreadMultiplier : 1.f;
	const float SpreadRadians = FMath::DegreesToRadians(WeaponConfig.HipSpreadDegrees * SpreadMultiplier);
	OutDirection = FMath::VRandCone(OutDirection, SpreadRadians);
	return true;
}

void ASWWeapon::BroadcastAmmoChanged()
{
	OnAmmoChanged.Broadcast(CurrentMagazineAmmo);
}

void ASWWeapon::ExecuteOwnerGameplayCue(const FGameplayTag CueTag) const
{
	if (!CueTag.IsValid())
	{
		return;
	}

	if (const IAbilitySystemInterface* AbilitySystemOwner = Cast<IAbilitySystemInterface>(GetOwner()))
	{
		if (UAbilitySystemComponent* AbilitySystemComponent = AbilitySystemOwner->GetAbilitySystemComponent())
		{
			AbilitySystemComponent->ExecuteGameplayCue(CueTag);
		}
	}
}
