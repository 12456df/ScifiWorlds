// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "UI/WidgetController/Overlay/SWProgressionOverlayWidgetController.h"

#include "AbilitySystem/Data/SWProgressionData.h"
#include "GameState/SWGameState.h"
#include "Player/SWPlayerState.h"

void USWProgressionOverlayWidgetController::BroadcastInitialValues()
{
	BroadcastProgression();
}

void USWProgressionOverlayWidgetController::BindCallbacksToDependencies()
{
	UnbindCallbacks();
	if (!PlayerState)
	{
		return;
	}

	LevelChangedHandle = PlayerState->OnLevelChanged.AddUObject(this, &ThisClass::HandleLevelChanged);
	ExperienceChangedHandle = PlayerState->OnExperienceChanged.AddUObject(this, &ThisClass::HandleExperienceChanged);
	bCallbacksBound = true;
}

void USWProgressionOverlayWidgetController::BeginDestroy()
{
	UnbindCallbacks();
	Super::BeginDestroy();
}

void USWProgressionOverlayWidgetController::BroadcastProgression()
{
	if (!PlayerState)
	{
		return;
	}

	FSWOverlayProgressionSnapshot Snapshot;
	Snapshot.Level = PlayerState->GetPlayerLevel();
	Snapshot.TotalExperience = PlayerState->GetExperience();

	const USWProgressionData* ProgressionData = GameState ? GameState->GetProgressionData() : nullptr;
	if (!ProgressionData || !ProgressionData->HasValidLevelEntries())
	{
		OnProgressionChanged.Broadcast(Snapshot);
		return;
	}

	const int32 MaximumLevel = ProgressionData->GetMaximumLevel();
	Snapshot.bAtMaximumLevel = Snapshot.Level >= MaximumLevel;
	const int32 CurrentLevelExperience = ProgressionData->GetRequiredTotalExperienceForLevel(Snapshot.Level);
	Snapshot.ExperienceIntoCurrentLevel = FMath::Max(0, Snapshot.TotalExperience - CurrentLevelExperience);

	if (!Snapshot.bAtMaximumLevel)
	{
		const int32 NextLevelExperience = ProgressionData->GetRequiredTotalExperienceForLevel(Snapshot.Level + 1);
		Snapshot.ExperienceRequiredForNextLevel = FMath::Max(0, NextLevelExperience - CurrentLevelExperience);
		Snapshot.ExperiencePercent = Snapshot.ExperienceRequiredForNextLevel > 0
			? FMath::Clamp(static_cast<float>(Snapshot.ExperienceIntoCurrentLevel) / Snapshot.ExperienceRequiredForNextLevel, 0.f, 1.f)
			: 0.f;
	}
	else
	{
		Snapshot.ExperiencePercent = 1.f;
	}

	OnProgressionChanged.Broadcast(Snapshot);
}

void USWProgressionOverlayWidgetController::UnbindCallbacks()
{
	if (bCallbacksBound && PlayerState)
	{
		PlayerState->OnLevelChanged.Remove(LevelChangedHandle);
		PlayerState->OnExperienceChanged.Remove(ExperienceChangedHandle);
	}

	LevelChangedHandle.Reset();
	ExperienceChangedHandle.Reset();
	bCallbacksBound = false;
}

void USWProgressionOverlayWidgetController::HandleLevelChanged(const int32 NewLevel)
{
	(void)NewLevel;
	BroadcastProgression();
}

void USWProgressionOverlayWidgetController::HandleExperienceChanged(const int32 NewExperience)
{
	(void)NewExperience;
	BroadcastProgression();
}
