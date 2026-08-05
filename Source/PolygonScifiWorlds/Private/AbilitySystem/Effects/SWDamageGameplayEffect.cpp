// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "AbilitySystem/Effects/SWDamageGameplayEffect.h"

#include "AbilitySystem/Executions/SWExecCalc_Damage.h"

USWDamageGameplayEffect::USWDamageGameplayEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayEffectExecutionDefinition& DamageExecution = Executions.AddDefaulted_GetRef();
	DamageExecution.CalculationClass = USWExecCalc_Damage::StaticClass();
}
