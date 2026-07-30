// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "Input/SWInputConfig.h"

#include "InputAction.h"

const UInputAction* USWInputConfig::FindAbilityInputActionForTag(const FGameplayTag InputTag) const
{
	if (!InputTag.IsValid())
	{
		return nullptr;
	}

	for (const FSWAbilityInputAction& AbilityInputAction : AbilityInputActions)
	{
		if (AbilityInputAction.InputTag == InputTag)
		{
			return AbilityInputAction.InputAction;
		}
	}

	return nullptr;
}
