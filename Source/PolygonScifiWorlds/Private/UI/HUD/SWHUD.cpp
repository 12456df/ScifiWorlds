// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "UI/HUD/SWHUD.h"

#include "GameState/SWGameState.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Player/SWPlayerController.h"
#include "Player/SWPlayerState.h"
#include "UI/Widget/SWUserWidget.h"
#include "UI/WidgetController/SWNetworkDiagnosticsWidgetController.h"
#include "UI/WidgetController/SWWidgetController.h"

void ASWHUD::BeginPlay()
{
	Super::BeginPlay();
	CreateRootWidget();
}

USWUserWidget* ASWHUD::CreateRootWidget()
{
	if (RootWidget)
	{
		return RootWidget;
	}

	APlayerController* OwningPlayerController = PlayerOwner;
	if (!RootWidgetClass || !OwningPlayerController || !OwningPlayerController->IsLocalController())
	{
		return nullptr;
	}

	RootWidget = CreateWidget<USWUserWidget>(OwningPlayerController, RootWidgetClass);
	if (!RootWidget)
	{
		return nullptr;
	}

	RootWidget->AddToPlayerScreen();
	return RootWidget;
}

USWNetworkDiagnosticsWidgetController* ASWHUD::GetNetworkDiagnosticsWidgetController()
{
	if (NetworkDiagnosticsWidgetController)
	{
		return NetworkDiagnosticsWidgetController;
	}

	ASWPlayerController* SWPlayerController = Cast<ASWPlayerController>(PlayerOwner);
	if (!SWPlayerController || !SWPlayerController->IsLocalController())
	{
		return nullptr;
	}

	UClass* ControllerClass = NetworkDiagnosticsWidgetControllerClass.Get();
	if (!ControllerClass)
	{
		ControllerClass = USWNetworkDiagnosticsWidgetController::StaticClass();
	}

	NetworkDiagnosticsWidgetController = NewObject<USWNetworkDiagnosticsWidgetController>(this, ControllerClass);
	if (!NetworkDiagnosticsWidgetController)
	{
		return nullptr;
	}

	const FSWWidgetControllerParams Params(
		SWPlayerController,
		SWPlayerController->GetPlayerState<ASWPlayerState>(),
		GetWorld() ? GetWorld()->GetGameState<ASWGameState>() : nullptr);
	NetworkDiagnosticsWidgetController->SetWidgetControllerParams(Params);
	NetworkDiagnosticsWidgetController->BindCallbacksToDependencies();
	NetworkDiagnosticsWidgetController->BroadcastInitialValues();
	return NetworkDiagnosticsWidgetController;
}
