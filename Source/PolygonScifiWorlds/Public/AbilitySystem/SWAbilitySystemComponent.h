// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "SWAbilitySystemComponent.generated.h"

/**
 * Project ability system component for ScifiWorlds.
 *
 * Owns ability, effect, tag and attribute aggregation as described in
 * Docs/Systems/M03_GASCoreFramework.md. This module establishes the type; ability
 * granting, input binding and effect helpers land with later delivery steps.
 */
UCLASS()
class POLYGONSCIFIWORLDS_API USWAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()
};
