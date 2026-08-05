// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "Weapon/SWProjectile.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/SWAbilitySystemComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
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
	CollisionComponent->SetCollisionProfileName(TEXT("BlockAll"));
	CollisionComponent->SetNotifyRigidBodyCollision(true);
	CollisionComponent->SetGenerateOverlapEvents(false);
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
}

void ASWProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

bool ASWProjectile::InitializeProjectileAuthority(APawn* InInstigatorPawn, const FVector& LaunchDirection)
{
	if (!HasAuthority() || bInitialized || !ProjectileConfig.IsValid() || !InInstigatorPawn)
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

void ASWProjectile::OnProjectileHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent,
	const FVector NormalImpulse, const FHitResult& Hit)
{
	if (HasAuthority())
	{
		HandleAuthoritativeImpact(Hit);
	}
}

void ASWProjectile::HandleAuthoritativeImpact(const FHitResult& Hit)
{
	if (bImpactHandled)
	{
		return;
	}

	bImpactHandled = true;
	BP_OnProjectileImpact(Hit);

	if (AActor* HitActor = Hit.GetActor())
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
	if (!HasAuthority() || !HitActor || !ProjectileConfig.DamageEffectClass)
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
		ProjectileConfig.DamageEffectClass,
		EffectLevel,
		this);
}
