// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/WidgetController/SWWidgetController.h"
#include "SWProgressionOverlayWidgetController.generated.h"

/** 玩家等级、经验条与对局金币所需的只读快照。 */
USTRUCT(BlueprintType)
struct FSWOverlayProgressionSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Overlay")
	int32 Level = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Overlay")
	int32 TotalExperience = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Overlay")
	int32 ExperienceIntoCurrentLevel = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Overlay")
	int32 ExperienceRequiredForNextLevel = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Overlay")
	float ExperiencePercent = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Overlay")
	bool bAtMaximumLevel = false;

	/** 所属 PlayerState 已复制的当前金币；其他玩家的私有金币不会进入本控制器。 */
	UPROPERTY(BlueprintReadOnly, Category = "Overlay")
	int32 Gold = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSWOnOverlayProgressionChanged, const FSWOverlayProgressionSnapshot&, Snapshot);

/** 订阅 PlayerState 的复制等级、经验和金币，并结合 GameState 成长数据生成只读快照。 */
UCLASS(BlueprintType, Blueprintable)
class POLYGONSCIFIWORLDS_API USWProgressionOverlayWidgetController : public USWWidgetController
{
	GENERATED_BODY()

public:
	virtual void BroadcastInitialValues() override;
	virtual void BindCallbacksToDependencies() override;
	virtual void BeginDestroy() override;

	UPROPERTY(BlueprintAssignable, Category = "Overlay|Progression")
	FSWOnOverlayProgressionChanged OnProgressionChanged;

private:
	void BroadcastProgression();
	void UnbindCallbacks();
	void HandleLevelChanged(int32 NewLevel);
	void HandleExperienceChanged(int32 NewExperience);
	void HandleGoldChanged(int32 NewGold);

	FDelegateHandle LevelChangedHandle;
	FDelegateHandle ExperienceChangedHandle;
	FDelegateHandle GoldChangedHandle;
	bool bCallbacksBound = false;
};
