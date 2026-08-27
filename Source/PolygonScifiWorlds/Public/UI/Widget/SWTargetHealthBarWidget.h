// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SWTargetHealthBarWidget.generated.h"

/**
 * 世界目标血条的最小数据契约。
 *
 * C++ 只负责将服务器已确认、已复制的生命快照交给本地 Widget；
 * 具体 ProgressBar、颜色与入场/退场动画由派生 WBP 负责。
 */
UCLASS(Abstract, Blueprintable)
class POLYGONSCIFIWORLDS_API USWTargetHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 仅由目标头顶血条组件在本地客户端调用，不写入任何玩法状态。 */
	UFUNCTION(BlueprintCallable, Category = "UI|Target Health Bar")
	void SetHealthSnapshot(float InCurrentHealth, float InMaximumHealth);

	UFUNCTION(BlueprintPure, Category = "UI|Target Health Bar")
	float GetHealthPercent() const { return HealthPercent; }

protected:
	UPROPERTY(BlueprintReadOnly, Category = "UI|Target Health Bar")
	float CurrentHealth = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "UI|Target Health Bar")
	float MaximumHealth = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "UI|Target Health Bar")
	float HealthPercent = 0.f;

	/** 派生 WBP 在此更新 ProgressBar；不得据此修改任何 GAS Attribute。 */
	UFUNCTION(BlueprintImplementableEvent, Category = "UI|Target Health Bar")
	void BP_OnHealthSnapshotChanged(float InCurrentHealth, float InMaximumHealth, float InHealthPercent);
};
