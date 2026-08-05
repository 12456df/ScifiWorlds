// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "Player/SWPlayerController.h"

#include "AbilitySystem/SWAbilitySystemComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Input/SWInputConfig.h"
#include "Player/SWPlayerState.h"
#include "UI/HUD/SWHUD.h"

void ASWPlayerController::BeginPlay()
{
	Super::BeginPlay();

	ApplyGameplayMappingContext();
}

void ASWPlayerController::BeginPlayingState()
{
	Super::BeginPlayingState();

	RefreshOverlayWidgetControllers();
}

void ASWPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RemoveGameplayMappingContext();

	Super::EndPlay(EndPlayReason);
}

void ASWPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	RefreshOverlayWidgetControllers();
}

void ASWPlayerController::PostProcessInput(const float DeltaTime, const bool bGamePaused)
{
	Super::PostProcessInput(DeltaTime, bGamePaused);

	if (ASWPlayerState* SWPlayerState = GetPlayerState<ASWPlayerState>())
	{
		if (USWAbilitySystemComponent* AbilitySystemComponent = Cast<USWAbilitySystemComponent>(SWPlayerState->GetAbilitySystemComponent()))
		{
			AbilitySystemComponent->ProcessAbilityInput(DeltaTime, bGamePaused);
		}
	}
}

void ASWPlayerController::ClientShowDamageNumber_Implementation(const FSWDamageNumberPayload& Payload)
{
	if (!IsLocalController())
	{
		return;
	}

	BP_ShowDamageNumber(Payload);
}

void ASWPlayerController::ApplyGameplayMappingContext()
{
	if (!IsLocalController() || !InputConfig || !InputConfig->DefaultMappingContext)
	{
		return;
	}

	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			InputSubsystem->AddMappingContext(InputConfig->DefaultMappingContext, 0);
		}
	}
}

void ASWPlayerController::RemoveGameplayMappingContext()
{
	if (!IsLocalController() || !InputConfig || !InputConfig->DefaultMappingContext)
	{
		return;
	}

	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			InputSubsystem->RemoveMappingContext(InputConfig->DefaultMappingContext);
		}
	}
}

void ASWPlayerController::RefreshOverlayWidgetControllers()
{
	if (!IsLocalController())
	{
		return;
	}

	if (ASWHUD* SWHUD = GetHUD<ASWHUD>())
	{
		SWHUD->RefreshOverlayWidgetControllers();
	}
}
