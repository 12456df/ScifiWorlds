// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "UI/HUD/SWHUD.h"

#include "GameState/SWGameState.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Player/SWPlayerController.h"
#include "Player/SWPlayerState.h"
#include "UI/Widget/SWUserWidget.h"
#include "UI/WidgetController/Overlay/SWAttributeOverlayWidgetController.h"
#include "UI/WidgetController/Overlay/SWEquipmentOverlayWidgetController.h"
#include "UI/WidgetController/Overlay/SWMatchOverlayWidgetController.h"
#include "UI/WidgetController/Overlay/SWProgressionOverlayWidgetController.h"
#include "UI/WidgetController/Overlay/SWSkillOverlayWidgetController.h"
#include "UI/WidgetController/Overlay/SWWeaponOverlayWidgetController.h"
#include "UI/WidgetController/SWShopWidgetController.h"
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
	}

	return SkillOverlayWidgetController;
}

USWEquipmentOverlayWidgetController* ASWHUD::GetEquipmentOverlayWidgetController()
{
	if (EquipmentOverlayWidgetController)
	{
		return EquipmentOverlayWidgetController;
	}

	ASWPlayerController* SWPlayerController = Cast<ASWPlayerController>(PlayerOwner);
	if (!SWPlayerController || !SWPlayerController->IsLocalController())
	{
		return nullptr;
	}

	UClass* ControllerClass = EquipmentOverlayWidgetControllerClass.Get();
	if (!ControllerClass)
	{
		ControllerClass = USWEquipmentOverlayWidgetController::StaticClass();
	}

	EquipmentOverlayWidgetController = NewObject<USWEquipmentOverlayWidgetController>(this, ControllerClass);
	if (EquipmentOverlayWidgetController)
	{
		const FSWWidgetControllerParams Params(SWPlayerController, SWPlayerController->GetPlayerState<ASWPlayerState>(), GetWorld() ? GetWorld()->GetGameState<ASWGameState>() : nullptr);
		EquipmentOverlayWidgetController->SetWidgetControllerParams(Params);
		EquipmentOverlayWidgetController->BindCallbacksToDependencies();
	}

	return EquipmentOverlayWidgetController;
}

USWMatchOverlayWidgetController* ASWHUD::GetMatchOverlayWidgetController()
{
	if (MatchOverlayWidgetController)
	{
		return MatchOverlayWidgetController;
	}

	ASWPlayerController* const SWPlayerController = Cast<ASWPlayerController>(PlayerOwner);
	if (!SWPlayerController || !SWPlayerController->IsLocalController())
	{
		return nullptr;
	}

	UClass* ControllerClass = MatchOverlayWidgetControllerClass.Get();
	if (!ControllerClass)
	{
		ControllerClass = USWMatchOverlayWidgetController::StaticClass();
	}

	MatchOverlayWidgetController = NewObject<USWMatchOverlayWidgetController>(this, ControllerClass);
	if (MatchOverlayWidgetController)
	{
		const FSWWidgetControllerParams Params(SWPlayerController, SWPlayerController->GetPlayerState<ASWPlayerState>(), GetWorld() ? GetWorld()->GetGameState<ASWGameState>() : nullptr);
		MatchOverlayWidgetController->SetWidgetControllerParams(Params);
		MatchOverlayWidgetController->BindCallbacksToDependencies();
	}

	return MatchOverlayWidgetController;
}

USWShopWidgetController* ASWHUD::GetShopWidgetController()
{
	if (ShopWidgetController)
	{
		return ShopWidgetController;
	}

	ASWPlayerController* const SWPlayerController = Cast<ASWPlayerController>(PlayerOwner);
	if (!SWPlayerController || !SWPlayerController->IsLocalController())
	{
		return nullptr;
	}

	UClass* ControllerClass = ShopWidgetControllerClass.Get();
	if (!ControllerClass)
	{
		ControllerClass = USWShopWidgetController::StaticClass();
	}

	ShopWidgetController = NewObject<USWShopWidgetController>(this, ControllerClass);
	if (ShopWidgetController)
	{
		const FSWWidgetControllerParams Params(SWPlayerController, SWPlayerController->GetPlayerState<ASWPlayerState>(), GetWorld() ? GetWorld()->GetGameState<ASWGameState>() : nullptr);
		ShopWidgetController->SetWidgetControllerParams(Params);
		ShopWidgetController->BindCallbacksToDependencies();
	}

	return ShopWidgetController;
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

	if (EquipmentOverlayWidgetController)
	{
		EquipmentOverlayWidgetController->SetWidgetControllerParams(Params);
		EquipmentOverlayWidgetController->BindCallbacksToDependencies();
		EquipmentOverlayWidgetController->BroadcastInitialValues();
	}

	if (MatchOverlayWidgetController)
	{
		MatchOverlayWidgetController->SetWidgetControllerParams(Params);
		MatchOverlayWidgetController->BindCallbacksToDependencies();
		MatchOverlayWidgetController->BroadcastInitialValues();
	}

	if (ShopWidgetController)
	{
		ShopWidgetController->SetWidgetControllerParams(Params);
		ShopWidgetController->BindCallbacksToDependencies();
		ShopWidgetController->BroadcastInitialValues();
	}
}
