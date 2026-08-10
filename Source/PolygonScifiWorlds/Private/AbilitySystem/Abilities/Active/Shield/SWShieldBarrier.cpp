// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "AbilitySystem/Abilities/Active/Shield/SWShieldBarrier.h"

#include "Collision/SWCollisionChannels.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "Interaction/SWProjectileAbsorptionInterface.h"
#include "Interaction/SWTeamInterface.h"
#include "Net/UnrealNetwork.h"

ASWShieldBarrier::ASWShieldBarrier()
{
	bReplicates = true;
	SetReplicateMovement(false);
	SetCanBeDamaged(false);
	PrimaryActorTick.bCanEverTick = false;

	AbsorptionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("AbsorptionVolume"));
	AbsorptionVolume->SetBoxExtent(FVector(50.f));
	ConfigureAuthorityCollision();
	RootComponent = AbsorptionVolume;
	AbsorptionVolume->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnAbsorptionVolumeBeginOverlap);
}

void ASWShieldBarrier::ConfigureAuthorityCollision()
{
	AbsorptionVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	AbsorptionVolume->SetCollisionObjectType(SWCollisionChannels::ShieldBarrier);
	AbsorptionVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	AbsorptionVolume->SetCollisionResponseToChannel(SWCollisionChannels::Projectile, ECR_Overlap);
	AbsorptionVolume->SetGenerateOverlapEvents(true);
}

FVector ASWShieldBarrier::GetDefaultAbsorptionBoxExtent() const
{
	return AbsorptionVolume ? AbsorptionVolume->GetUnscaledBoxExtent() : FVector::ZeroVector;
}

FVector ASWShieldBarrier::GetAreaScaleVector() const
{
	// X 是屏障厚度；范围加成只能扩展横向与纵向的覆盖面。
	return FVector(1.f, AreaScale, AreaScale);
}

bool ASWShieldBarrier::InitializeShieldAuthority(APawn* InInstigatorPawn, const ESWTeamId InTeamId,
	const float InDurationSeconds, const float InAreaScale)
{
	if (!HasAuthority() || bInitialized || !InInstigatorPawn
		|| (InTeamId != ESWTeamId::TeamA && InTeamId != ESWTeamId::TeamB)
		|| !FMath::IsFinite(InDurationSeconds) || InDurationSeconds <= 0.f
		|| !FMath::IsFinite(InAreaScale) || InAreaScale <= 0.f)
	{
		return false;
	}

	SetInstigator(InInstigatorPawn);
	SetOwner(InInstigatorPawn);
	ShieldTeamId = InTeamId;
	AreaScale = InAreaScale;
	DurationSeconds = InDurationSeconds;
	EndServerTimeSeconds = GetWorld()->GetTimeSeconds() + InDurationSeconds;
	SetActorScale3D(GetAreaScaleVector());
	ConfigureAuthorityCollision();
	SetLifeSpan(InDurationSeconds);
	bInitialized = true;
	BP_OnShieldAreaScaleChanged(AreaScale);
	ForceNetUpdate();
	return true;
}

void ASWShieldBarrier::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		// Deferred Spawn 之后蓝图 Construction Script 可能覆写组件默认值，因此在开始运行时再次收口权威碰撞和倍率。
		ConfigureAuthorityCollision();
		SetActorScale3D(GetAreaScaleVector());
		BP_OnShieldAreaScaleChanged(AreaScale);
		BP_OnShieldDurationInitialized(DurationSeconds, EndServerTimeSeconds);
		return;
	}

	if (!HasAuthority())
	{
		AbsorptionVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SetActorScale3D(GetAreaScaleVector());
		BP_OnShieldAreaScaleChanged(AreaScale);
	}
}

void ASWShieldBarrier::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASWShieldBarrier, AreaScale);
	DOREPLIFETIME(ASWShieldBarrier, DurationSeconds);
	DOREPLIFETIME(ASWShieldBarrier, EndServerTimeSeconds);
}

void ASWShieldBarrier::OnAbsorptionVolumeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, const int32 OtherBodyIndex, const bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || !bInitialized || !ShouldAbsorbProjectileAuthority(OtherActor))
	{
		return;
	}

	ISWProjectileAbsorptionInterface::Execute_AbsorbByShieldAuthority(OtherActor, this);
}

void ASWShieldBarrier::OnRep_AreaScale()
{
	SetActorScale3D(GetAreaScaleVector());
	BP_OnShieldAreaScaleChanged(AreaScale);
}

void ASWShieldBarrier::OnRep_EndServerTimeSeconds()
{
	BP_OnShieldDurationInitialized(DurationSeconds, EndServerTimeSeconds);
}

bool ASWShieldBarrier::ShouldAbsorbProjectileAuthority(AActor* ProjectileActor) const
{
	if (!ProjectileActor || ProjectileActor == GetInstigator()
		|| !ProjectileActor->Implements<USWProjectileAbsorptionInterface>()
		|| !ISWProjectileAbsorptionInterface::Execute_CanBeAbsorbedByShield(ProjectileActor))
	{
		return false;
	}

	const APawn* const ProjectileInstigator = ProjectileActor->GetInstigator();
	const ISWTeamInterface* const ProjectileTeamOwner = ProjectileInstigator ? Cast<ISWTeamInterface>(ProjectileInstigator) : nullptr;
	if (!ProjectileTeamOwner || ProjectileTeamOwner->GetTeamId() == ESWTeamId::None || ProjectileTeamOwner->GetTeamId() == ShieldTeamId)
	{
		return false;
	}

	// 护盾为双向屏障：只要是敌方、可吸收的投射物，即不区分其接触护盾的正反面。
	return true;
}
