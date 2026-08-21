// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "AbilitySystem/Abilities/Minions/SWMinionAttackGameplayAbility.h"

#include "AbilitySystem/Effects/SWDamageGameplayEffect.h"
#include "AbilitySystem/SWAbilitySystemComponent.h"
#include "AbilitySystem/SWAttributeSet.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Character/SWCharacter_Minion.h"
#include "GameplayTags/SWGameplayTags.h"
#include "MassSignalSubsystem.h"
#include "MassEntitySubsystem.h"
#include "MassEntityView.h"
#include "Mass/Minions/SWMinionMassTypes.h"
#include "MassStateTreeTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SWMinionAttackGameplayAbility)

USWMinionAttackGameplayAbility::USWMinionAttackGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	FAbilityTriggerData& Trigger = AbilityTriggers.AddDefaulted_GetRef();
	Trigger.TriggerTag = SWGameplayTags::Event_Combat_MinionAttack;
	Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
}

void USWMinionAttackGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const bool bReplicateEndAbility, const bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

	// 由 Ability 的蓝图时序决定何时结束。StateTree 仍是行为状态唯一来源；
	// GA 只在一次攻击生命周期结束后重新发布下一次攻击请求，Attack Processor
	// 会再次进行权威目标校验，绝不在这里直接伤害或改写 Behavior。
	ASWCharacter_Minion* const SourceMinion = ActorInfo ? Cast<ASWCharacter_Minion>(ActorInfo->AvatarActor.Get()) : nullptr;
	UWorld* const World = SourceMinion ? SourceMinion->GetWorld() : nullptr;
	if (!SourceMinion || !SourceMinion->HasAuthority() || !World || !SourceMinion->GetMassEntityHandleAuthority().IsValid())
	{
		return;
	}

	if (UMassEntitySubsystem* const EntitySubsystem = World->GetSubsystem<UMassEntitySubsystem>())
	{
		FMassEntityManager& EntityManager = EntitySubsystem->GetMutableEntityManager();
		const FMassEntityHandle Entity = SourceMinion->GetMassEntityHandleAuthority();
		if (EntityManager.IsEntityValid(Entity))
		{
			FMassEntityView EntityView(EntityManager, Entity);
			if (FSWMinionIntentFragment* const Intent = EntityView.GetFragmentDataPtr<FSWMinionIntentFragment>())
			{
				Intent->bAttackRequested = Intent->Behavior == ESWMinionBehaviorIntent::Attacking;
			}
		}
	}

	if (UMassSignalSubsystem* const SignalSubsystem = World->GetSubsystem<UMassSignalSubsystem>())
	{
		SignalSubsystem->SignalEntity(UE::Mass::Signals::NewStateTreeTaskRequired, SourceMinion->GetMassEntityHandleAuthority());
	}
}

bool USWMinionAttackGameplayAbility::ApplyMinionAttackDamageAuthority(AActor* TargetActor)
{
	ASWCharacter_Minion* const SourceMinion = Cast<ASWCharacter_Minion>(GetAvatarActorFromActorInfo());
	USWAbilitySystemComponent* const SourceASC = SourceMinion ? Cast<USWAbilitySystemComponent>(SourceMinion->GetAbilitySystemComponent()) : nullptr;
	UAbilitySystemComponent* const TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	if (!SourceMinion || !SourceASC || !TargetASC || !DamageEffectClass
		|| SourceMinion->ValidateMinionAttackTargetAuthority(TargetActor) != ESWMinionAttackAttemptResult::Accepted)
	{
		return false;
	}

	const int32 EffectLevel = FMath::Max(1, SourceMinion->GetMinionCombatLevel());
	const float RawDamage = FMath::Max(0.f, BaseDamage.GetValueAtLevel(EffectLevel)
		+ SourceASC->GetNumericAttribute(USWAttributeSet::GetAttackPowerAttribute()) * AttackPowerCoefficient);
	if (RawDamage <= 0.f)
	{
		return false;
	}

	FSWDamageApplicationParams DamageParams;
	DamageParams.RawDamage = RawDamage;
	DamageParams.DamageType = DamageType.IsValid() ? DamageType : SWGameplayTags::Damage_Type_Physical;
	DamageParams.bCanCritical = bCanCritical;
	return SourceASC->ApplyDamageEffectToTargetAuthority(TargetASC, DamageEffectClass, EffectLevel, SourceMinion, DamageParams);
}
