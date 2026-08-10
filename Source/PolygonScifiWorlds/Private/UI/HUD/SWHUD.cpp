// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "UI/HUD/SWHUD.h"

#include "GameState/SWGameState.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Player/SWPlayerController.h"
#include "Player/SWPlayerState.h"
#include "UI/Widget/SWUserWidget.h"
#include "UI/WidgetController/Overlay/SWAttributeOverlayWidgetController.h"
#include "UI/WidgetController/Overlay/SWProgressionOverlayWidgetController.h"
#include "UI/WidgetController/Overlay/SWSkillOverlayWidgetController.h"
#include "UI/WidgetController/Overlay/SWWeaponOverlayWidgetController.h"
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

USWAttributeOverlayWidgetController* ASWHUD::GetAttributeOverlayWidgetController()
{
	if (AttributeOverlayWidgetController)
	{
		return AttributeOverlayWidgetController;
	}

	ASWPlayerController* SWPlayerController = Cast<ASWPlayerController>(PlayerOwner);
	if (!SWPlayerController || !SWPlayerController->IsLocalController())
	{
		return nullptr;
	}

	UClass* ControllerClass = AttributeOverlayWidgetControllerClass.Get();
	if (!ControllerClass)
	{
		ControllerClass = USWAttributeOverlayWidgetController::StaticClass();
	}

	AttributeOverlayWidgetController = NewObject<USWAttributeOverlayWidgetController>(this, ControllerClass);
	if (AttributeOverlayWidgetController)
	{
		const FSWWidgetControllerParams Params(SWPlayerController, SWPlayerController->GetPlayerState<ASWPlayerState>(), GetWorld() ? GetWorld()->GetGameState<ASWGameState>() : nullptr);
		AttributeOverlayWidgetController->SetWidgetControllerParams(Params);
		AttributeOverlayWidgetController->BindCallbacksToDependencies();
		AttributeOverlayWidgetController->BroadcastInitialValues();
	}

	return AttributeOverlayWidgetController;
}

USWWeaponOverlayWidgetController* ASWHUD::GetWeaponOverlayWidgetController()
{
	if (WeaponOverlayWidgetController)
	{
		return WeaponOverlayWidgetController;
	}

	ASWPlayerController* SWPlayerController = Cast<ASWPlayerController>(PlayerOwner);
	if (!SWPlayerController || !SWPlayerController->IsLocalController())
	{
		return nullptr;
	}

	UClass* ControllerClass = WeaponOverlayWidgetControllerClass.Get();
	if (!ControllerClass)
	{
		ControllerClass = USWWeaponOverlayWidgetController::StaticClass();
	}

	WeaponOverlayWidgetController = NewObject<USWWeaponOverlayWidgetController>(this, ControllerClass);
	if (WeaponOverlayWidgetController)
	{
		const FSWWidgetControllerParams Params(SWPlayerController, SWPlayerController->GetPlayerState<ASWPlayerState>(), GetWorld() ? GetWorld()->GetGameState<ASWGameState>() : nullptr);
		WeaponOverlayWidgetController->SetWidgetControllerParams(Params);
		WeaponOverlayWidgetController->BindCallbacksToDependencies();
		WeaponOverlayWidgetController->BroadcastInitialValues();
	}

	return WeaponOverlayWidgetController;
}

USWProgressionOverlayWidgetController* ASWHUD::GetProgressionOverlayWidgetController()
{
	if (ProgressionOverlayWidgetController)
	{
		return ProgressionOverlayWidgetController;
	}

	ASWPlayerController* SWPlayerController = Cast<ASWPlayerController>(PlayerOwner);
	if (!SWPlayerController || !SWPlayerController->IsLocalController())
	{
		return nullptr;
	}

	UClass* ControllerClass = ProgressionOverlayWidgetControllerClass.Get();
	if (!ControllerClass)
	{
		ControllerClass = USWProgressionOverlayWidgetController::StaticClass();
	}

	ProgressionOverlayWidgetController = NewObject<USWProgressionOverlayWidgetController>(this, ControllerClass);
	if (ProgressionOverlayWidgetController)
	{
		const FSWWidgetControllerParams Params(SWPlayerController, SWPlayerController->GetPlayerState<ASWPlayerState>(), GetWorld() ? GetWorld()->GetGameState<ASWGameState>() : nullptr);
		ProgressionOverlayWidgetController->SetWidgetControllerParams(Params);
		ProgressionOverlayWidgetController->BindCallbacksToDependencies();
		ProgressionOverlayWidgetController->BroadcastInitialValues();
	}

	return ProgressionOverlayWidgetController;
}

USWSkillOverlayWidgetController* ASWHUD::GetSkillOverlayWidgetController()
{
	if (SkillOverlayWidgetController)
	{
		return SkillOverlayWidgetController;
	}

	ASWPlayerController* SWPlayerController = Cast<ASWPlayerController>(PlayerOwner);
	if (!SWPlayerController || !SWPlayerController->IsLocalController())
	{
		return nullptr;
	}

	UClass* ControllerClass = SkillOverlayWidgetControllerClass.Get();
	if (!ControllerClass)
	{
		ControllerClass = USWSkillOverlayWidgetController::StaticClass();
	}

	SkillOverlayWidgetController = NewObject<USWSkillOverlayWidgetController>(this, ControllerClass);
	if (SkillOverlayWidgetController)
	{
		const FSWWidgetControllerParams Params(SWPlayerController, SWPlayerController->GetPlayerState<ASWPlayerState>(), GetWorld() ? GetWorld()->GetGameState<ASWGameState>() : nullptr);
		SkillOverlayWidgetController->SetWidgetControllerParams(Params);
		SkillOverlayWidgetController->BindCallbacksToDependencies();
		SkillOverlayWidgetController->BroadcastInitialValues();
	}

	return SkillOverlayWidgetController;
}

void ASWHUD::RefreshOverlayWidgetControllers()
{
	ASWPlayerController* SWPlayerController = Cast<ASWPlayerController>(PlayerOwner);
	if (!SWPlayerController || !SWPlayerController->IsLocalController())
	{
		return;
	}

	const FSWWidgetControllerParams Params(
		SWPlayerController,
		SWPlayerController->GetPlayerState<ASWPlayerState>(),
		GetWorld() ? GetWorld()->GetGameState<ASWGameState>() : nullptr);

	// HUD 可能早于客户端 PlayerState 创建；在其就绪后统一重绑已存在的控制器。
	if (AttributeOverlayWidgetController)
	{
		AttributeOverlayWidgetController->SetWidgetControllerParams(Params);
		AttributeOverlayWidgetController->BindCallbacksToDependencies();
		AttributeOverlayWidgetController->BroadcastInitialValues();
	}

	if (WeaponOverlayWidgetController)
	{
		WeaponOverlayWidgetController->SetWidgetControllerParams(Params);
		WeaponOverlayWidgetController->BindCallbacksToDependencies();
		WeaponOverlayWidgetController->BroadcastInitialValues();
	}

	if (ProgressionOverlayWidgetController)
	{
		ProgressionOverlayWidgetController->SetWidgetControllerParams(Params);
		ProgressionOverlayWidgetController->BindCallbacksToDependencies();
		ProgressionOverlayWidgetController->BroadcastInitialValues();
	}

	if (SkillOverlayWidgetController)
	{
		SkillOverlayWidgetController->SetWidgetControllerParams(Params);
		SkillOverlayWidgetController->BindCallbacksToDependencies();
		SkillOverlayWidgetController->BroadcastInitialValues();
	}
}
