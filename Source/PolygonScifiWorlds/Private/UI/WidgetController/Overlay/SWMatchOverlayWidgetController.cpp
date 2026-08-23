// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "UI/WidgetController/Overlay/SWMatchOverlayWidgetController.h"

#include "GameState/SWGameState.h"
#include "GameFramework/GameMode.h"
#include "TimerManager.h"

void USWMatchOverlayWidgetController::BroadcastInitialValues()
{
	RefreshElapsedTimeTimer();
	BroadcastMatchOverlay();
}

void USWMatchOverlayWidgetController::BindCallbacksToDependencies()
{
	UnbindCallbacks();
	if (!GameState)
	{
		return;
	}

	MatchStateChangedHandle = GameState->OnSWMatchStateChanged.AddUObject(this, &ThisClass::HandleMatchStateChanged);
	TeamMatchStatsChangedHandle = GameState->OnTeamMatchStatsChanged.AddUObject(this, &ThisClass::HandleTeamMatchStatsChanged);
	GameState->OnMatchResultChanged.AddDynamic(this, &ThisClass::HandleMatchResultChanged);
	bCallbacksBound = true;
	RefreshElapsedTimeTimer();
}

void USWMatchOverlayWidgetController::BeginDestroy()
{
	UnbindCallbacks();
	Super::BeginDestroy();
}

void USWMatchOverlayWidgetController::BroadcastMatchOverlay()
{
	FSWMatchOverlaySnapshot Snapshot;
	if (GameState)
	{
		Snapshot.MatchState = GameState->GetMatchState();
		Snapshot.ElapsedMatchSeconds = FMath::Max(0, FMath::FloorToInt(GameState->GetMatchElapsedSeconds()));

		const FSWTeamMatchStats TeamAStats = GameState->GetTeamMatchStats(ESWTeamId::TeamA);
		Snapshot.TeamAKillCount = TeamAStats.KillCount;
		Snapshot.TeamATowerDestroyCount = TeamAStats.TowerDestroyCount;

		const FSWTeamMatchStats TeamBStats = GameState->GetTeamMatchStats(ESWTeamId::TeamB);
		Snapshot.TeamBKillCount = TeamBStats.KillCount;
		Snapshot.TeamBTowerDestroyCount = TeamBStats.TowerDestroyCount;
		Snapshot.MatchResult = GameState->GetMatchResult();
	}

	OnMatchOverlayChanged.Broadcast(Snapshot);
}

void USWMatchOverlayWidgetController::UnbindCallbacks()
{
	if (GameState)
	{
		GameState->GetWorldTimerManager().ClearTimer(ElapsedTimeRefreshTimer);
	}

	if (bCallbacksBound && GameState)
	{
		GameState->OnSWMatchStateChanged.Remove(MatchStateChangedHandle);
		GameState->OnTeamMatchStatsChanged.Remove(TeamMatchStatsChangedHandle);
		GameState->OnMatchResultChanged.RemoveDynamic(this, &ThisClass::HandleMatchResultChanged);
	}

	MatchStateChangedHandle.Reset();
	TeamMatchStatsChangedHandle.Reset();
	ElapsedTimeRefreshTimer.Invalidate();
	bCallbacksBound = false;
}

void USWMatchOverlayWidgetController::RefreshElapsedTimeTimer()
{
	if (!GameState)
	{
		return;
	}

	FTimerManager& TimerManager = GameState->GetWorldTimerManager();
	TimerManager.ClearTimer(ElapsedTimeRefreshTimer);
	if (GameState->GetMatchState() == MatchState::InProgress)
	{
		TimerManager.SetTimer(
			ElapsedTimeRefreshTimer,
			this,
			&ThisClass::HandleElapsedTimeTimer,
			1.0f,
			true);
	}
}

void USWMatchOverlayWidgetController::HandleMatchStateChanged(const FName NewMatchState)
{
	(void)NewMatchState;
	RefreshElapsedTimeTimer();
	BroadcastMatchOverlay();
}

void USWMatchOverlayWidgetController::HandleTeamMatchStatsChanged()
{
	BroadcastMatchOverlay();
}

void USWMatchOverlayWidgetController::HandleElapsedTimeTimer()
{
	BroadcastMatchOverlay();
}

void USWMatchOverlayWidgetController::HandleMatchResultChanged(const FSWMatchResult& NewMatchResult)
{
	(void)NewMatchResult;
	BroadcastMatchOverlay();
}
