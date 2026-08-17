// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "Character/SWCharacter_Minion.h"

#include "AbilitySystem/Data/SWCombatantDefinition.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SWCharacter_Minion)

ASWCharacter_Minion::ASWCharacter_Minion()
{
	PrimaryActorTick.bCanEverTick = false;
	SetReplicates(true);
	SetReplicateMovement(true);
}

bool ASWCharacter_Minion::InitializeMinionAuthority(const FSWMinionActorInitializationData& InitializationData)
{
	if (!HasAuthority() || HasActorBegunPlay() || bHasDeferredInitialization)
	{
		ensureMsgf(false, TEXT("小兵 %s 必须在服务器 Deferred Spawn 与 FinishSpawning 之间仅初始化一次。"), *GetName());
		return false;
	}

	if (InitializationData.UnitId.IsNone()
		|| (InitializationData.TeamId != ESWTeamId::TeamA && InitializationData.TeamId != ESWTeamId::TeamB)
		|| !InitializationData.CombatantDefinition)
	{
		ensureMsgf(false, TEXT("小兵 %s 收到了无效的 Deferred Spawn 初始化数据。"), *GetName());
		return false;
	}

	MinionUnitId = InitializationData.UnitId;
	MinionWaveIndex = InitializationData.WaveIndex;
	TeamId = InitializationData.TeamId;
	CombatLevel = FMath::Max(1, InitializationData.CombatLevel);
	CombatantDefinition = InitializationData.CombatantDefinition;
	bHasDeferredInitialization = true;
	return true;
}

bool ASWCharacter_Minion::SetMassEntityHandleAuthority(const FMassEntityHandle& InMassEntityHandle)
{
	if (!HasAuthority() || !bHasDeferredInitialization || MassEntityHandle.IsValid() || !InMassEntityHandle.IsValid())
	{
		ensureMsgf(false, TEXT("小兵 %s 的 Mass Entity Handle 只能在服务器初始化完成后写入一次。"), *GetName());
		return false;
	}

	MassEntityHandle = InMassEntityHandle;
	return true;
}
