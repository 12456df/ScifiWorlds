// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "AbilitySystem/Effects/SWCommonBuffGameplayEffects.h"

#include "AbilitySystem/SWAttributeSet.h"
#include "GameplayTags/SWGameplayTags.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

namespace
{
	void AddGrantedTargetTag(UGameplayEffect& GameplayEffect, const FGameplayTag Tag)
	{
		FInheritedTagContainer TargetTags;
		TargetTags.Added.AddTag(Tag);
		GameplayEffect.FindOrAddComponent<UTargetTagsGameplayEffectComponent>().SetAndApplyTargetTagChanges(TargetTags);
	}

	void SetAggregateBySourceStacking(UGameplayEffect& GameplayEffect)
	{
#if WITH_EDITOR
		GameplayEffect.SetStackingType(EGameplayEffectStackingType::AggregateBySource);
#else
		// UE 5.7 的 SetStackingType 仅在 WITH_EDITOR 下公开；Dedicated Server 仍需要
		// 在 CDO 构造阶段写入同一设置，直到引擎提供运行时 setter 为止。
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		GameplayEffect.StackingType = EGameplayEffectStackingType::AggregateBySource;
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
#endif
	}
}

USWDurationBuffGameplayEffect::USWDurationBuffGameplayEffect()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	FSetByCallerFloat DurationByCaller;
	DurationByCaller.DataTag = SWGameplayTags::SetByCaller_Ability_Duration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(DurationByCaller);
	SetAggregateBySourceStacking(*this);
}

USWSpeedBuffGameplayEffect::USWSpeedBuffGameplayEffect()
{
	AddGrantedTargetTag(*this, SWGameplayTags::State_Buff_Speed);
	FGameplayModifierInfo& Modifier = Modifiers.AddDefaulted_GetRef();
	Modifier.Attribute = USWAttributeSet::GetMovementSpeedMultiplierAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;
	FSetByCallerFloat Magnitude;
	Magnitude.DataTag = SWGameplayTags::SetByCaller_Buff_MovementSpeedDelta;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(Magnitude);
}

USWHealBuffGameplayEffect::USWHealBuffGameplayEffect()
{
	AddGrantedTargetTag(*this, SWGameplayTags::State_Buff_Heal);
	Period.Value = 1.f;
	bExecutePeriodicEffectOnApplication = false;
	FGameplayModifierInfo& Modifier = Modifiers.AddDefaulted_GetRef();
	Modifier.Attribute = USWAttributeSet::GetHealthAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;
	FSetByCallerFloat Magnitude;
	Magnitude.DataTag = SWGameplayTags::SetByCaller_Buff_HealthPerSecond;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(Magnitude);
}

USWStunDebuffGameplayEffect::USWStunDebuffGameplayEffect()
{
	AddGrantedTargetTag(*this, SWGameplayTags::State_Debuff_Stunned);

	FGameplayModifierInfo& Modifier = Modifiers.AddDefaulted_GetRef();
	Modifier.Attribute = USWAttributeSet::GetMovementSpeedMultiplierAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;
	FSetByCallerFloat Magnitude;
	Magnitude.DataTag = SWGameplayTags::SetByCaller_Buff_MovementSpeedDelta;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(Magnitude);
}

USWPoisonDebuffGameplayEffect::USWPoisonDebuffGameplayEffect()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	FSetByCallerFloat DurationByCaller;
	DurationByCaller.DataTag = SWGameplayTags::SetByCaller_Ability_Duration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(DurationByCaller);
	SetAggregateBySourceStacking(*this);
	AddGrantedTargetTag(*this, SWGameplayTags::State_Debuff_Poisoned);
	Period.Value = 1.f;
	bExecutePeriodicEffectOnApplication = false;
}
