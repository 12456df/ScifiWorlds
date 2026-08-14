// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "AbilitySystem/Abilities/Active/Shield/SWShieldGameplayAbility.h"

#include "AbilitySystem/Abilities/Active/Shield/SWShieldBarrier.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameplayTags/SWGameplayTags.h"
#include "Interaction/SWTeamInterface.h"

USWShieldGameplayAbility::USWShieldGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	AbilityIdTag = SWGameplayTags::Ability_Skill_Shield;
	CooldownTag = SWGameplayTags::Cooldown_Ability_Shield;
	// Shield 是充能技能；允许装备通过 AbilityChargeBonus 增加其最大充能。
	bUseAbilityChargeBonus = true;
}

float USWShieldGameplayAbility::GetForwardSpawnDistance(const int32 AbilityLevel) const
{
	const float Distance = ForwardSpawnDistance.GetValueAtLevel(FMath::Max(1, AbilityLevel));
	return FMath::IsFinite(Distance) ? FMath::Max(0.f, Distance) : 0.f;
}

float USWShieldGameplayAbility::GetEffectiveShieldDuration(const int32 AbilityLevel) const
{
	return GetEffectiveDuration(BaseDuration.GetValueAtLevel(FMath::Max(1, AbilityLevel)));
}

FVector USWShieldGameplayAbility::GetEffectiveShieldPreviewBoxExtent() const
{
	const ASWShieldBarrier* const ShieldDefaults = ShieldClass ? ShieldClass->GetDefaultObject<ASWShieldBarrier>() : nullptr;
	if (!ShieldDefaults)
	{
		return FVector::ZeroVector;
	}

	const FVector DefaultExtent = ShieldDefaults->GetDefaultAbsorptionBoxExtent();
	const float AreaScale = GetEffectiveAreaScale();
	return FVector(DefaultExtent.X, DefaultExtent.Y * AreaScale, DefaultExtent.Z * AreaScale);
}

bool USWShieldGameplayAbility::SpawnShieldAuthority()
{
	const FGameplayAbilityActorInfo* const ActorInfo = GetCurrentActorInfo();
	APawn* const AvatarPawn = ActorInfo ? Cast<APawn>(ActorInfo->AvatarActor.Get()) : nullptr;
	UWorld* const World = AvatarPawn ? AvatarPawn->GetWorld() : nullptr;
	const ISWTeamInterface* const TeamOwner = AvatarPawn ? Cast<ISWTeamInterface>(AvatarPawn) : nullptr;
	if (!AvatarPawn || !World || !AvatarPawn->HasAuthority() || !ShieldClass || !TeamOwner)
	{
		return false;
	}

	const ESWTeamId TeamId = TeamOwner->GetTeamId();
	if (TeamId != ESWTeamId::TeamA && TeamId != ESWTeamId::TeamB)
	{
		return false;
	}

	const int32 AbilityLevel = GetAbilityLevel();
	const float Duration = GetEffectiveShieldDuration(AbilityLevel);
	const float AreaScale = GetEffectiveAreaScale();
	if (Duration <= 0.f || AreaScale <= 0.f)
	{
		return false;
	}

	const FVector Forward = AvatarPawn->GetActorForwardVector().GetSafeNormal();
	if (Forward.IsNearlyZero())
	{
		return false;
	}

	const FTransform SpawnTransform(Forward.Rotation(), AvatarPawn->GetActorLocation() + Forward * GetForwardSpawnDistance(AbilityLevel));
	ASWShieldBarrier* const Shield = World->SpawnActorDeferred<ASWShieldBarrier>(
		ShieldClass, SpawnTransform, AvatarPawn, AvatarPawn, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Shield)
	{
		return false;
	}

	if (!Shield->InitializeShieldAuthority(AvatarPawn, TeamId, Duration, AreaScale))
	{
		Shield->Destroy();
		return false;
	}

	Shield->FinishSpawning(SpawnTransform);
	return true;
}
