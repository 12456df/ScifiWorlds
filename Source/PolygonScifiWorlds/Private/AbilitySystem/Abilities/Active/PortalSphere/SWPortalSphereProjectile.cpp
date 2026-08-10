// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "AbilitySystem/Abilities/Active/PortalSphere/SWPortalSphereProjectile.h"

#include "AbilitySystem/Effects/SWPortalSphereDamageGameplayEffect.h"
#include "AbilitySystem/SWAbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Collision/SWCollisionChannels.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Interaction/SWCombatInterface.h"
#include "Net/UnrealNetwork.h"
#include "Vector"

ASWPortalSphereProjectile::ASWPortalSphereProjectile()
{
	bReplicates = true;
	SetReplicateMovement(true);
	SetCanBeDamaged(false);
	PrimaryActorTick.bCanEverTick = false;

	BlockingSphere = CreateDefaultSubobject<USphereComponent>(TEXT("BlockingSphere"));
	BlockingSphere->InitSphereRadius(10.f);
	// 小球只用于命中世界障碍物：阻挡移动扫掠，但对 Pawn 始终是重叠，不能成为可站立实体。
	BlockingSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	BlockingSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	BlockingSphere->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	BlockingSphere->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	BlockingSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	BlockingSphere->SetCollisionResponseToChannel(SWCollisionChannels::Projectile, ECR_Overlap);
	BlockingSphere->SetCollisionResponseToChannel(SWCollisionChannels::ShieldBarrier, ECR_Overlap);
	BlockingSphere->SetCollisionObjectType(SWCollisionChannels::Projectile);
	BlockingSphere->SetNotifyRigidBodyCollision(true);
	BlockingSphere->SetGenerateOverlapEvents(true);
	RootComponent = BlockingSphere;

	DamageSphere = CreateDefaultSubobject<USphereComponent>(TEXT("DamageSphere"));
	DamageSphere->InitSphereRadius(100.f);
	DamageSphere->SetupAttachment(BlockingSphere);
	DamageSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DamageSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	DamageSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	DamageSphere->SetCollisionObjectType(SWCollisionChannels::Projectile);
	DamageSphere->SetGenerateOverlapEvents(true);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = BlockingSphere;
	ProjectileMovement->bAutoActivate = false;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->ProjectileGravityScale = 0.f;
	ProjectileMovement->bShouldBounce = false;

	BlockingSphere->OnComponentHit.AddDynamic(this, &ThisClass::OnBlockingSphereHit);
	BlockingSphere->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnBlockingSphereBeginOverlap);
	BlockingSphere->OnComponentEndOverlap.AddDynamic(this, &ThisClass::OnBlockingSphereEndOverlap);
	DamageSphere->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnDamageSphereBeginOverlap);
	DamageSphere->OnComponentEndOverlap.AddDynamic(this, &ThisClass::OnDamageSphereEndOverlap);
}

void ASWPortalSphereProjectile::ConfigureAuthorityCollision()
{
	// 小球仅扫描并阻挡世界；Pawn 一律重叠，所以不会成为可站立的实体。
	BlockingSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	BlockingSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	BlockingSphere->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	BlockingSphere->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	BlockingSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	BlockingSphere->SetCollisionResponseToChannel(SWCollisionChannels::Projectile, ECR_Overlap);
	BlockingSphere->SetCollisionResponseToChannel(SWCollisionChannels::ShieldBarrier, ECR_Overlap);
	BlockingSphere->SetCollisionObjectType(SWCollisionChannels::Projectile);
	BlockingSphere->SetNotifyRigidBodyCollision(true);
	BlockingSphere->SetGenerateOverlapEvents(true);

	// 大球只关心可受伤的 Pawn；它不参与物理碰撞，也不会挡住其他动态物体。
	DamageSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DamageSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	DamageSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	DamageSphere->SetCollisionObjectType(SWCollisionChannels::Projectile);
	DamageSphere->SetGenerateOverlapEvents(true);
}

void ASWPortalSphereProjectile::RefreshProjectileSpeedAuthority()
{
	if (!HasAuthority() || bReachedMaximumRange || !ProjectileMovement || BaseProjectileSpeed <= 0.f)
	{
		return;
	}

	const float SpeedMultiplier = SlowingOverlapComponents.IsEmpty() ? 1.f : FMath::Clamp(OverlapSpeedMultiplier, 0.01f, 1.f);
	const float TargetSpeed = BaseProjectileSpeed * SpeedMultiplier;
	const FVector TravelDirection = ProjectileMovement->Velocity.IsNearlyZero()
		? LaunchVelocity.GetSafeNormal()
		: ProjectileMovement->Velocity.GetSafeNormal();
	ProjectileMovement->MaxSpeed = TargetSpeed;
	ProjectileMovement->Velocity = TravelDirection * TargetSpeed;
	ScheduleRangeStopAuthority();
}

void ASWPortalSphereProjectile::ScheduleRangeStopAuthority()
{
	if (!HasAuthority() || bReachedMaximumRange || !ProjectileMovement || MaximumTravelDistance <= 0.f)
	{
		return;
	}

	const float TravelDistance = FVector::DotProduct(GetActorLocation() - LaunchStartLocation, InitialLaunchDirection);
	const float RemainingDistance = MaximumTravelDistance - TravelDistance;
	if (RemainingDistance <= KINDA_SMALL_NUMBER)
	{
		StopAtMaximumRangeAuthority();
		return;
	}

	const float CurrentSpeed = ProjectileMovement->Velocity.Size();
	if (CurrentSpeed <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(RangeStopTimer);
	GetWorldTimerManager().SetTimer(RangeStopTimer, this, &ThisClass::StopAtMaximumRangeAuthority,
		RemainingDistance / CurrentSpeed, false);
}

void ASWPortalSphereProjectile::StopAtMaximumRangeAuthority()
{
	if (!HasAuthority() || bReachedMaximumRange)
	{
		return;
	}

	bReachedMaximumRange = true;
	GetWorldTimerManager().ClearTimer(RangeStopTimer);
	// 最大距离仅终止移动。不能传送至理论终点，否则会绕过已发生的世界阻挡。
	ProjectileMovement->StopMovementImmediately();
	ProjectileMovement->Deactivate();
	ForceNetUpdate();
}

void ASWPortalSphereProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		BlockingSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		DamageSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		BP_OnPortalSphereVisualScaleChanged(VisualScale);
		return;
	}

	if (bInitialized && !bReachedMaximumRange && !LaunchVelocity.IsNearlyZero())
	{
		// Deferred Spawn 的蓝图构造流程已结束；在移动激活前恢复权威碰撞与扫掠配置。
		ConfigureAuthorityCollision();
		ProjectileMovement->SetUpdatedComponent(BlockingSphere);
		ProjectileMovement->bSweepCollision = true;
		ProjectileMovement->bShouldBounce = false;
		ProjectileMovement->ProjectileGravityScale = 0.f;
		ProjectileMovement->Velocity = LaunchVelocity;
		ProjectileMovement->Activate(true);
		ScheduleRangeStopAuthority();
		BP_OnPortalSphereLaunched();
	}
}

void ASWPortalSphereProjectile::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(RangeStopTimer);
	ClearDamageEffectsAuthority();
	Super::EndPlay(EndPlayReason);
}

void ASWPortalSphereProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASWPortalSphereProjectile, VisualScale);
}

bool ASWPortalSphereProjectile::InitializePortalSphereAuthority(
	APawn* const InInstigatorPawn,
	const FVector LaunchDirection,
	const TSubclassOf<USWPortalSphereDamageGameplayEffect> InDamageEffectClass,
	const FSWDamageApplicationParams& InDamageParams,
	const float InProjectileSpeed,
	const float InMaximumRange,
	const float InDurationSeconds,
	const float InBlockingSphereRadius,
	const float InDamageSphereRadius,
	const float InVisualScale)
{
	if (!HasAuthority() || bInitialized || !InInstigatorPawn || !InDamageEffectClass
		|| !FMath::IsFinite(InDamageParams.RawDamage) || InDamageParams.RawDamage <= 0.f
		|| !FMath::IsFinite(InProjectileSpeed) || InProjectileSpeed <= 0.f
		|| !FMath::IsFinite(InMaximumRange) || InMaximumRange <= 0.f
		|| !FMath::IsFinite(InDurationSeconds) || InDurationSeconds <= 0.f
		|| !FMath::IsFinite(InBlockingSphereRadius) || InBlockingSphereRadius <= 0.f
		|| !FMath::IsFinite(InDamageSphereRadius) || InDamageSphereRadius < InBlockingSphereRadius
		|| !FMath::IsFinite(InVisualScale) || InVisualScale <= 0.f)
	{
		return false;
	}

	const FVector NormalizedDirection = LaunchDirection.GetSafeNormal();
	if (NormalizedDirection.IsNearlyZero())
	{
		return false;
	}

	SetInstigator(InInstigatorPawn);
	SetOwner(InInstigatorPawn);
	// 强制恢复 C++ 的权威碰撞契约，避免已有 BP 子类保存的旧组件覆盖值继续产生实体碰撞。
	ConfigureAuthorityCollision();
	DamageEffectClass = InDamageEffectClass;
	DamageParams = InDamageParams;
	DamageParams.EffectDurationSeconds = InDurationSeconds;
	VisualScale = InVisualScale;
	BlockingSphere->SetSphereRadius(InBlockingSphereRadius, true);
	DamageSphere->SetSphereRadius(InDamageSphereRadius, true);
	BlockingSphere->IgnoreActorWhenMoving(InInstigatorPawn, true);
	DamageSphere->IgnoreActorWhenMoving(InInstigatorPawn, true);

	ProjectileMovement->InitialSpeed = InProjectileSpeed;
	ProjectileMovement->MaxSpeed = InProjectileSpeed;
	BaseProjectileSpeed = InProjectileSpeed;
	LaunchVelocity = NormalizedDirection * InProjectileSpeed;
	LaunchStartLocation = GetActorLocation();
	InitialLaunchDirection = NormalizedDirection;
	MaximumTravelDistance = InMaximumRange;
	// Duration 是唯一的销毁计时；Range 通过单次计时器限制移动终点，命中终点后保留伤害区域。
	SetLifeSpan(InDurationSeconds);
	bInitialized = true;
	BP_OnPortalSphereVisualScaleChanged(VisualScale);

	// 弹体若在生成时已覆盖目标，半径更新不一定再次触发 BeginOverlap，因此主动补一次服务器检测。
	TArray<AActor*> InitialOverlaps;
	DamageSphere->GetOverlappingActors(InitialOverlaps);
	for (AActor* const OverlappingActor : InitialOverlaps)
	{
		OnDamageSphereBeginOverlap(DamageSphere, OverlappingActor, nullptr, INDEX_NONE, false, FHitResult());
	}

	return true;
}

bool ASWPortalSphereProjectile::CanBeAbsorbedByShield_Implementation() const
{
	return bInitialized;
}

void ASWPortalSphereProjectile::AbsorbByShieldAuthority_Implementation(AActor* ShieldActor)
{
	if (HasAuthority() && bInitialized)
	{
		Destroy();
	}
}

void ASWPortalSphereProjectile::OnRep_VisualScale()
{
	BP_OnPortalSphereVisualScaleChanged(VisualScale);
}

void ASWPortalSphereProjectile::OnBlockingSphereHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!HasAuthority() || bReachedMaximumRange || OtherActor == GetInstigator())
	{
		return;
	}

	// 命中世界阻挡后固定在实际命中位置，持续伤害区域继续存在至寿命结束。
	bReachedMaximumRange = true;
	GetWorldTimerManager().ClearTimer(RangeStopTimer);
	ProjectileMovement->StopMovementImmediately();
	ProjectileMovement->Deactivate();
	BP_OnPortalSphereBlocked(Hit);
	ForceNetUpdate();
}

void ASWPortalSphereProjectile::OnBlockingSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, const int32 OtherBodyIndex, const bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || !bInitialized || bReachedMaximumRange || !OtherActor || OtherActor == GetInstigator() || !OtherComponent)
	{
		return;
	}

	SlowingOverlapComponents.Add(OtherComponent);
	RefreshProjectileSpeedAuthority();
}

void ASWPortalSphereProjectile::OnBlockingSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, const int32 OtherBodyIndex)
{
	if (!HasAuthority() || !OtherComponent)
	{
		return;
	}

	SlowingOverlapComponents.Remove(OtherComponent);
	RefreshProjectileSpeedAuthority();
}

void ASWPortalSphereProjectile::OnDamageSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, const int32 OtherBodyIndex, const bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || !bInitialized || !OtherActor || OtherActor == GetInstigator())
	{
		return;
	}

	const TWeakObjectPtr<AActor> TargetKey(OtherActor);
	if (ActiveDamageEffects.Contains(TargetKey))
	{
		return;
	}

	const FActiveGameplayEffectHandle EffectHandle = ApplyDamageEffectAuthority(OtherActor);
	if (EffectHandle.IsValid())
	{
		ActiveDamageEffects.Add(TargetKey, EffectHandle);
	}
}

void ASWPortalSphereProjectile::OnDamageSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, const int32 OtherBodyIndex)
{
	if (!HasAuthority() || !OtherActor)
	{
		return;
	}

	RemoveDamageEffectAuthority(OtherActor);
}

FActiveGameplayEffectHandle ASWPortalSphereProjectile::ApplyDamageEffectAuthority(AActor* const TargetActor)
{
	if (!HasAuthority() || !TargetActor || !DamageEffectClass)
	{
		return FActiveGameplayEffectHandle();
	}

	APawn* const InstigatorPawn = GetInstigator();
	UAbilitySystemComponent* const SourceAbilitySystemComponent = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InstigatorPawn);
	UAbilitySystemComponent* const TargetAbilitySystemComponent = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	USWAbilitySystemComponent* const SourceSWAbilitySystemComponent = Cast<USWAbilitySystemComponent>(SourceAbilitySystemComponent);
	if (!SourceSWAbilitySystemComponent || !TargetAbilitySystemComponent)
	{
		return FActiveGameplayEffectHandle();
	}

	if (TargetActor->Implements<USWCombatInterface>() && ISWCombatInterface::Execute_IsDead(TargetActor))
	{
		return FActiveGameplayEffectHandle();
	}

	const int32 EffectLevel = InstigatorPawn && InstigatorPawn->Implements<USWCombatInterface>()
		? FMath::Max(1, ISWCombatInterface::Execute_GetCombatLevel(InstigatorPawn))
		: 1;
	return SourceSWAbilitySystemComponent->ApplyDamageEffectToTargetWithHandleAuthority(
		TargetAbilitySystemComponent,
		DamageEffectClass,
		EffectLevel,
		this,
		DamageParams);
}

void ASWPortalSphereProjectile::RemoveDamageEffectAuthority(AActor* const TargetActor)
{
	if (!HasAuthority() || !TargetActor)
	{
		return;
	}

	const TWeakObjectPtr<AActor> TargetKey(TargetActor);
	const FActiveGameplayEffectHandle* const EffectHandle = ActiveDamageEffects.Find(TargetKey);
	if (!EffectHandle)
	{
		return;
	}

	if (UAbilitySystemComponent* const TargetAbilitySystemComponent = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor))
	{
		TargetAbilitySystemComponent->RemoveActiveGameplayEffect(*EffectHandle);
	}
	ActiveDamageEffects.Remove(TargetKey);
}

void ASWPortalSphereProjectile::ClearDamageEffectsAuthority()
{
	if (!HasAuthority())
	{
		return;
	}

	for (const TPair<TWeakObjectPtr<AActor>, FActiveGameplayEffectHandle>& Pair : ActiveDamageEffects)
	{
		if (AActor* const TargetActor = Pair.Key.Get())
		{
			if (UAbilitySystemComponent* const TargetAbilitySystemComponent = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor))
			{
				TargetAbilitySystemComponent->RemoveActiveGameplayEffect(Pair.Value);
			}
		}
	}
	ActiveDamageEffects.Reset();
}
