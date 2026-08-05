// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "AbilitySystem/Data/SWProgressionData.h"

int32 USWProgressionData::FindLevelForExperience(const int32 TotalExperience) const
{
	if (!HasValidLevelEntries())
	{
		return 1;
	}

	const int32 SafeExperience = FMath::Max(0, TotalExperience);
	int32 ResolvedLevel = 1;

	for (int32 Index = 0; Index < LevelEntries.Num(); ++Index)
	{
		const FSWLevelProgressionEntry& Entry = LevelEntries[Index];
		if (SafeExperience < Entry.RequiredTotalExperience)
		{
			break;
		}

		ResolvedLevel = Index + 1;
	}

	return ResolvedLevel;
}

int32 USWProgressionData::GetAbilityPointRewardForLevel(const int32 TargetLevel) const
{
	const int32 EntryIndex = TargetLevel - 1;
	return LevelEntries.IsValidIndex(EntryIndex) ? FMath::Max(0, LevelEntries[EntryIndex].AbilityPointReward) : 0;
}

int32 USWProgressionData::GetRequiredTotalExperienceForLevel(const int32 TargetLevel) const
{
	const int32 EntryIndex = TargetLevel - 1;
	return LevelEntries.IsValidIndex(EntryIndex) ? FMath::Max(0, LevelEntries[EntryIndex].RequiredTotalExperience) : 0;
}

int32 USWProgressionData::GetMaximumLevel() const
{
	return HasValidLevelEntries() ? LevelEntries.Num() : 1;
}

bool USWProgressionData::HasValidLevelEntries() const
{
	if (LevelEntries.IsEmpty() || LevelEntries[0].RequiredTotalExperience != 0)
	{
		return false;
	}

	for (int32 Index = 1; Index < LevelEntries.Num(); ++Index)
	{
		if (LevelEntries[Index].RequiredTotalExperience <= LevelEntries[Index - 1].RequiredTotalExperience)
		{
			return false;
		}
	}

	return true;
}
