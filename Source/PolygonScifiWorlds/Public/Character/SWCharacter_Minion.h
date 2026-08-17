// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/SWCharacter_Enemy.h"
#include "MassEntityHandle.h"
#include "SWCharacter_Minion.generated.h"

class USWCombatantDefinition;

/**
 * 服务器在 Deferred Spawn 与 FinishSpawning 之间传入的小兵 Actor 初始化数据。
 * 该结构是 Actor/ASC 的玩法真值输入，不包含 Mass Handle、路线距离或 StateTree 状态。
 */
USTRUCT()
struct FSWMinionActorInitializationData
{
	GENERATED_BODY()

	FName UnitId = NAME_None;
	int32 WaveIndex = INDEX_NONE;
	ESWTeamId TeamId = ESWTeamId::None;
	int32 CombatLevel = 1;
	USWCombatantDefinition* CombatantDefinition = nullptr;
};

/**
 * 兵线小兵的可复制 Actor/ASC 表现基类。
 * Mass 负责批量行为与路线数据；本 Actor 继续拥有碰撞、GAS、生命、死亡和网络表现真值。
 */
UCLASS(Abstract, Blueprintable, HideCategories = (Combat, Team))
class POLYGONSCIFIWORLDS_API ASWCharacter_Minion : public ASWCharacter_Enemy
{
	GENERATED_BODY()

public:
	ASWCharacter_Minion();

	/**
	 * 仅服务器在 Deferred Spawn 与 FinishSpawning 之间调用一次。
	 * 此时只写入后续 BeginPlay/ASC 初始化所需的静态输入，绝不创建或修改 Mass Entity。
	 */
	bool InitializeMinionAuthority(const FSWMinionActorInitializationData& InitializationData);

	/** 仅用于服务器诊断和未来 Actor/Entity 一致性校验。 */
	FName GetMinionUnitId() const { return MinionUnitId; }
	int32 GetMinionWaveIndex() const { return MinionWaveIndex; }

	/** 仅服务器：Factory 在 Actor 与 Entity 均已创建后写入，不参与网络复制。 */
	bool SetMassEntityHandleAuthority(const FMassEntityHandle& InMassEntityHandle);

	/** 仅服务器诊断：客户端不持有 Mass Entity。 */
	const FMassEntityHandle& GetMassEntityHandleAuthority() const { return MassEntityHandle; }

private:
	FName MinionUnitId = NAME_None;
	int32 MinionWaveIndex = INDEX_NONE;
	FMassEntityHandle MassEntityHandle;
	bool bHasDeferredInitialization = false;
};
