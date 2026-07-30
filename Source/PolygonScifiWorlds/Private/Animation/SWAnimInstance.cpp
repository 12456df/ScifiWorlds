// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "Animation/SWAnimInstance.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayTags/SWGameplayTags.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Movement/SWCharacterMovementComponent.h"

namespace
{
	constexpr float MovementThreshold = 3.f;
}

USWAnimInstance::USWAnimInstance()
{
	RootMotionMode = ERootMotionMode::RootMotionFromMontagesOnly;
}

void USWAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	CacheCharacterReferences();
}

void USWAnimInstance::NativeUpdateAnimation(const float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!OwningCharacter || !MovementComponent)
	{
		CacheCharacterReferences();
	}
	if (!OwningCharacter || !MovementComponent)
	{
		return;
	}

	const FVector Velocity = MovementComponent->Velocity;
	GroundSpeed = Velocity.Size2D();
	Acceleration = MovementComponent->GetCurrentAcceleration().Size2D();
	bIsInAir = MovementComponent->IsFalling();
	bIsCrouching = MovementComponent->IsCrouching();
	bShouldMove = GroundSpeed > MovementThreshold && Acceleration > MovementThreshold;

	if (GroundSpeed > MovementThreshold)
	{
		const FRotator DirectionDelta = (Velocity.ToOrientationRotator() - OwningCharacter->GetActorRotation()).GetNormalized();
		Direction = DirectionDelta.Yaw;
	}

	if (const USWCharacterMovementComponent* SWMovementComponent = Cast<USWCharacterMovementComponent>(MovementComponent))
	{
		bIsSprinting = SWMovementComponent->IsSprinting();
	}
	else
	{
		bIsSprinting = false;
	}

	bIsAiming = false;
	bIsReloading = false;
	if (const IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(OwningCharacter))
	{
		if (const UAbilitySystemComponent* AbilitySystemComponent = AbilitySystemInterface->GetAbilitySystemComponent())
		{
			bIsAiming = AbilitySystemComponent->HasMatchingGameplayTag(SWGameplayTags::State_Weapon_Aiming);
			bIsReloading = AbilitySystemComponent->HasMatchingGameplayTag(SWGameplayTags::State_Weapon_Reloading);
		}
	}

	const FRotator AimDelta = (OwningCharacter->GetBaseAimRotation() - OwningCharacter->GetActorRotation()).GetNormalized();
	AimYaw = FMath::Clamp(AimDelta.Yaw, -90.f, 90.f);
	AimPitch = FMath::Clamp(AimDelta.Pitch, -90.f, 90.f);

	if (bIsInAir)
	{
		LocomotionState = ESWLocomotionState::InAir;
	}
	else if (bIsCrouching)
	{
		LocomotionState = ESWLocomotionState::Crouching;
	}
	else if (bIsSprinting)
	{
		LocomotionState = ESWLocomotionState::Sprinting;
	}
	else if (GroundSpeed > MovementThreshold)
	{
		LocomotionState = ESWLocomotionState::Moving;
	}
	else
	{
		LocomotionState = ESWLocomotionState::Idle;
	}
}

void USWAnimInstance::CacheCharacterReferences()
{
	OwningCharacter = Cast<ACharacter>(TryGetPawnOwner());
	MovementComponent = OwningCharacter ? OwningCharacter->GetCharacterMovement() : nullptr;
}
