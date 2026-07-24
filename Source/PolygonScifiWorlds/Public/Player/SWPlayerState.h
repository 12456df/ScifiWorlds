// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Team/SWTeamTypes.h"
#include "SWPlayerState.generated.h"

class ASWGameMode;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSWOnTeamIdChanged, ESWTeamId, PreviousTeamId, ESWTeamId, NewTeamId);

/**
 * Replicated, per-player match data.
 *
 * Team membership lives here rather than on a pawn, so it remains available to
 * every client while the player is respawning or changing pawns.
 */
UCLASS()
class POLYGONSCIFIWORLDS_API ASWPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	ASWPlayerState();

	/** Returns this player's server-assigned team. Safe to read on every machine. */
	UFUNCTION(BlueprintPure, Category = "Team")
	ESWTeamId GetTeamId() const { return TeamId; }

	/** Broadcast on the server and on clients when TeamId changes. */
	UPROPERTY(BlueprintAssignable, Category = "Team")
	FSWOnTeamIdChanged OnTeamIdChanged;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	/** The match referee is the sole writer of a player's team assignment. */
	friend class ASWGameMode;

	void SetTeamId(ESWTeamId NewTeamId);
	bool IsValidTeamId(ESWTeamId TeamIdToValidate) const;

	UFUNCTION()
	void OnRep_TeamId(ESWTeamId PreviousTeamId);

	UPROPERTY(ReplicatedUsing = OnRep_TeamId, BlueprintReadOnly, Category = "Team", meta = (AllowPrivateAccess = "true"))
	ESWTeamId TeamId = ESWTeamId::None;
};
