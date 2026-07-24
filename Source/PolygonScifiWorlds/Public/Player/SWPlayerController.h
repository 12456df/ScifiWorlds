// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SWPlayerController.generated.h"

/**
 * Per-player control boundary for ScifiWorlds.
 *
 * This controller exists on the server and its owning client only. M02 keeps
 * it intentionally empty: input belongs to M04, while client requests and
 * local HUD responsibilities will be added by their owning systems.
 */
UCLASS()
class POLYGONSCIFIWORLDS_API ASWPlayerController : public APlayerController
{
	GENERATED_BODY()
};
