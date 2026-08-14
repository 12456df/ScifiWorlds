// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "UI/Widget/SWUserWidget.h"

#include "UI/WidgetController/SWWidgetController.h"

void USWUserWidget::SetWidgetController(UObject* InWidgetController)
{
	WidgetController = InWidgetController;
	BP_OnWidgetControllerSet();

	// 先让蓝图完成 Delegate 绑定，再拉取当前快照，避免 HUD 创建控制器时的首包被丢失。
	if (USWWidgetController* const SWWidgetController = Cast<USWWidgetController>(WidgetController))
	{
		SWWidgetController->BroadcastInitialValues();
	}
}
