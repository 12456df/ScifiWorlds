// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "GameState/SWGameState.h"

#include "Net/UnrealNetwork.h"
#include "Player/SWPlayerState.h"

ASWGameState::ASWGameState()
{
	// AGameState is replicated and always relevant by the engine. All custom
	// state below is written by the server and replicated to every client.
}

int32 ASWGameState::GetTeamPlayerCount(const ESWTeamId TeamId) const
{
	if (TeamId == ESWTeamId::None)
	{
		return 0;
	}

	int32 PlayerCount = 0;
	for (const APlayerState* PlayerState : PlayerArray)
	{
		const ASWPlayerState* SWPlayerState = Cast<ASWPlayerState>(PlayerState);
		if (SWPlayerState && !SWPlayerState->IsInactive() && SWPlayerState->GetTeamId() == TeamId)
		{
			++PlayerCount;
		}
	}

	return PlayerCount;
}

FSWTeamMatchStats ASWGameState::GetTeamMatchStats(const ESWTeamId TeamId) const
{
	switch (TeamId)
	{
	case ESWTeamId::TeamA:
		return TeamAStats;

	case ESWTeamId::TeamB:
		return TeamBStats;

	default:
		return FSWTeamMatchStats();
	}
}

float ASWGameState::GetWarmupSecondsRemaining() const
{
	if (WarmupEndServerTime <= 0.0)
	{
		return 0.0f;
	}

	return FMath::Max(0.0f, static_cast<float>(WarmupEndServerTime - GetServerWorldTimeSeconds()));
}

float ASWGameState::GetMatchElapsedSeconds() const
{
	if (MatchStartServerTime <= 0.0)
	{
		return 0.0f;
	}

	return FMath::Max(0.0f, static_cast<float>(GetServerWorldTimeSeconds() - MatchStartServerTime));
}

void ASWGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASWGameState, WarmupEndServerTime);
	DOREPLIFETIME(ASWGameState, MatchStartServerTime);
	DOREPLIFETIME(ASWGameState, WinningTeam);
	DOREPLIFETIME(ASWGameState, TeamAStats);
	DOREPLIFETIME(ASWGameState, TeamBStats);
}

void ASWGameState::SetWarmupEndServerTime(const double NewWarmupEndServerTime)
{
	check(HasAuthority());
	WarmupEndServerTime = FMath::Max(0.0, NewWarmupEndServerTime);
}

void ASWGameState::SetMatchStartServerTime(const double NewMatchStartServerTime)
{
	check(HasAuthority());
	MatchStartServerTime = FMath::Max(0.0, NewMatchStartServerTime);
}

void ASWGameState::SetWinningTeam(const ESWTeamId NewWinningTeam)
{
	check(HasAuthority());
	check(NewWinningTeam == ESWTeamId::None || NewWinningTeam == ESWTeamId::TeamA || NewWinningTeam == ESWTeamId::TeamB);
	WinningTeam = NewWinningTeam;
}

void ASWGameState::RecordTeamKill(const ESWTeamId TeamId)
{
	check(HasAuthority());

	if (FSWTeamMatchStats* TeamMatchStats = GetMutableTeamMatchStats(TeamId))
	{
		++TeamMatchStats->KillCount;
	}
}

void ASWGameState::RecordTowerDestroyed(const ESWTeamId TeamId)
{
	check(HasAuthority());

	if (FSWTeamMatchStats* TeamMatchStats = GetMutableTeamMatchStats(TeamId))
	{
		++TeamMatchStats->TowerDestroyCount;
	}
}

FSWTeamMatchStats* ASWGameState::GetMutableTeamMatchStats(const ESWTeamId TeamId)
{
	switch (TeamId)
	{
	case ESWTeamId::TeamA:
		return &TeamAStats;

	case ESWTeamId::TeamB:
		return &TeamBStats;

	default:
		return nullptr;
	}
}
