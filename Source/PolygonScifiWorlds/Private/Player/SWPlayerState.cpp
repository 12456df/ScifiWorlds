// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "Player/SWPlayerState.h"

#include "AbilitySystem/SWAbilitySystemComponent.h"
#include "AbilitySystem/SWAttributeSet.h"
#include "Net/UnrealNetwork.h"

ASWPlayerState::ASWPlayerState()
{
	// PlayerState replicates GAS state; raise the update rate above the default.
	SetNetUpdateFrequency(100.f);

	AbilitySystemComponent = CreateDefaultSubobject<USWAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	// Players use Mixed: GEs replicate to the owner, cues/tags to everyone.
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<USWAttributeSet>(TEXT("AttributeSet"));
}

void ASWPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(ASWPlayerState, Level, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ASWPlayerState, Experience, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ASWPlayerState, AbilityPoints, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME(ASWPlayerState, TeamId);
}

UAbilitySystemComponent* ASWPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ASWPlayerState::SetTeamId(const ESWTeamId NewTeamId)
{
	check(HasAuthority());

	if (!ensureMsgf(IsValidTeamId(NewTeamId), TEXT("请求了无效的队伍归属。")))
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

void ASWPlayerState::AddExperience(int32 DeltaExperience)
{
	if (!HasAuthority() || DeltaExperience <= 0)
	{
		return;
	}

	Experience = FMath::Max(0, Experience + DeltaExperience);
	OnExperienceChanged.Broadcast(Experience);

	// Data-driven level-up (threshold table, multi-level crossing, per-level ability
	// point rewards) is applied by the progression config in a later delivery step.
}

void ASWPlayerState::SetLevel(int32 NewLevel)
{
	if (!HasAuthority())
	{
		return;
	}

	Level = FMath::Max(1, NewLevel);
	OnLevelChanged.Broadcast(Level);
}

void ASWPlayerState::GrantAbilityPoints(int32 DeltaPoints)
{
	if (!HasAuthority() || DeltaPoints <= 0)
	{
		return;
	}

	AbilityPoints = FMath::Max(0, AbilityPoints + DeltaPoints);
	OnAbilityPointsChanged.Broadcast(AbilityPoints);
}

bool ASWPlayerState::SpendAbilityPoint()
{
	if (!HasAuthority() || AbilityPoints <= 0)
	{
		return false;
	}

	AbilityPoints -= 1;
	OnAbilityPointsChanged.Broadcast(AbilityPoints);
	return true;
}

void ASWPlayerState::OnRep_TeamId(const ESWTeamId PreviousTeamId)
{
	OnTeamIdChanged.Broadcast(PreviousTeamId, TeamId);
}

void ASWPlayerState::OnRep_Level(int32 OldLevel)
{
	OnLevelChanged.Broadcast(Level);
}

void ASWPlayerState::OnRep_Experience(int32 OldExperience)
{
	OnExperienceChanged.Broadcast(Experience);
}

void ASWPlayerState::OnRep_AbilityPoints(int32 OldAbilityPoints)
{
	OnAbilityPointsChanged.Broadcast(AbilityPoints);
}
