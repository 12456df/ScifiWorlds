// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SWUserWidget.generated.h"

/**
 * 项目运行时 UMG 的共同基类。
 *
 * 当前只提供 WidgetController 注入契约；具体控制器、属性绑定和完整 HUD
 * 留待对应 UI 模块实现，避免在 M04 提前建立 UI 业务层。
 */
UCLASS(Abstract, Blueprintable)
class POLYGONSCIFIWORLDS_API USWUserWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 为 Widget 注入只读数据控制器，并通知蓝图完成后续表现绑定。 */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetWidgetController(UObject* InWidgetController);

	UFUNCTION(BlueprintPure, Category = "UI")
	UObject* GetWidgetController() const { return WidgetController; }

protected:
	/** 当前 Widget 的数据控制器；目前允许为空。 */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "UI")
	TObjectPtr<UObject> WidgetController;

	/** WidgetController 设置完成后调用，蓝图只在此处绑定表现，不写入玩法状态。 */
	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void BP_OnWidgetControllerSet();
};
