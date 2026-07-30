// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "UI/Widget/SWUserWidget.h"

void USWUserWidget::SetWidgetController(UObject* InWidgetController)
{
	WidgetController = InWidgetController;
	BP_OnWidgetControllerSet();
}
