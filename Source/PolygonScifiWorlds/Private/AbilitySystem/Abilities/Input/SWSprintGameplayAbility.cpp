// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "AbilitySystem/Abilities/Input/SWSprintGameplayAbility.h"

#include "AbilitySystem/SWAttributeSet.h"
#include "Character/SWCharacter_Player.h"
#include "GameplayEffect.h"
#include "GameplayTags/SWGameplayTags.h"
#include "Movement/SWCharacterMovementComponent.h"

USWSprintGameplayAbility::USWSprintGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(SWGameplayTags::Ability_Movement_Sprint);
	SetAssetTags(AssetTags);
	ActivationOwnedTags.AddTag(SWGameplayTags::State_Movement_Sprinting);
	ActivationBlockedTags.AddTag(SWGameplayTags::State_Weapon_Aiming);
	ActivationBlockedTags.AddTag(SWGameplayTags::State_Weapon_Reloading);
	ActivationBlockedTags.AddTag(SWGameplayTags::State_Weapon_Firing);
}

void USWSprintGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!ApplyStaminaDrainAuthority(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	SetSprintRequested(true);
}

bool USWSprintGameplayAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const float CurrentStamina = GetOwnerAttributeValue(USWAttributeSet::GetStaminaAttribute());
	if (!FMath::IsFinite(CurrentStamina) || CurrentStamina <= 0.f)
	{
		if (OptionalRelevantTags)
		{
			OptionalRelevantTags->AddTag(SWGameplayTags::Ability_Fail_NoStamina);
		}
		return false;
	}

	return true;
}

void USWSprintGameplayAbility::InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputReleased(Handle, ActorInfo, ActivationInfo);
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void USWSprintGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const bool bReplicateEndAbility, const bool bWasCancelled)
{
	SetSprintRequested(false);
	RemoveStaminaDrainAuthority(ActorInfo);
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool USWSprintGameplayAbility::ApplyStaminaDrainAuthority(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	if (!IsAvatarAuthority())
	{
		return true;
	}

	if (!ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid() || !StaminaDrainEffect)
	{
		return false;
	}

	const FGameplayEffectSpecHandle DrainSpec = MakeOutgoingGameplayEffectSpec(
		Handle, ActorInfo, ActivationInfo, StaminaDrainEffect, GetAbilityLevel(Handle, ActorInfo));
	if (!DrainSpec.IsValid())
	{
		return false;
	}

	StaminaDrainEffectHandle = ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, DrainSpec);
	return StaminaDrainEffectHandle.IsValid();
}

void USWSprintGameplayAbility::RemoveStaminaDrainAuthority(const FGameplayAbilityActorInfo* ActorInfo)
{
	if (!IsAvatarAuthority() || !StaminaDrainEffectHandle.IsValid())
	{
		return;
	}

	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		ActorInfo->AbilitySystemComponent->RemoveActiveGameplayEffect(StaminaDrainEffectHandle);
	}

	StaminaDrainEffectHandle.Invalidate();
}

void USWSprintGameplayAbility::SetSprintRequested(const bool bRequested) const
{
	if (ASWCharacter_Player* const Character = GetPlayerCharacter())
	{
		if (USWCharacterMovementComponent* const MovementComponent = Character->GetCharacterMovement<USWCharacterMovementComponent>())
		{
			MovementComponent->SetSprintRequested(bRequested);
		}
	}
}
