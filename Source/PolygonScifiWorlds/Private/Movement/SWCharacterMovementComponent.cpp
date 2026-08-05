// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "Movement/SWCharacterMovementComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/SWAttributeSet.h"
#include "GameplayTags/SWGameplayTags.h"
#include "GameFramework/Character.h"

namespace
{
	class FSWCharacterSavedMove final : public FSavedMove_Character
	{
	public:
		typedef FSavedMove_Character Super;

		virtual void Clear() override
		{
			Super::Clear();
			bSavedWantsToSprint = false;
		}

		virtual uint8 GetCompressedFlags() const override
		{
			uint8 Result = Super::GetCompressedFlags();
			if (bSavedWantsToSprint)
			{
				Result |= FLAG_Custom_0;
			}
			return Result;
		}

		virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* Character, float MaxDelta) const override
		{
			const FSWCharacterSavedMove* OtherMove = static_cast<const FSWCharacterSavedMove*>(NewMove.Get());
			return bSavedWantsToSprint == OtherMove->bSavedWantsToSprint && Super::CanCombineWith(NewMove, Character, MaxDelta);
		}

		virtual void SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAcceleration, FNetworkPredictionData_Client_Character& ClientData) override
		{
			Super::SetMoveFor(Character, InDeltaTime, NewAcceleration, ClientData);

			const USWCharacterMovementComponent* MovementComponent = Character ? Cast<USWCharacterMovementComponent>(Character->GetCharacterMovement()) : nullptr;
			bSavedWantsToSprint = MovementComponent && MovementComponent->IsSprintRequested();
		}

		virtual void PrepMoveFor(ACharacter* Character) override
		{
			Super::PrepMoveFor(Character);

			if (USWCharacterMovementComponent* MovementComponent = Character ? Cast<USWCharacterMovementComponent>(Character->GetCharacterMovement()) : nullptr)
			{
				MovementComponent->SetSprintRequested(bSavedWantsToSprint);
			}
		}

	private:
		uint8 bSavedWantsToSprint : 1;
	};

	class FSWNetworkPredictionData_Client_Character final : public FNetworkPredictionData_Client_Character
	{
	public:
		using Super = FNetworkPredictionData_Client_Character;

		explicit FSWNetworkPredictionData_Client_Character(const UCharacterMovementComponent& ClientMovement)
			: Super(ClientMovement)
		{
		}

		virtual FSavedMovePtr AllocateNewMove() override
		{
			return FSavedMovePtr(new FSWCharacterSavedMove());
		}
	};
}

USWCharacterMovementComponent::USWCharacterMovementComponent()
	: bWantsToSprint(false)
	, bWasSprinting(false)
{
}

void USWCharacterMovementComponent::SetSprintRequested(const bool bRequested)
{
	bWantsToSprint = bRequested;
	UpdateSprintingState();
}

bool USWCharacterMovementComponent::IsSprinting() const
{
	return CanSprint();
}

bool USWCharacterMovementComponent::CanSprint() const
{
	if (!bWantsToSprint || !IsMovingOnGround() || IsCrouching() || Acceleration.IsNearlyZero())
	{
		return false;
	}

	const ACharacter* Character = CharacterOwner;
	if (!Character)
	{
		return false;
	}

	if (const IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(Character))
	{
		if (const UAbilitySystemComponent* AbilitySystemComponent = AbilitySystemInterface->GetAbilitySystemComponent())
		{
			if (AbilitySystemComponent->HasMatchingGameplayTag(SWGameplayTags::State_Weapon_Aiming)
				|| AbilitySystemComponent->HasMatchingGameplayTag(SWGameplayTags::State_Weapon_Reloading))
			{
				return false;
			}
		}
	}

	const float ForwardInput = FVector::DotProduct(Acceleration.GetSafeNormal2D(), Character->GetActorForwardVector().GetSafeNormal2D());
	return ForwardInput >= SprintForwardThreshold;
}

float USWCharacterMovementComponent::GetMaxSpeed() const
{
	if (!IsMovingOnGround())
	{
		return Super::GetMaxSpeed();
	}

	float BaseSpeed = WalkSpeed;
	if (IsCrouching())
	{
		BaseSpeed = CrouchSpeed;
	}
	else if (CanSprint())
	{
		BaseSpeed = SprintSpeed;
	}

	float MovementSpeedMultiplier = 1.f;
	if (const IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(CharacterOwner))
	{
		if (const UAbilitySystemComponent* AbilitySystemComponent = AbilitySystemInterface->GetAbilitySystemComponent())
		{
			MovementSpeedMultiplier = AbilitySystemComponent->GetNumericAttribute(USWAttributeSet::GetMovementSpeedMultiplierAttribute());
		}
	}

	// 具体上下限由后续数据资产配置；此处仅防止非法属性让 CMC 返回负速度。
	return BaseSpeed * FMath::Max(0.01f, MovementSpeedMultiplier);
}

void USWCharacterMovementComponent::UpdateFromCompressedFlags(const uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);
	bWantsToSprint = (Flags & FSavedMove_Character::FLAG_Custom_0) != 0;
}

FNetworkPredictionData_Client* USWCharacterMovementComponent::GetPredictionData_Client() const
{
	if (!ClientPredictionData)
	{
		USWCharacterMovementComponent* MutableThis = const_cast<USWCharacterMovementComponent*>(this);
		MutableThis->ClientPredictionData = new FSWNetworkPredictionData_Client_Character(*this);
	}

	return ClientPredictionData;
}

void USWCharacterMovementComponent::OnMovementUpdated(const float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity)
{
	Super::OnMovementUpdated(DeltaSeconds, OldLocation, OldVelocity);
	UpdateSprintingState();
}

void USWCharacterMovementComponent::UpdateSprintingState()
{
	const bool bIsSprintingNow = IsSprinting();
	if (bWasSprinting == bIsSprintingNow)
	{
		return;
	}

	bWasSprinting = bIsSprintingNow;
	OnSprintingChanged.Broadcast(bIsSprintingNow);
}
