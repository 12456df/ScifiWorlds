// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SWPlayerController.generated.h"

class USWInputConfig;

/**
 * ScifiWorlds 的每名玩家控制边界。
 *
 * Controller 仅在服务器与所属客户端存在；M04 起由其持有唯一输入配置，后续再按各自系统加入
 * 本地 UI 与客户端请求职责。
 */
UCLASS()
class POLYGONSCIFIWORLDS_API ASWPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	//~ Begin APlayerController interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PostProcessInput(const float DeltaTime, const bool bGamePaused) override;
	//~ End APlayerController interface

	/** 返回唯一输入配置；Pawn 只能读取，不能持有第二份配置。 */
	UFUNCTION(BlueprintPure, Category = "Input")
	const USWInputConfig* GetInputConfig() const { return InputConfig; }

protected:
	/** 由 PlayerController 蓝图默认值指定的唯一输入数据资产。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USWInputConfig> InputConfig = nullptr;

	/** 仅本地 Controller 的 IMC 生命周期入口；重生 Pawn 不会重复添加映射。 */
	void ApplyGameplayMappingContext();
	void RemoveGameplayMappingContext();
};
