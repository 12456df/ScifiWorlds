// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SWWidgetController.generated.h"

class ASWGameState;
class ASWPlayerController;
class ASWPlayerState;

/** WidgetController 初始化时所需的运行时数据源。 */
USTRUCT(BlueprintType)
struct FSWWidgetControllerParams
{
	GENERATED_BODY()

	FSWWidgetControllerParams() = default;
	FSWWidgetControllerParams(ASWPlayerController* InPlayerController, ASWPlayerState* InPlayerState, ASWGameState* InGameState)
		: PlayerController(InPlayerController)
		, PlayerState(InPlayerState)
		, GameState(InGameState)
	{
	}

	/** 仅本地玩家拥有；负责连接本机输入、视口与网络连接。 */
	UPROPERTY(BlueprintReadOnly, Category = "WidgetController")
	TObjectPtr<ASWPlayerController> PlayerController = nullptr;

	/** 已复制的玩家长期状态；供后续个人 HUD 控制器复用。 */
	UPROPERTY(BlueprintReadOnly, Category = "WidgetController")
	TObjectPtr<ASWPlayerState> PlayerState = nullptr;

	/** 已复制的对局公共状态；供全局 HUD 控制器复用。 */
	UPROPERTY(BlueprintReadOnly, Category = "WidgetController")
	TObjectPtr<ASWGameState> GameState = nullptr;
};

/**
 * UI 的只读数据连接基类。
 * WidgetController 不拥有玩法状态，不创建 Widget，也不承担 PlayerController 的输入职责。
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class POLYGONSCIFIWORLDS_API USWWidgetController : public UObject
{
	GENERATED_BODY()

public:
	/** HUD 创建控制器后调用一次，注入该本地玩家可读取的运行时对象。 */
	UFUNCTION(BlueprintCallable, Category = "WidgetController")
	virtual void SetWidgetControllerParams(const FSWWidgetControllerParams& InParams);

	/** 向已绑定的蓝图广播首个快照。 */
	UFUNCTION(BlueprintCallable, Category = "WidgetController")
	virtual void BroadcastInitialValues();

	/** 绑定数据源回调；由 HUD 在注入参数后调用一次。 */
	virtual void BindCallbacksToDependencies();

protected:
	UPROPERTY(BlueprintReadOnly, Category = "WidgetController")
	TObjectPtr<ASWPlayerController> PlayerController;

	UPROPERTY(BlueprintReadOnly, Category = "WidgetController")
	TObjectPtr<ASWPlayerState> PlayerState;

	UPROPERTY(BlueprintReadOnly, Category = "WidgetController")
	TObjectPtr<ASWGameState> GameState;
};
