// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "Weapon/SWWeapon.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/SWAbilitySystemComponent.h"
#include "AbilitySystem/SWAttributeSet.h"
#include "Animation/AnimMontage.h"
#include "Components/MeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameplayTags/SWGameplayTags.h"
#include "Interaction/SWCombatInterface.h"
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
	BindMagazineCapacityMultiplierAuthority();
	BroadcastAmmoChanged();
}

void ASWWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(ASWWeapon, CurrentMagazineAmmo, COND_OwnerOnly, REPNOTIFY_Always);
}

bool ASWWeapon::CanFire() const
{
	// 射击节奏完全由 FireCycle Section 的播放推进决定，Weapon 只验证可射击状态。
	return WeaponConfig.IsValidForFire() && CurrentMagazineAmmo > 0;
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

float ASWWeapon::GetEffectiveFireMontagePlayRate(const float BasePlayRate) const
{
	return FMath::Max(0.01f, BasePlayRate) / GetFireIntervalMultiplier();
}

bool ASWWeapon::ResolveFireMontageSelection(const int32 RequestedVariantIndex, FSWFireMontageSelection& OutSelection) const
{
	OutSelection = FSWFireMontageSelection();

	if (WeaponConfig.FireMontageVariants.IsEmpty())
	{
		return false;
	}

	const int32 FirstCandidateIndex = WeaponConfig.FireMontageSelectionMode == ESWFireMontageSelectionMode::FirstValid
		? 0
		: FMath::Abs(RequestedVariantIndex) % WeaponConfig.FireMontageVariants.Num();

	for (int32 Offset = 0; Offset < WeaponConfig.FireMontageVariants.Num(); ++Offset)
	{
		const int32 CandidateIndex = (FirstCandidateIndex + Offset) % WeaponConfig.FireMontageVariants.Num();
		const FSWFireMontageVariant& Candidate = WeaponConfig.FireMontageVariants[CandidateIndex];
		if (!Candidate.Montage)
		{
			continue;
		}

		OutSelection.Montage = Candidate.Montage;
		OutSelection.StartSection = Candidate.StartSection;
		OutSelection.EffectivePlayRate = GetEffectiveFireMontagePlayRate(Candidate.PlayRate);
		OutSelection.VariantIndex = CandidateIndex;
		OutSelection.bValid = true;
		return true;
	}

	return false;
}

FSWResolvedShot ASWWeapon::TryFireAuthority()
{
	FSWResolvedShot Result;
	Result.Mode = WeaponConfig.ShotResolutionMode;
	Result.MagazineAmmoAfterShot = CurrentMagazineAmmo;

	if (!HasAuthority() || !CanFire())
	{
		return Result;
	}

	FTransform MuzzleTransform;
	FVector ShotDirection;
	FVector TraceEnd;
	if (!GetMuzzleTransform(MuzzleTransform) || !BuildAuthoritativeShotQuery(MuzzleTransform, ShotDirection, TraceEnd))
	{
		UE_LOG(LogTemp, Warning, TEXT("武器 %s 缺少有效枪口或瞄准上下文，本次射击被拒绝。"), *GetName());
		return Result;
	}

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	Result.MuzzleTransform = MuzzleTransform;
	Result.TraceEnd = TraceEnd;

	switch (WeaponConfig.ShotResolutionMode)
	{
	case ESWShotResolutionMode::Projectile:
		if (!ResolveProjectileAuthority(OwnerPawn, MuzzleTransform, ShotDirection))
		{
			return Result;
		}
		break;

	case ESWShotResolutionMode::Hitscan:
		ResolveHitscanAuthority(OwnerPawn, MuzzleTransform.GetLocation(), TraceEnd, Result);
		break;

	default:
		return Result;
	}

	CurrentMagazineAmmo = FMath::Max(0, CurrentMagazineAmmo - 1);
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
	float CapacityMultiplier = 1.f;
	if (const IAbilitySystemInterface* AbilitySystemOwner = Cast<IAbilitySystemInterface>(GetOwner()))
	{
		if (const UAbilitySystemComponent* AbilitySystemComponent = AbilitySystemOwner->GetAbilitySystemComponent())
		{
			CapacityMultiplier = AbilitySystemComponent->GetNumericAttribute(USWAttributeSet::GetMagazineCapacityMultiplierAttribute());
		}
	}

	return FMath::Max(1, FMath::FloorToInt(static_cast<float>(WeaponConfig.MagazineCapacity) * FMath::Max(0.f, CapacityMultiplier)));
}

float ASWWeapon::GetBaseFireCycleDurationSeconds() const
{
	for (const FSWFireMontageVariant& Variant : WeaponConfig.FireMontageVariants)
	{
		const UAnimMontage* Montage = Variant.Montage;
		if (!Montage)
		{
			continue;
		}

		const int32 FireCycleSectionIndex = Montage->GetSectionIndex(TEXT("FireCycle"));
		if (FireCycleSectionIndex == INDEX_NONE)
		{
			continue;
		}

		float StartTime = 0.f;
		float EndTime = 0.f;
		Montage->GetSectionStartAndEndTime(FireCycleSectionIndex, StartTime, EndTime);
		const float BasePlayRate = FMath::Max(0.01f, Variant.PlayRate);
		const float DurationSeconds = (EndTime - StartTime) / BasePlayRate;
		if (DurationSeconds > KINDA_SMALL_NUMBER)
		{
			return DurationSeconds;
		}
	}

	return 0.f;
}

float ASWWeapon::GetEffectiveRoundsPerMinute() const
{
	const float BaseFireCycleDuration = GetBaseFireCycleDurationSeconds();
	if (BaseFireCycleDuration <= KINDA_SMALL_NUMBER)
	{
		return 0.f;
	}

	const float EffectiveFireCycleDuration = BaseFireCycleDuration * GetFireIntervalMultiplier();
	return 60.f / EffectiveFireCycleDuration;
}

float ASWWeapon::GetFireIntervalMultiplier() const
{
	float FireIntervalReductionPercent = 0.f;
	if (const IAbilitySystemInterface* AbilitySystemOwner = Cast<IAbilitySystemInterface>(GetOwner()))
	{
		if (const UAbilitySystemComponent* AbilitySystemComponent = AbilitySystemOwner->GetAbilitySystemComponent())
		{
			FireIntervalReductionPercent = AbilitySystemComponent->GetNumericAttribute(USWAttributeSet::GetFireIntervalReductionPercentAttribute());
		}
	}

	return FMath::Clamp(1.f - FireIntervalReductionPercent, 0.01f, 1.f);
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

bool ASWWeapon::BuildAuthoritativeShotQuery(const FTransform& MuzzleTransform, FVector& OutDirection, FVector& OutTraceEnd) const
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
	const FVector ViewTraceEnd = ViewLocation + ViewDirection * WeaponConfig.MaxAimDistance;
	const FVector AimPoint = GetWorld()->LineTraceSingleByChannel(AimHit, ViewLocation, ViewTraceEnd, ECC_GameTraceChannel1, QueryParameters)
		? AimHit.ImpactPoint
		: ViewTraceEnd;

	OutDirection = (AimPoint - MuzzleTransform.GetLocation()).GetSafeNormal();
	if (OutDirection.IsNearlyZero())
	{
		return false;
	}

	const float SpreadMultiplier = IsAiming() ? WeaponConfig.AimSpreadMultiplier : 1.f;
	const float SpreadRadians = FMath::DegreesToRadians(WeaponConfig.HipSpreadDegrees * SpreadMultiplier);
	OutDirection = FMath::VRandCone(OutDirection, SpreadRadians);
	OutTraceEnd = MuzzleTransform.GetLocation() + OutDirection * WeaponConfig.MaxAimDistance;
	return true;
}

bool ASWWeapon::ResolveProjectileAuthority(APawn* const OwnerPawn, const FTransform& MuzzleTransform, const FVector& ShotDirection)
{
	ASWProjectile* const Projectile = GetWorld()->SpawnActorDeferred<ASWProjectile>(WeaponConfig.ProjectileClass, MuzzleTransform, OwnerPawn, OwnerPawn,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Projectile)
	{
		UE_LOG(LogTemp, Warning, TEXT("武器 %s 未能生成权威弹丸，本次射击未扣弹。"), *GetName());
		return false;
	}

	UGameplayStatics::FinishSpawningActor(Projectile, MuzzleTransform);
	if (Projectile->InitializeProjectileAuthority(OwnerPawn, ShotDirection, WeaponConfig.DamageEffectClass))
	{
		return true;
	}

	Projectile->Destroy();
	return false;
}

void ASWWeapon::ResolveHitscanAuthority(APawn* const OwnerPawn, const FVector& TraceStart, const FVector& TraceEnd,
	FSWResolvedShot& InOutResult)
{
	FCollisionQueryParams QueryParameters(SCENE_QUERY_STAT(SWWeaponHitscanTrace), false, OwnerPawn);
	QueryParameters.AddIgnoredActor(this);
	QueryParameters.AddIgnoredActor(OwnerPawn);

	InOutResult.bBlockingHit = GetWorld()->LineTraceSingleByChannel(
		InOutResult.HitResult,
		TraceStart,
		TraceEnd,
		ECC_GameTraceChannel1,
		QueryParameters);

	if (InOutResult.bBlockingHit)
	{
		if (AActor* const HitActor = InOutResult.HitResult.GetActor())
		{
			ApplyDamageEffectAuthority(HitActor);
		}
	}
}

bool ASWWeapon::ApplyDamageEffectAuthority(AActor* const HitActor)
{
	if (!HasAuthority() || !HitActor || !WeaponConfig.DamageEffectClass)
	{
		return false;
	}

	APawn* const OwnerPawn = Cast<APawn>(GetOwner());
	UAbilitySystemComponent* const SourceAbilitySystemComponent = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerPawn);
	UAbilitySystemComponent* const TargetAbilitySystemComponent = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor);
	USWAbilitySystemComponent* const SourceSWAbilitySystemComponent = Cast<USWAbilitySystemComponent>(SourceAbilitySystemComponent);
	if (!SourceSWAbilitySystemComponent || !TargetAbilitySystemComponent)
	{
		return false;
	}

	if (HitActor->Implements<USWCombatInterface>() && ISWCombatInterface::Execute_IsDead(HitActor))
	{
		return false;
	}

	int32 EffectLevel = 1;
	if (OwnerPawn && OwnerPawn->Implements<USWCombatInterface>())
	{
		EffectLevel = FMath::Max(1, ISWCombatInterface::Execute_GetCombatLevel(OwnerPawn));
	}

	return SourceSWAbilitySystemComponent->ApplyDamageEffectToTargetAuthority(
		TargetAbilitySystemComponent,
		WeaponConfig.DamageEffectClass,
		EffectLevel,
		this);
}

void ASWWeapon::BroadcastAmmoChanged()
{
	OnAmmoChanged.Broadcast(CurrentMagazineAmmo);
}

void ASWWeapon::BindMagazineCapacityMultiplierAuthority()
{
	if (!HasAuthority())
	{
		return;
	}

	if (const IAbilitySystemInterface* AbilitySystemOwner = Cast<IAbilitySystemInterface>(GetOwner()))
	{
		if (UAbilitySystemComponent* AbilitySystemComponent = AbilitySystemOwner->GetAbilitySystemComponent())
		{
			AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(USWAttributeSet::GetMagazineCapacityMultiplierAttribute())
				.AddUObject(this, &ASWWeapon::HandleMagazineCapacityMultiplierChanged);
		}
	}
}

void ASWWeapon::HandleMagazineCapacityMultiplierChanged(const FOnAttributeChangeData& ChangeData)
{
	(void)ChangeData;

	if (!HasAuthority())
	{
		return;
	}

	const int32 EffectiveCapacity = GetEffectiveMagazineCapacity();
	if (CurrentMagazineAmmo > EffectiveCapacity)
	{
		// 容量下降不返还溢出弹药；M04 的无限备弹规则下直接截断即可。
		CurrentMagazineAmmo = EffectiveCapacity;
		BroadcastAmmoChanged();
	}
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
