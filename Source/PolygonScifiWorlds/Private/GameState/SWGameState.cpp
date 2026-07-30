// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "GameState/SWGameState.h"

#include "Engine/NetDriver.h"
#include "Engine/World.h"
#include "Misc/App.h"
#include "Net/UnrealNetwork.h"
#include "Player/SWPlayerState.h"
#include "TimerManager.h"

ASWGameState::ASWGameState()
{
	// AGameState is replicated and always relevant by the engine. All custom
	// state below is written by the server and replicated to every client.
}

void ASWGameState::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		UpdateServerNetworkSnapshot();
		GetWorldTimerManager().SetTimer(
			ServerNetworkSnapshotTimer,
			this,
			&ThisClass::UpdateServerNetworkSnapshot,
			ServerNetworkSnapshotIntervalSeconds,
			true);
	}
}

void ASWGameState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(ServerNetworkSnapshotTimer);

	Super::EndPlay(EndPlayReason);
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
	DOREPLIFETIME(ASWGameState, ServerNetworkSnapshot);
}

void ASWGameState::UpdateServerNetworkSnapshot()
{
	check(HasAuthority());

	FSWServerNetworkSnapshot NewSnapshot;
	NewSnapshot.FrameTimeMilliseconds = FApp::GetDeltaTime() * 1000.0f;

	if (const UNetDriver* NetDriver = GetWorld()->GetNetDriver())
	{
		NewSnapshot.ConnectedClientCount = NetDriver->ClientConnections.Num();
		NewSnapshot.InKilobitsPerSecond = static_cast<float>(NetDriver->InBytesPerSecond) * 8.0f / 1000.0f;
		NewSnapshot.OutKilobitsPerSecond = static_cast<float>(NetDriver->OutBytesPerSecond) * 8.0f / 1000.0f;
		NewSnapshot.InPacketsPerSecond = static_cast<int32>(NetDriver->InPackets);
		NewSnapshot.OutPacketsPerSecond = static_cast<int32>(NetDriver->OutPackets);
		NewSnapshot.InPacketLossPercent = static_cast<float>(NetDriver->InPacketsLost);
		NewSnapshot.OutPacketLossPercent = static_cast<float>(NetDriver->OutPacketsLost);
	}

	ServerNetworkSnapshot = NewSnapshot;
	OnServerNetworkSnapshotChanged.Broadcast(ServerNetworkSnapshot);
}

void ASWGameState::OnRep_ServerNetworkSnapshot()
{
	OnServerNetworkSnapshotChanged.Broadcast(ServerNetworkSnapshot);
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
