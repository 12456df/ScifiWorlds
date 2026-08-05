// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "Player/SWPlayerState.h"

#include "AbilitySystem/SWAbilitySystemComponent.h"
#include "AbilitySystem/SWAttributeSet.h"
#include "AbilitySystem/Data/SWProgressionData.h"
#include "Engine/World.h"
#include "GameState/SWGameState.h"
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
		// 即使 TeamId 未变化，也修复可能因重生或晚初始化缺失的 ASC 派生 Tag。
		AbilitySystemComponent->SetTeamIdTagAuthority(TeamId);
		return;
	}

	const ESWTeamId PreviousTeamId = TeamId;
	TeamId = NewTeamId;
	AbilitySystemComponent->SetTeamIdTagAuthority(TeamId);
	OnTeamIdChanged.Broadcast(PreviousTeamId, TeamId);
}

bool ASWPlayerState::IsValidTeamId(const ESWTeamId TeamIdToValidate) const
{
	return TeamIdToValidate == ESWTeamId::None
		|| TeamIdToValidate == ESWTeamId::TeamA
		|| TeamIdToValidate == ESWTeamId::TeamB;
}

void ASWPlayerState::AddExperienceAuthority(const int32 DeltaExperience)
{
	if (!HasAuthority() || DeltaExperience <= 0)
	{
		return;
	}

	const USWProgressionData* ProgressionData = GetProgressionData();
	if (!ProgressionData || !ProgressionData->HasValidLevelEntries())
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerState %s 的 ProgressionData 缺失或无效；拒绝本次经验结算。"), *GetName());
		return;
	}

	const int64 AccumulatedExperience = static_cast<int64>(Experience) + DeltaExperience;
	Experience = static_cast<int32>(FMath::Min<int64>(AccumulatedExperience, MAX_int32));

	const int32 PreviousLevel = Level;
	const int32 NewLevel = FMath::Max(PreviousLevel, FindLevelForExperience(Experience));
	int32 AbilityPointsToGrant = 0;
	for (int32 AwardedLevel = PreviousLevel + 1; AwardedLevel <= NewLevel; ++AwardedLevel)
	{
		const int64 AccumulatedAbilityPoints = static_cast<int64>(AbilityPointsToGrant) + ProgressionData->GetAbilityPointRewardForLevel(AwardedLevel);
		AbilityPointsToGrant = static_cast<int32>(FMath::Min<int64>(AccumulatedAbilityPoints, MAX_int32));
	}

	// 先写入完整最终状态，再广播委托，确保 HUD 与 Avatar 观察到的是同一份快照。
	Level = NewLevel;
	AbilityPoints = static_cast<int32>(FMath::Min<int64>(static_cast<int64>(AbilityPoints) + AbilityPointsToGrant, MAX_int32));
	OnExperienceChanged.Broadcast(Experience);

	if (Level != PreviousLevel)
	{
		OnLevelChanged.Broadcast(Level);
	}

	if (AbilityPointsToGrant > 0)
	{
		OnAbilityPointsChanged.Broadcast(AbilityPoints);
	}
}

void ASWPlayerState::SetLevel(int32 NewLevel)
{
	if (!HasAuthority())
	{
		return;
	}

	const USWProgressionData* ProgressionData = GetProgressionData();
	const int32 MaximumLevel = ProgressionData ? ProgressionData->GetMaximumLevel() : MAX_int32;
	Level = FMath::Clamp(NewLevel, 1, MaximumLevel);
	OnLevelChanged.Broadcast(Level);
}

const USWProgressionData* ASWPlayerState::GetProgressionData() const
{
	const UWorld* World = GetWorld();
	const ASWGameState* GameState = World ? World->GetGameState<ASWGameState>() : nullptr;
	return GameState ? GameState->GetProgressionData() : nullptr;
}

int32 ASWPlayerState::FindLevelForExperience(const int32 TotalExperience) const
{
	const USWProgressionData* ProgressionData = GetProgressionData();
	return ProgressionData && ProgressionData->HasValidLevelEntries()
		? ProgressionData->FindLevelForExperience(TotalExperience)
		: 1;
}

void ASWPlayerState::GrantAbilityPoints(int32 DeltaPoints)
{
	if (!HasAuthority() || DeltaPoints <= 0)
	{
		return;
	}

	AbilityPoints = static_cast<int32>(FMath::Min<int64>(static_cast<int64>(AbilityPoints) + DeltaPoints, MAX_int32));
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
