// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "Weapon/SWProjectile.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/SWAbilitySystemComponent.h"
#include "Collision/SWCollisionChannels.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameplayTags/SWGameplayTags.h"
#include "Interaction/SWCombatInterface.h"

ASWProjectile::ASWProjectile()
{
	bReplicates = true;
	SetReplicateMovement(true);
	SetCanBeDamaged(false);
	InitialLifeSpan = 0.f;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	CollisionComponent->InitSphereRadius(5.f);
	ConfigureAuthorityCollision();
	RootComponent = CollisionComponent;

	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	ProjectileMesh->SetupAttachment(CollisionComponent);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionComponent;
	ProjectileMovement->bAutoActivate = false;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;

	CollisionComponent->OnComponentHit.AddDynamic(this, &ThisClass::OnProjectileHit);
	CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnProjectileBeginOverlap);
}

void ASWProjectile::ConfigureAuthorityCollision()
{
	// 弹丸属于 Projectile 对象类型。世界阻挡它；Pawn 与屏障等对象可通过 Projectile 默认重叠接收事件。
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComponent->SetCollisionObjectType(SWCollisionChannels::Projectile);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComponent->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CollisionComponent->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	CollisionComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	CollisionComponent->SetCollisionResponseToChannel(SWCollisionChannels::Projectile, ECR_Overlap);
	CollisionComponent->SetCollisionResponseToChannel(SWCollisionChannels::ShieldBarrier, ECR_Overlap);
	CollisionComponent->SetNotifyRigidBodyCollision(true);
	CollisionComponent->SetGenerateOverlapEvents(true);
}

void ASWProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

bool ASWProjectile::InitializeProjectileAuthority(APawn* InInstigatorPawn, const FVector& LaunchDirection,
	const TSubclassOf<USWDamageGameplayEffect> InDamageEffectClass, const FSWDamageApplicationParams& InDamageParams)
{
	if (!HasAuthority() || bInitialized || !ProjectileConfig.IsValid() || !InInstigatorPawn || !InDamageEffectClass
		|| !FMath::IsFinite(InDamageParams.RawDamage) || InDamageParams.RawDamage <= 0.f)
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
	// 运行时重设，防止已有蓝图子类保存的旧碰撞覆盖值破坏 Projectile 通道契约。
	ConfigureAuthorityCollision();
	DamageEffectClass = InDamageEffectClass;
	DamageParams = InDamageParams;
	CollisionComponent->IgnoreActorWhenMoving(InInstigatorPawn, true);
	CollisionComponent->SetSphereRadius(ProjectileConfig.CollisionRadius, true);
	ProjectileMovement->InitialSpeed = ProjectileConfig.InitialSpeed;
	ProjectileMovement->MaxSpeed = ProjectileConfig.MaxSpeed > 0.f ? ProjectileConfig.MaxSpeed : ProjectileConfig.InitialSpeed;
	ProjectileMovement->ProjectileGravityScale = ProjectileConfig.GravityScale;
	ProjectileMovement->bRotationFollowsVelocity = ProjectileConfig.bRotationFollowsVelocity;
	ProjectileMovement->bShouldBounce = ProjectileConfig.bShouldBounce;
	ProjectileMovement->Bounciness = ProjectileConfig.Bounciness;
	ProjectileMovement->Velocity = NormalizedDirection * ProjectileConfig.InitialSpeed;
	ProjectileMovement->Activate(true);
	SetLifeSpan(ProjectileConfig.LifeSeconds);
	bInitialized = true;

	BP_OnProjectileLaunched();
	return true;
}

FVector ASWProjectile::GetLaunchVelocity() const
{
	return ProjectileMovement ? ProjectileMovement->Velocity : FVector::ZeroVector;
}

bool ASWProjectile::CanBeAbsorbedByShield_Implementation() const
{
	return bInitialized && !bImpactHandled;
}

void ASWProjectile::AbsorbByShieldAuthority_Implementation(AActor* ShieldActor)
{
	if (HasAuthority() && CanBeAbsorbedByShield_Implementation())
	{
		Destroy();
	}
}

void ASWProjectile::OnProjectileHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent,
	const FVector NormalImpulse, const FHitResult& Hit)
{
	if (HasAuthority())
	{
		HandleAuthoritativeImpact(OtherActor, Hit);
	}
}

void ASWProjectile::OnProjectileBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, const int32 OtherBodyIndex, const bool bFromSweep, const FHitResult& SweepResult)
{
	// Pawn 对 Projectile 的默认响应为重叠；仍由服务器在这里执行原有命中伤害。
	if (HasAuthority() && OtherActor && OtherActor != GetInstigator() && OtherActor->IsA<APawn>())
	{
		HandleAuthoritativeImpact(OtherActor, SweepResult);
	}
}

void ASWProjectile::HandleAuthoritativeImpact(AActor* const HitActor, const FHitResult& Hit)
{
	if (bImpactHandled)
	{
		return;
	}

	bImpactHandled = true;
	BP_OnProjectileImpact(Hit);

	if (HitActor)
	{
		ApplyDamageEffectAuthority(HitActor);

		FGameplayEventData ImpactEvent;
		ImpactEvent.EventTag = SWGameplayTags::Event_Weapon_ProjectileImpact;
		ImpactEvent.Instigator = GetInstigator();
		ImpactEvent.Target = HitActor;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(HitActor, ImpactEvent.EventTag, ImpactEvent);
	}

	FGameplayCueParameters ImpactCueParameters;
	ImpactCueParameters.Location = Hit.ImpactPoint;
	ImpactCueParameters.Normal = Hit.ImpactNormal;
	ImpactCueParameters.EffectCauser = this;
	if (IAbilitySystemInterface* InstigatorAbilitySystemOwner = Cast<IAbilitySystemInterface>(GetInstigator()))
	{
		if (UAbilitySystemComponent* AbilitySystemComponent = InstigatorAbilitySystemOwner->GetAbilitySystemComponent())
		{
			AbilitySystemComponent->ExecuteGameplayCue(SWGameplayTags::GameplayCue_Projectile_Impact, ImpactCueParameters);
		}
	}

	Destroy();
}

bool ASWProjectile::ApplyDamageEffectAuthority(AActor* const HitActor)
{
	if (!HasAuthority() || !HitActor || !DamageEffectClass)
	{
		return false;
	}

	APawn* const InstigatorPawn = GetInstigator();
	UAbilitySystemComponent* const SourceAbilitySystemComponent = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InstigatorPawn);
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
	if (InstigatorPawn && InstigatorPawn->Implements<USWCombatInterface>())
	{
		EffectLevel = FMath::Max(1, ISWCombatInterface::Execute_GetCombatLevel(InstigatorPawn));
	}

	return SourceSWAbilitySystemComponent->ApplyDamageEffectToTargetAuthority(
		TargetAbilitySystemComponent,
		DamageEffectClass,
		EffectLevel,
		this,
		DamageParams);
}
