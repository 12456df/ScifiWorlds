// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "AbilitySystem/Abilities/Structures/SWStructureAttackGameplayAbility.h"

#include "AbilitySystem/Effects/SWDamageGameplayEffect.h"
#include "AbilitySystem/SWAbilitySystemComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameplayTags/SWGameplayTags.h"
#include "Structures/SWDefenseStructure.h"
#include "Structures/SWStructureAttackProjectile.h"
#include "Structures/SWStructureTargetingComponent.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SWStructureAttackGameplayAbility)

USWStructureAttackGameplayAbility::USWStructureAttackGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	FAbilityTriggerData& Trigger = AbilityTriggers.AddDefaulted_GetRef();
	Trigger.TriggerTag = SWGameplayTags::Event_Combat_StructureAttack;
	Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;

	RawDamageByLevel.Value = 1.f;
	DamageType = SWGameplayTags::Damage_Type_Physical;
	bRequiresMatchInProgress = true;
}

UGameplayEffect* USWStructureAttackGameplayAbility::GetCooldownGameplayEffect() const
{
	return CooldownEffectClass ? CooldownEffectClass->GetDefaultObject<UGameplayEffect>() : nullptr;
}

void USWStructureAttackGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	ASWDefenseStructure* const Structure = ActorInfo ? Cast<ASWDefenseStructure>(ActorInfo->AvatarActor.Get()) : nullptr;
	// FGameplayEventData 将目标声明为 const；本 Ability 只读取目标并传入既有权威查询，因此不修改目标本身。
	AActor* const TargetActor = TriggerEventData
		? const_cast<AActor*>(TriggerEventData->Target.Get())
		: nullptr;
	if (!Structure || !Structure->HasAuthority() || !TargetActor || !DamageEffectClass || !ProjectileClass
		|| !Structure->GetTargetingComponent()
		|| Structure->GetTargetingComponent()->GetCurrentTargetAuthority() != TargetActor
		|| !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	PendingTarget = TargetActor;
	PendingHandle = Handle;
	PendingActivationInfo = ActivationInfo;

	const float SafeWindupSeconds = FMath::Max(0.f, AttackWindupSeconds);
	if (SafeWindupSeconds <= KINDA_SMALL_NUMBER)
	{
		ExecutePendingAttackAuthority();
		return;
	}

	Structure->GetWorldTimerManager().SetTimer(
		AttackWindupTimer,
		this,
		&ThisClass::ExecutePendingAttackAuthority,
		SafeWindupSeconds,
		false);
}

void USWStructureAttackGameplayAbility::CancelAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const bool bReplicateCancelAbility)
{
	ClearPendingAttack();
	Super::CancelAbility(Handle, ActorInfo, ActivationInfo, bReplicateCancelAbility);
}

void USWStructureAttackGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const bool bReplicateEndAbility, const bool bWasCancelled)
{
	ClearPendingAttack();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void USWStructureAttackGameplayAbility::ExecutePendingAttackAuthority()
{
	ASWDefenseStructure* const Structure = Cast<ASWDefenseStructure>(GetAvatarActorFromActorInfo());
	AActor* const TargetActor = PendingTarget.Get();
	const FGameplayAbilitySpecHandle Handle = PendingHandle;
	const FGameplayAbilityActivationInfo ActivationInfo = PendingActivationInfo;

	const bool bSpawnedProjectile = Structure && Structure->HasAuthority() && TargetActor
		&& Structure->GetTargetingComponent()
		&& Structure->GetTargetingComponent()->GetCurrentTargetAuthority() == TargetActor
		&& SpawnProjectileAuthority(TargetActor);

	EndAbility(Handle, GetCurrentActorInfo(), ActivationInfo, true, !bSpawnedProjectile);
}

bool USWStructureAttackGameplayAbility::SpawnProjectileAuthority(AActor* const TargetActor)
{
	ASWDefenseStructure* const Structure = Cast<ASWDefenseStructure>(GetAvatarActorFromActorInfo());
	USWAbilitySystemComponent* const SourceASC = Structure
		? Cast<USWAbilitySystemComponent>(Structure->GetAbilitySystemComponent())
		: nullptr;
	UAbilitySystemComponent* const TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	UWorld* const World = Structure ? Structure->GetWorld() : nullptr;
	if (!Structure || !SourceASC || !TargetASC || !World || !ProjectileClass || !DamageEffectClass)
	{
		return false;
	}

	const int32 AbilityLevel = FMath::Max(1, GetAbilityLevel());
	const float RawDamage = FMath::Max(0.f, RawDamageByLevel.GetValueAtLevel(AbilityLevel));
	if (RawDamage <= 0.f)
	{
		return false;
	}

	const FVector SpawnLocation = Structure->GetAttackOrigin()->GetComponentLocation();
	const FVector LaunchDirection = (TargetActor->GetActorLocation() - SpawnLocation).GetSafeNormal();
	if (LaunchDirection.IsNearlyZero())
	{
		return false;
	}

	FSWDamageApplicationParams DamageParams;
	DamageParams.RawDamage = RawDamage;
	DamageParams.DamageType = DamageType.IsValid() ? DamageType : SWGameplayTags::Damage_Type_Physical;
	DamageParams.bCanCritical = bCanCritical;

	const FTransform SpawnTransform(LaunchDirection.Rotation(), SpawnLocation);
	ASWStructureAttackProjectile* const Projectile = World->SpawnActorDeferred<ASWStructureAttackProjectile>(
		ProjectileClass,
		SpawnTransform,
		Structure,
		Structure,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Projectile)
	{
		return false;
	}

	// 先完成 Deferred Spawn，确保蓝图构造脚本和组件注册结束后再激活 ProjectileMovement。
	// 该顺序与普通武器弹丸一致，避免 FinishSpawning 覆盖已设置的初速度或激活状态。
	Projectile->FinishSpawning(SpawnTransform);

	if (!Projectile->InitializeStructureAttackProjectileAuthority(
		Structure,
		TargetActor,
		LaunchDirection,
		DamageEffectClass,
		DamageParams))
	{
		Projectile->Destroy();
		return false;
	}

	if (AttackCueTag.IsValid())
	{
		FGameplayCueParameters CueParameters;
		CueParameters.Location = SpawnLocation;
		CueParameters.EffectCauser = Structure;
		SourceASC->ExecuteGameplayCue(AttackCueTag, CueParameters);
	}

	return true;
}

void USWStructureAttackGameplayAbility::ClearPendingAttack()
{
	if (ASWDefenseStructure* const Structure = Cast<ASWDefenseStructure>(GetAvatarActorFromActorInfo()))
	{
		Structure->GetWorldTimerManager().ClearTimer(AttackWindupTimer);
	}

	PendingTarget.Reset();
	PendingHandle = FGameplayAbilitySpecHandle();
	PendingActivationInfo = FGameplayAbilityActivationInfo();
}
