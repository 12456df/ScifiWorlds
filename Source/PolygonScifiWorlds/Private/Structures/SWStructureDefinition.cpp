// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "Structures/SWStructureDefinition.h"

#if WITH_EDITOR
EDataValidationResult USWStructureDefinition::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	auto AddError = [&Context, &Result](const TCHAR* Message)
	{
		Context.AddError(FText::FromString(Message));
		Result = EDataValidationResult::Invalid;
	};

	if (!CombatantDefinition)
	{
		AddError(TEXT("CombatantDefinition must be assigned."));
	}
	if (CombatLevel < 1)
	{
		AddError(TEXT("CombatLevel must be at least 1."));
	}
	if (!FMath::IsFinite(CombatRadius) || CombatRadius <= 0.f)
	{
		AddError(TEXT("CombatRadius must be finite and greater than zero."));
	}
	if (!FMath::IsFinite(DamageReductionPercent) || DamageReductionPercent < 0.f || DamageReductionPercent > 1.f)
	{
		AddError(TEXT("DamageReductionPercent must be finite and within [0, 1]."));
	}
	if (!AttackAbilityClass)
	{
		AddError(TEXT("AttackAbilityClass must be assigned."));
	}
	if (!BehaviorTree)
	{
		AddError(TEXT("BehaviorTree must be assigned."));
	}

	return Result;
}
#endif
