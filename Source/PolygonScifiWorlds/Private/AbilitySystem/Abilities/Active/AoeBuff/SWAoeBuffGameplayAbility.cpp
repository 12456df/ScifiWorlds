// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "AbilitySystem/Abilities/Active/AoeBuff/SWAoeBuffGameplayAbility.h"
#include "AbilitySystem/SWAbilityTypes.h"
#include "AbilitySystem/SWAttributeSet.h"
#include "AbilitySystem/Effects/SWCommonBuffGameplayEffects.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayTags/SWGameplayTags.h"
#include "Interaction/SWCombatInterface.h"
#include "Interaction/SWTeamInterface.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

USWAoeBuffGameplayAbility::USWAoeBuffGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	AbilityIdTag = SWGameplayTags::Ability_Skill_AoeBuff;
	CooldownTag = SWGameplayTags::Cooldown_Ability_AoeBuff;
	StunMovementSpeedDelta = FScalableFloat(-0.5f);
}

int32 USWAoeBuffGameplayAbility::ApplyCasterCenteredHemisphereBuffsAuthority()
{
	const FGameplayAbilityActorInfo* Info = GetCurrentActorInfo();
	APawn* const Caster = Info ? Cast<APawn>(Info->AvatarActor.Get()) : nullptr;
	UAbilitySystemComponent* const SourceASC = Info ? Info->AbilitySystemComponent.Get() : nullptr;
	const ISWTeamInterface* const SourceTeam = Cast<ISWTeamInterface>(Caster);
	const int32 Level = GetAbilityLevel();
	const float Radius = GetEffectiveArea(FMath::Max(0.f, BaseHemisphereRadius.GetValueAtLevel(Level)));
	const float Duration = GetEffectiveDuration(FMath::Max(0.f, BaseBuffDuration.GetValueAtLevel(Level)));
	if (!Caster || !Caster->HasAuthority() || !SourceASC || !SourceTeam || Radius <= 0.f || Duration <= 0.f
		|| !SpeedBuffEffectClass || !HealBuffEffectClass || !StunDebuffEffectClass || !PoisonDebuffEffectClass)
	{
		return 0;
	}

	FGameplayCueParameters CueParameters;
	CueParameters.Location = Caster->GetActorLocation();
	CueParameters.EffectCauser = Caster;
	CueParameters.RawMagnitude = Radius;
	SourceASC->ExecuteGameplayCue(SWGameplayTags::GameplayCue_Ability_AoeBuff_Cast, CueParameters);

	TArray<FOverlapResult> Hits;
	FCollisionObjectQueryParams ObjectTypes(ECC_Pawn);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SWAoeBuff), false, Caster);
	Caster->GetWorld()->OverlapMultiByObjectType(Hits, Caster->GetActorLocation(), FQuat::Identity, ObjectTypes, FCollisionShape::MakeSphere(Radius), QueryParams);
	TArray<AActor*> Candidates;
	Candidates.Reserve(Hits.Num() + 1);
	Candidates.Add(Caster);
	for (const FOverlapResult& Hit : Hits)
	{
		if (AActor* const HitActor = Hit.GetActor())
		{
			Candidates.Add(HitActor);
		}
	}
	const float HealPerSecond = FMath::Max(0.f, BaseHealPerSecond.GetValueAtLevel(Level) + CasterCurrentHealthHealCoefficientPerSecond.GetValueAtLevel(Level) * SourceASC->GetNumericAttribute(USWAttributeSet::GetHealthAttribute()));
	const float PoisonDamage = FMath::Max(0.f, BasePoisonDamagePerTick.GetValueAtLevel(Level) + SpellPowerCoefficientPerTick.GetValueAtLevel(Level) * SourceASC->GetNumericAttribute(USWAttributeSet::GetSpellPowerAttribute()));
	const float StunSlowDelta = FMath::Clamp(StunMovementSpeedDelta.GetValueAtLevel(Level), -0.99f, 0.f);
	int32 AppliedTargets = 0;
	TSet<TObjectPtr<AActor>> Processed;
	for (AActor* const Target : Candidates)
	{
		IAbilitySystemInterface* const TargetASI = Cast<IAbilitySystemInterface>(Target);
		const ISWTeamInterface* const TargetTeam = Cast<ISWTeamInterface>(Target);
		UAbilitySystemComponent* const TargetASC = TargetASI ? TargetASI->GetAbilitySystemComponent() : nullptr;
		if (!Target || Processed.Contains(Target) || !TargetASC || !TargetTeam
			|| (Target->Implements<USWCombatInterface>() && ISWCombatInterface::Execute_IsDead(Target))) continue;
		Processed.Add(Target);
		const bool bFriendly = TargetTeam->GetTeamId() == SourceTeam->GetTeamId();
		FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext(); Context.AddSourceObject(Caster);
		auto Apply = [&](TSubclassOf<UGameplayEffect> Class, float MagnitudeTagValue, FGameplayTag MagnitudeTag)
		{ FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(Class, Level, Context); if (Spec.IsValid()) { Spec.Data->SetSetByCallerMagnitude(SWGameplayTags::SetByCaller_Ability_Duration, Duration); Spec.Data->SetSetByCallerMagnitude(MagnitudeTag, MagnitudeTagValue); TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get()); } };
		if (bFriendly) { Apply(SpeedBuffEffectClass, FMath::Max(0.f, SpeedMultiplierDelta.GetValueAtLevel(Level)), SWGameplayTags::SetByCaller_Buff_MovementSpeedDelta); Apply(HealBuffEffectClass, HealPerSecond, SWGameplayTags::SetByCaller_Buff_HealthPerSecond); }
		else { Apply(StunDebuffEffectClass, StunSlowDelta, SWGameplayTags::SetByCaller_Buff_MovementSpeedDelta); FSWGameplayEffectContext* Raw = static_cast<FSWGameplayEffectContext*>(Context.Get()); if (Raw) Raw->SetDamageType(SWGameplayTags::Damage_Type_Magical); Apply(PoisonDebuffEffectClass, PoisonDamage, SWGameplayTags::SetByCaller_Damage_Raw); }
		++AppliedTargets;
	}
	return AppliedTargets;
}
