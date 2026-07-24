// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "SWTeamTypes.generated.h"

/**
 * The only team identities supported by the M02 match framework.
 *
 * None is the initial state before the server assigns a player to a match team.
 */
UENUM(BlueprintType)
enum class ESWTeamId : uint8
{
	None UMETA(DisplayName = "None"),
	TeamA UMETA(DisplayName = "Team A"),
	TeamB UMETA(DisplayName = "Team B"),
};
