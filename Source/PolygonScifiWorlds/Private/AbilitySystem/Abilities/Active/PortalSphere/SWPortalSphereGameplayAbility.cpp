// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "AbilitySystem/Abilities/Active/PortalSphere/SWPortalSphereGameplayAbility.h"

#include "AbilitySystem/Abilities/Active/PortalSphere/SWPortalSphereProjectile.h"
#include "AbilitySystem/SWAttributeSet.h"
#include "AbilitySystem/Effects/SWPortalSphereDamageGameplayEffect.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameplayTags/SWGameplayTags.h"

USWPortalSphereGameplayAbility::USWPortalSphereGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	AbilityIdTag = SWGameplayTags::Ability_Skill_PortalSphere;
	CooldownTag = SWGameplayTags::Cooldown_Ability_PortalSphere;
}

float USWPortalSphereGameplayAbility::GetBaseMagicDamagePerTick(const int32 AbilityLevel) const
{
	return FMath::Max(0.f, BaseMagicDamagePerTick.GetValueAtLevel(FMath::Max(1, AbilityLevel)));
}

float USWPortalSphereGameplayAbility::GetSpellPowerCoefficientPerTick(const int32 AbilityLevel) const
{
	return FMath::Max(0.f, SpellPowerCoefficientPerTick.GetValueAtLevel(FMath::Max(1, AbilityLevel)));
}

float USWPortalSphereGameplayAbility::GetSpellPowerForPortalDamage() const
{
	return FMath::Max(0.f, GetOwnerAttributeValue(USWAttributeSet::GetSpellPowerAttribute()));
}

float USWPortalSphereGameplayAbility::GetBlockingSphereRadius(const int32 AbilityLevel) const
{
	return GetEffectiveArea(BlockingSphereRadius.GetValueAtLevel(FMath::Max(1, AbilityLevel)));
}

float USWPortalSphereGameplayAbility::GetDamageSphereRadius(const int32 AbilityLevel) const
{
	return GetEffectiveArea(DamageSphereRadius.GetValueAtLevel(FMath::Max(1, AbilityLevel)));
}

float USWPortalSphereGameplayAbility::GetProjectileSpeed(const int32 AbilityLevel) const
{
	return FMath::Max(0.f, ProjectileSpeed.GetValueAtLevel(FMath::Max(1, AbilityLevel)));
}

float USWPortalSphereGameplayAbility::GetEffectiveProjectileRange(const int32 AbilityLevel) const
{
	return GetEffectiveRange(FMath::Max(0.f, BaseRange.GetValueAtLevel(FMath::Max(1, AbilityLevel))));
}

float USWPortalSphereGameplayAbility::GetEffectiveProjectileDuration(const int32 AbilityLevel) const
{
	return GetEffectiveDuration(FMath::Max(0.f, BaseDuration.GetValueAtLevel(FMath::Max(1, AbilityLevel))));
}

bool USWPortalSphereGameplayAbility::SpawnPortalSphereAuthority(const FTransform& SpawnTransform, FVector LaunchDirection)
{
	const FGameplayAbilityActorInfo* const ActorInfo = GetCurrentActorInfo();
	APawn* const InstigatorPawn = ActorInfo ? Cast<APawn>(ActorInfo->AvatarActor.Get()) : nullptr;
	UWorld* const World = InstigatorPawn ? InstigatorPawn->GetWorld() : nullptr;
	if (!InstigatorPawn || !World || !InstigatorPawn->HasAuthority() || !ProjectileClass || !DamageEffectClass)
	{
		return false;
	}

	const int32 AbilityLevel = GetAbilityLevel();
	FSWDamageApplicationParams DamageParams;
	DamageParams.RawDamage = GetBaseMagicDamagePerTick(AbilityLevel)
		+ GetSpellPowerCoefficientPerTick(AbilityLevel) * GetSpellPowerForPortalDamage();
	DamageParams.DamageType = SWGameplayTags::Damage_Type_Magical;
	DamageParams.bCanCritical = bCanCritical;
	DamageParams.EffectDurationSeconds = GetEffectiveProjectileDuration(AbilityLevel);

	const float ProjectileSpeedValue = GetProjectileSpeed(AbilityLevel);
	const float MaximumRangeValue = GetEffectiveProjectileRange(AbilityLevel);
	const float BlockingRadiusValue = GetBlockingSphereRadius(AbilityLevel);
	const float DamageRadiusValue = GetDamageSphereRadius(AbilityLevel);
	const float VisualScaleValue = GetEffectiveAreaScale();
	if (DamageParams.RawDamage <= 0.f || DamageParams.EffectDurationSeconds <= 0.f || ProjectileSpeedValue <= 0.f
		|| MaximumRangeValue <= 0.f || BlockingRadiusValue <= 0.f || DamageRadiusValue < BlockingRadiusValue)
	{
		return false;
	}

	ASWPortalSphereProjectile* const PortalSphere = World->SpawnActorDeferred<ASWPortalSphereProjectile>(
		ProjectileClass,
		SpawnTransform,
		InstigatorPawn,
		InstigatorPawn,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!PortalSphere)
	{
		return false;
	}

	if (!PortalSphere->InitializePortalSphereAuthority(
		InstigatorPawn,
		LaunchDirection,
		DamageEffectClass,
		DamageParams,
		ProjectileSpeedValue,
		MaximumRangeValue,
		DamageParams.EffectDurationSeconds,
		BlockingRadiusValue,
		DamageRadiusValue,
		VisualScaleValue))
	{
		PortalSphere->Destroy();
		return false;
	}

	PortalSphere->FinishSpawning(SpawnTransform);
	return true;
}
