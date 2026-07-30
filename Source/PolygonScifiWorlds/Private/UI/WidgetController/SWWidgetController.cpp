// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "UI/WidgetController/SWWidgetController.h"

void USWWidgetController::SetWidgetControllerParams(const FSWWidgetControllerParams& InParams)
{
	PlayerController = InParams.PlayerController;
	PlayerState = InParams.PlayerState;
	GameState = InParams.GameState;
}

void USWWidgetController::BroadcastInitialValues()
{
}

void USWWidgetController::BindCallbacksToDependencies()
{
}
