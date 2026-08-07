// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "AbilitySystem/SWAttributeSet.h"

#include "AbilitySystem/SWAbilityTypes.h"
#include "AbilitySystem/SWAbilitySystemComponent.h"
#include "GameFramework/Pawn.h"
#include "GameplayEffectExtension.h"
#include "Interaction/SWCombatInterface.h"
#include "Interaction/SWPlayerProgressionInterface.h"
#include "Net/UnrealNetwork.h"
#include "Player/SWPlayerController.h"
#include "UI/DamageNumber/SWDamageNumberTypes.h"
#include "GameplayTags//SWGameplayTags.h"

USWAttributeSet::USWAttributeSet()
{
	// 乘数属性采用 1 表示无修正；装备与临时 GE 在此基础上聚合。
	InitMovementSpeedMultiplier(1.f);
	InitMagazineCapacityMultiplier(1.f);
	InitCriticalDamage(1.f);
}

void USWAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// REPNOTIFY_Always keeps client prediction and observers in sync even when a
	// value is set back to its previous replicated value.
	DOREPLIFETIME_CONDITION_NOTIFY(USWAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USWAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USWAttributeSet, Mana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USWAttributeSet, MaxMana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USWAttributeSet, Stamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USWAttributeSet, MaxStamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USWAttributeSet, MovementSpeedMultiplier, COND_None, REPNOTIFY_Always);

	DOREPLIFETIME_CONDITION_NOTIFY(USWAttributeSet, AttackPower, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USWAttributeSet, SpellPower, COND_None, REPNOTIFY_Always);

	DOREPLIFETIME_CONDITION_NOTIFY(USWAttributeSet, PhysicalArmor, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USWAttributeSet, MagicalArmor, COND_None, REPNOTIFY_Always);

	DOREPLIFETIME_CONDITION_NOTIFY(USWAttributeSet, PhysicalPenetrationPercent, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USWAttributeSet, MagicalPenetrationPercent, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USWAttributeSet, PhysicalPenetrationFlat, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USWAttributeSet, MagicalPenetrationFlat, COND_None, REPNOTIFY_Always);

	DOREPLIFETIME_CONDITION_NOTIFY(USWAttributeSet, CriticalChance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USWAttributeSet, CriticalDamage, COND_None, REPNOTIFY_Always);

	DOREPLIFETIME_CONDITION_NOTIFY(USWAttributeSet, Tenacity, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USWAttributeSet, PhysicalLifesteal, COND_None, REPNOTIFY_Always);

	DOREPLIFETIME_CONDITION_NOTIFY(USWAttributeSet, ManaRegeneration, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USWAttributeSet, HealthRegeneration, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USWAttributeSet, StaminaRegeneration, COND_None, REPNOTIFY_Always);

	DOREPLIFETIME_CONDITION_NOTIFY(USWAttributeSet, AbilityRangeBonusPercent, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USWAttributeSet, AbilityDurationBonusPercent, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USWAttributeSet, CooldownReductionPercent, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USWAttributeSet, MagazineCapacityMultiplier, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USWAttributeSet, FireIntervalReductionPercent, COND_None, REPNOTIFY_Always);
}

void USWAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	// 当前资源不能超出最大值；死亡等游戏规则由后续模块处理。
	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}
	else if (Attribute == GetManaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxMana());
	}
	else if (Attribute == GetStaminaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxStamina());
	}
	else if (Attribute == GetMaxHealthAttribute())
	{
		NewValue = FMath::Max(0.f, NewValue);
		// 最大生命降低时只截断当前生命，不在最大值恢复后返还被截断的生命。
		SetHealth(FMath::Clamp(GetHealth(), 0.f, NewValue));
	}
	else if (Attribute == GetMaxManaAttribute())
	{
		NewValue = FMath::Max(0.f, NewValue);
		// 最大蓝量降低时只截断当前蓝量，不在最大值恢复后返还被截断的蓝量。
		SetMana(FMath::Clamp(GetMana(), 0.f, NewValue));
	}
	else if (Attribute == GetMaxStaminaAttribute())
	{
		NewValue = FMath::Max(0.f, NewValue);
		SetStamina(FMath::Clamp(GetStamina(), 0.f, NewValue));
	}
	else if (Attribute == GetPhysicalPenetrationPercentAttribute() || Attribute == GetMagicalPenetrationPercentAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, 1.f);
	}
}

void USWAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	// Re-clamp after an executed (instant/periodic) effect modifies the base value.
	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));
	}
	else if (Data.EvaluatedData.Attribute == GetManaAttribute())
	{
		SetMana(FMath::Clamp(GetMana(), 0.f, GetMaxMana()));
	}
	else if (Data.EvaluatedData.Attribute == GetStaminaAttribute())
	{
		SetStamina(FMath::Clamp(GetStamina(), 0.f, GetMaxStamina()));
	}
	else if (Data.EvaluatedData.Attribute == GetPhysicalPenetrationPercentAttribute())
	{
		SetPhysicalPenetrationPercent(FMath::Clamp(GetPhysicalPenetrationPercent(), 0.f, 1.f));
	}
	else if (Data.EvaluatedData.Attribute == GetMagicalPenetrationPercentAttribute())
	{
		SetMagicalPenetrationPercent(FMath::Clamp(GetMagicalPenetrationPercent(), 0.f, 1.f));
	}
	else if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		ConsumeIncomingDamage(Data);
	}
	else if (Data.EvaluatedData.Attribute == GetIncomingXPAttribute())
	{
		ConsumeIncomingXP(Data);
	}
}

void USWAttributeSet::ConsumeIncomingDamage(const FGameplayEffectModCallbackData& Data)
{
	AActor* const TargetOwner = Data.Target.GetOwnerActor();
	const float Damage = FMath::Max(0.f, GetIncomingDamage());
	SetIncomingDamage(0.f);

	// Damage GE 的执行计算仅应在服务器写入 Meta Attribute；此处再次收口，避免预测或误用 GE 改写权威生命值。
	if (!TargetOwner || !TargetOwner->HasAuthority() || Damage <= 0.f)
	{
		return;
	}

	const float HealthBeforeDamage = GetHealth();
	const float AppliedDamage = FMath::Min(HealthBeforeDamage, Damage);
	SetHealth(FMath::Clamp(HealthBeforeDamage - Damage, 0.f, GetMaxHealth()));

	const FGameplayEffectContextHandle& EffectContext = Data.EffectSpec.GetContext();
	const FGameplayEffectContext* const RawContext = EffectContext.Get();
	const FSWGameplayEffectContext* const SWContext = RawContext && RawContext->GetScriptStruct() == FSWGameplayEffectContext::StaticStruct()
		? static_cast<const FSWGameplayEffectContext*>(RawContext)
		: nullptr;

	// 吸血只能根据实际扣除的生命结算，因而天然排除 Overkill；魔法与真实伤害的上下文比例为 0。
	if (AppliedDamage > 0.f && SWContext && SWContext->GetDamageType() == SWGameplayTags::Damage_Type_Physical)
	{
		const float LifestealRatio = SWContext->GetPhysicalLifesteal();
		if (FMath::IsFinite(LifestealRatio) && LifestealRatio > 0.f)
		{
			if (USWAbilitySystemComponent* const SourceAbilitySystemComponent = Cast<USWAbilitySystemComponent>(EffectContext.GetOriginalInstigatorAbilitySystemComponent()))
			{
				SourceAbilitySystemComponent->ApplyHealingToSelfAuthority(AppliedDamage * LifestealRatio, EffectContext.GetEffectCauser());
			}
		}
	}

	AActor* const TargetAvatar = Data.Target.GetAvatarActor();
	if (APawn* const InstigatorPawn = Cast<APawn>(EffectContext.GetOriginalInstigator()))
	{
		if (ASWPlayerController* const SourcePlayerController = Cast<ASWPlayerController>(InstigatorPawn->GetController()))
		{
			FSWDamageNumberPayload Payload;
			Payload.AppliedDamage = AppliedDamage;
			Payload.DamageType = SWContext ? SWContext->GetDamageType() : FGameplayTag();
			Payload.bCritical = SWContext && SWContext->IsCriticalHit();
			Payload.TargetActor = TargetAvatar;
			Payload.WorldLocation = TargetAvatar ? TargetAvatar->GetActorLocation() : TargetOwner->GetActorLocation();
			SourcePlayerController->ClientShowDamageNumber(Payload);
		}
	}

	if (GetHealth() > 0.f)
	{
		return;
	}

	ISWCombatInterface* const Combatant = TargetAvatar ? Cast<ISWCombatInterface>(TargetAvatar) : nullptr;
	if (!Combatant)
	{
		return;
	}

	FSWDeathContext DeathContext;
	DeathContext.VictimActor = TargetAvatar;
	DeathContext.InstigatorActor = EffectContext.GetOriginalInstigator();
	DeathContext.EffectCauser = EffectContext.GetEffectCauser();
	DeathContext.AppliedDamage = AppliedDamage;
	Combatant->TryCommitDeathAuthority(DeathContext);
}

void USWAttributeSet::ConsumeIncomingXP(const FGameplayEffectModCallbackData& Data)
{
	AActor* const TargetOwner = Data.Target.GetOwnerActor();
	const float IncomingExperience = GetIncomingXP();
	const int32 ExperienceToGrant = !FMath::IsFinite(IncomingExperience) || IncomingExperience <= 0.f
		? 0
		: (IncomingExperience >= static_cast<float>(MAX_int32) ? MAX_int32 : FMath::FloorToInt(IncomingExperience));
	SetIncomingXP(0.f);

	if (!TargetOwner || !TargetOwner->HasAuthority() || ExperienceToGrant <= 0)
	{
		return;
	}

	if (ISWPlayerProgressionInterface* const PlayerProgression = Cast<ISWPlayerProgressionInterface>(TargetOwner))
	{
		PlayerProgression->AddExperienceAuthority(ExperienceToGrant);
	}
}

void USWAttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USWAttributeSet, Health, OldValue);
}

void USWAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USWAttributeSet, MaxHealth, OldValue);
}

void USWAttributeSet::OnRep_Mana(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USWAttributeSet, Mana, OldValue);
}

void USWAttributeSet::OnRep_MaxMana(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USWAttributeSet, MaxMana, OldValue);
}

void USWAttributeSet::OnRep_Stamina(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USWAttributeSet, Stamina, OldValue);
}

void USWAttributeSet::OnRep_MaxStamina(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USWAttributeSet, MaxStamina, OldValue);
}

void USWAttributeSet::OnRep_MovementSpeedMultiplier(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USWAttributeSet, MovementSpeedMultiplier, OldValue);
}

void USWAttributeSet::OnRep_AttackPower(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USWAttributeSet, AttackPower, OldValue);
}

void USWAttributeSet::OnRep_SpellPower(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USWAttributeSet, SpellPower, OldValue);
}

void USWAttributeSet::OnRep_PhysicalArmor(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USWAttributeSet, PhysicalArmor, OldValue);
}

void USWAttributeSet::OnRep_MagicalArmor(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USWAttributeSet, MagicalArmor, OldValue);
}

void USWAttributeSet::OnRep_PhysicalPenetrationPercent(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USWAttributeSet, PhysicalPenetrationPercent, OldValue);
}

void USWAttributeSet::OnRep_MagicalPenetrationPercent(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USWAttributeSet, MagicalPenetrationPercent, OldValue);
}

void USWAttributeSet::OnRep_PhysicalPenetrationFlat(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USWAttributeSet, PhysicalPenetrationFlat, OldValue);
}

void USWAttributeSet::OnRep_MagicalPenetrationFlat(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USWAttributeSet, MagicalPenetrationFlat, OldValue);
}

void USWAttributeSet::OnRep_CriticalChance(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USWAttributeSet, CriticalChance, OldValue);
}

void USWAttributeSet::OnRep_CriticalDamage(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USWAttributeSet, CriticalDamage, OldValue);
}

void USWAttributeSet::OnRep_Tenacity(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USWAttributeSet, Tenacity, OldValue);
}

void USWAttributeSet::OnRep_PhysicalLifesteal(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USWAttributeSet, PhysicalLifesteal, OldValue);
}

void USWAttributeSet::OnRep_ManaRegeneration(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USWAttributeSet, ManaRegeneration, OldValue);
}

void USWAttributeSet::OnRep_HealthRegeneration(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USWAttributeSet, HealthRegeneration, OldValue);
}

void USWAttributeSet::OnRep_StaminaRegeneration(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USWAttributeSet, StaminaRegeneration, OldValue);
}

void USWAttributeSet::OnRep_AbilityRangeBonusPercent(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USWAttributeSet, AbilityRangeBonusPercent, OldValue);
}

void USWAttributeSet::OnRep_AbilityDurationBonusPercent(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USWAttributeSet, AbilityDurationBonusPercent, OldValue);
}

void USWAttributeSet::OnRep_CooldownReductionPercent(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USWAttributeSet, CooldownReductionPercent, OldValue);
}

void USWAttributeSet::OnRep_MagazineCapacityMultiplier(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USWAttributeSet, MagazineCapacityMultiplier, OldValue);
}

void USWAttributeSet::OnRep_FireIntervalReductionPercent(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USWAttributeSet, FireIntervalReductionPercent, OldValue);
}
