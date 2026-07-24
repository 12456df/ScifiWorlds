// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "Player/SWPlayerState.h"

#include "Net/UnrealNetwork.h"

ASWPlayerState::ASWPlayerState()
{
	// APlayerState is already replicated and always relevant. TeamId starts as None
	// until ASWGameMode assigns it during the server-side join flow.
}

void ASWPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASWPlayerState, TeamId);
}

void ASWPlayerState::SetTeamId(const ESWTeamId NewTeamId)
{
	check(HasAuthority());

	if (!ensureMsgf(IsValidTeamId(NewTeamId), TEXT("Invalid team assignment requested.")))
	{
		return;
	}

	if (TeamId == NewTeamId)
	{
		return;
	}

	const ESWTeamId PreviousTeamId = TeamId;
	TeamId = NewTeamId;
	OnTeamIdChanged.Broadcast(PreviousTeamId, TeamId);
}

bool ASWPlayerState::IsValidTeamId(const ESWTeamId TeamIdToValidate) const
{
	return TeamIdToValidate == ESWTeamId::None
		|| TeamIdToValidate == ESWTeamId::TeamA
		|| TeamIdToValidate == ESWTeamId::TeamB;
}

void ASWPlayerState::OnRep_TeamId(const ESWTeamId PreviousTeamId)
{
	OnTeamIdChanged.Broadcast(PreviousTeamId, TeamId);
}
