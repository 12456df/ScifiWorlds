// Copyright (c) 2024 Synty Studios Limited. All rights reserved.


#include "Character/SWCharacter_Base.h"

// Sets default values
ASWCharacter_Base::ASWCharacter_Base()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ASWCharacter_Base::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ASWCharacter_Base::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void ASWCharacter_Base::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

