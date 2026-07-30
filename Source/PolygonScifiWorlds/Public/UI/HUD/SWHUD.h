// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "SWHUD.generated.h"

class USWUserWidget;
class USWNetworkDiagnosticsWidgetController;

/**
 * 本地游戏 UI 的根创建入口。
 *
 * 仅在本地 PlayerController 上创建配置的根 Widget；Dedicated Server 和远端
 * 玩家不会创建 UMG。具体准星、属性栏和菜单由后续模块通过独立 Widget 配置。
 */
UCLASS()
class POLYGONSCIFIWORLDS_API ASWHUD : public AHUD
{
	GENERATED_BODY()

public:
	/** 返回已创建的根 Widget；未配置或非本地 HUD 时返回 nullptr。 */
	UFUNCTION(BlueprintPure, Category = "UI")
	USWUserWidget* GetRootWidget() const { return RootWidget; }

	/** 创建根 Widget；可重复调用但只会创建一个实例。 */
	UFUNCTION(BlueprintCallable, Category = "UI")
	USWUserWidget* CreateRootWidget();

	/**
	 * 返回此本地 HUD 唯一的网络诊断数据控制器。
	 * 控制器只读采样网络状态；Widget 的创建和显示仍由 HUD/蓝图负责。
	 */
	UFUNCTION(BlueprintCallable, Category = "UI|Network Diagnostics")
	USWNetworkDiagnosticsWidgetController* GetNetworkDiagnosticsWidgetController();

protected:
	virtual void BeginPlay() override;

	/** 由 HUD 蓝图选择的根 Widget 类；为空时 HUD 保持无 UI。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<USWUserWidget> RootWidgetClass;

	/** 本地 Viewport 中根 Widget 的运行时实例。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "UI")
	TObjectPtr<USWUserWidget> RootWidget;

	/** 可由 HUD 蓝图替换为专用的 WidgetController 子类。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Network Diagnostics")
	TSubclassOf<USWNetworkDiagnosticsWidgetController> NetworkDiagnosticsWidgetControllerClass;

	/** 本地 HUD 缓存的只读网络诊断数据控制器。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "UI|Network Diagnostics")
	TObjectPtr<USWNetworkDiagnosticsWidgetController> NetworkDiagnosticsWidgetController;
};
