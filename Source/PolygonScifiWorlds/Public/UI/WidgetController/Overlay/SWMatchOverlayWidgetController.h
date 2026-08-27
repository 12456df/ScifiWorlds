// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameState/SWMatchResultTypes.h"
#include "TimerManager.h"
#include "UI/WidgetController/SWWidgetController.h"
#include "SWMatchOverlayWidgetController.generated.h"

/** 比赛 Overlay 所需的只读公共快照；所有数值均来自已复制的 GameState。 */
USTRUCT(BlueprintType)
struct FSWMatchOverlaySnapshot
{
	GENERATED_BODY()

	/** 当前引擎比赛阶段，例如 WaitingToStart、InProgress、WaitingPostMatch。 */
	UPROPERTY(BlueprintReadOnly, Category = "Match Overlay")
	FName MatchState = NAME_None;

	/** 正式比赛已进行的完整秒数；仅 InProgress 期间每秒更新。 */
	UPROPERTY(BlueprintReadOnly, Category = "Match Overlay")
	int32 ElapsedMatchSeconds = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Match Overlay")
	int32 TeamAKillCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Match Overlay")
	int32 TeamBKillCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Match Overlay")
	int32 TeamATowerDestroyCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Match Overlay")
	int32 TeamBTowerDestroyCount = 0;

	/** 保留赛后胜负/平局结果，供未来结算表现直接读取。 */
	UPROPERTY(BlueprintReadOnly, Category = "Match Overlay")
	FSWMatchResult MatchResult;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSWOnMatchOverlayChanged, const FSWMatchOverlaySnapshot&, Snapshot);

/**
 * 本地 Overlay 的比赛公共数据入口。
 * 只订阅 GameState 的复制结果与统计，不拥有比赛规则，也不驱动服务器状态。
 */
UCLASS(BlueprintType, Blueprintable)
class POLYGONSCIFIWORLDS_API USWMatchOverlayWidgetController : public USWWidgetController
{
	GENERATED_BODY()

public:
	virtual void BroadcastInitialValues() override;
	virtual void BindCallbacksToDependencies() override;
	virtual void BeginDestroy() override;

	/** 阶段、双方击杀/推塔数、结果或每秒比赛时间变化时广播。 */
	UPROPERTY(BlueprintAssignable, Category = "Overlay|Match")
	FSWOnMatchOverlayChanged OnMatchOverlayChanged;

private:
	void BroadcastMatchOverlay();
	/**
	 * 解除对 GameState 的订阅。
	 * 正常重绑时清理本地计时器；GC 销毁阶段不得再访问 World/GameInstance 的计时器。
	 */
	void UnbindCallbacks(bool bClearElapsedTimeTimer = true);
	void RefreshElapsedTimeTimer();
	void HandleMatchStateChanged(FName NewMatchState);
	void HandleTeamMatchStatsChanged();
	void HandleElapsedTimeTimer();

	UFUNCTION()
	void HandleMatchResultChanged(const FSWMatchResult& NewMatchResult);

	FDelegateHandle MatchStateChangedHandle;
	FDelegateHandle TeamMatchStatsChangedHandle;
	FTimerHandle ElapsedTimeRefreshTimer;
	bool bCallbacksBound = false;
};
