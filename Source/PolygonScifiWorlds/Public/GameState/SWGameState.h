// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "Team/SWTeamTypes.h"
#include "SWGameState.generated.h"

class ASWGameMode;

/** Public, replicated match statistics for one team. */
USTRUCT(BlueprintType)
struct FSWTeamMatchStats
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Match")
	int32 KillCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Match")
	int32 TowerDestroyCount = 0;
};

/**
 * Replicated public state of one match.
 *
 * ASWGameMode is the server-side referee and is the sole writer. Clients read
 * this class for timers, team statistics, and match result information.
 */
UCLASS()
class POLYGONSCIFIWORLDS_API ASWGameState : public AGameState
{
	GENERATED_BODY()

public:
	ASWGameState();

	/** Counts active, assigned players using the engine-owned PlayerArray. */
	UFUNCTION(BlueprintPure, Category = "Team")
	int32 GetTeamPlayerCount(ESWTeamId TeamId) const;

	/** Returns a copy of one team's public match statistics. */
	UFUNCTION(BlueprintPure, Category = "Match")
	FSWTeamMatchStats GetTeamMatchStats(ESWTeamId TeamId) const;

	/** Remaining warmup seconds, calculated from the synchronized server clock. */
	UFUNCTION(BlueprintPure, Category = "Match|Time")
	float GetWarmupSecondsRemaining() const;

	/** Elapsed match seconds, calculated from the synchronized server clock. */
	UFUNCTION(BlueprintPure, Category = "Match|Time")
	float GetMatchElapsedSeconds() const;

	UFUNCTION(BlueprintPure, Category = "Match")
	ESWTeamId GetWinningTeam() const { return WinningTeam; }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	/** ASWGameMode is the only class allowed to update authoritative match state. */
	friend class ASWGameMode;

	void SetWarmupEndServerTime(double NewWarmupEndServerTime);
	void SetMatchStartServerTime(double NewMatchStartServerTime);
	void SetWinningTeam(ESWTeamId NewWinningTeam);
	void RecordTeamKill(ESWTeamId TeamId);
	void RecordTowerDestroyed(ESWTeamId TeamId);

	FSWTeamMatchStats* GetMutableTeamMatchStats(ESWTeamId TeamId);

	/** Zero means that no warmup window is currently active. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match|Time", meta = (AllowPrivateAccess = "true"))
	double WarmupEndServerTime = 0.0;

	/** Zero means that the match has not formally started. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match|Time", meta = (AllowPrivateAccess = "true"))
	double MatchStartServerTime = 0.0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match", meta = (AllowPrivateAccess = "true"))
	ESWTeamId WinningTeam = ESWTeamId::None;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match", meta = (AllowPrivateAccess = "true"))
	FSWTeamMatchStats TeamAStats;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match", meta = (AllowPrivateAccess = "true"))
	FSWTeamMatchStats TeamBStats;
};
