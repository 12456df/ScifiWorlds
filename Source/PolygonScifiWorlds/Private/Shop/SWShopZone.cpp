// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "Shop/SWShopZone.h"

#include "Character/SWCharacter_Base.h"
#include "Components/BoxComponent.h"
#include "Player/SWPlayerState.h"

ASWShopZone::ASWShopZone()
{
	bReplicates = false;
	TradeVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TradeVolume"));
	SetRootComponent(TradeVolume);
	TradeVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TradeVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	TradeVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TradeVolume->SetGenerateOverlapEvents(true);
	TradeVolume->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::HandleTradeVolumeBeginOverlap);
	TradeVolume->OnComponentEndOverlap.AddDynamic(this, &ThisClass::HandleTradeVolumeEndOverlap);
}

void ASWShopZone::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HasAuthority())
	{
		TArray<AActor*> OverlappingActors;
		TradeVolume->GetOverlappingActors(OverlappingActors, ASWCharacter_Base::StaticClass());
		for (AActor* const Actor : OverlappingActors)
		{
			if (const ASWCharacter_Base* const Character = Cast<ASWCharacter_Base>(Actor))
			{
				if (ASWPlayerState* const PlayerState = Character->GetPlayerState<ASWPlayerState>())
				{
					PlayerState->RemoveShopZoneAuthority(this);
				}
			}
		}
	}

	Super::EndPlay(EndPlayReason);
}

void ASWShopZone::HandleTradeVolumeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, const int32 OtherBodyIndex, const bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority())
	{
		return;
	}

	if (const ASWCharacter_Base* const Character = Cast<ASWCharacter_Base>(OtherActor))
	{
		if (ASWPlayerState* const PlayerState = Character->GetPlayerState<ASWPlayerState>())
		{
			PlayerState->AddShopZoneAuthority(this);
		}
	}
}

void ASWShopZone::HandleTradeVolumeEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, const int32 OtherBodyIndex)
{
	if (!HasAuthority())
	{
		return;
	}

	if (const ASWCharacter_Base* const Character = Cast<ASWCharacter_Base>(OtherActor))
	{
		if (ASWPlayerState* const PlayerState = Character->GetPlayerState<ASWPlayerState>())
		{
			PlayerState->RemoveShopZoneAuthority(this);
		}
	}
}
