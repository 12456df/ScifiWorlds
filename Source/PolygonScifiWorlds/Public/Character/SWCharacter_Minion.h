// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "AbilitySystem/Abilities/Minions/SWMinionAttackGameplayAbility.h"
#include "Character/SWCharacter_Enemy.h"
#include "MassEntityHandle.h"
#include "SWCharacter_Minion.generated.h"

class USWCombatantDefinition;

/** 小兵 Actor 攻击桥的权威结果；仅用于 Processor/日志诊断，不复制给客户端。 */
UENUM()
enum class ESWMinionAttackAttemptResult : uint8
{
	Accepted,
	NotAuthority,
	SourceDead,
	InvalidTarget,
	TargetDead,
	SameTeam,
	OutOfRange,
	AbilityUnavailable,
};

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
	TSubclassOf<USWMinionAttackGameplayAbility> AttackAbilityClass;
	float AttackRange = 0.f;
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

	/**
	 * 仅服务器：重新校验 Target 后，以 Gameplay Event 激活已授予的小兵攻击 Ability。
	 * 该函数不直接扣血；伤害必须由攻击 GA 在其权威时点调用统一 Damage GE 入口。
	 */
	ESWMinionAttackAttemptResult TryActivateMinionAttackAuthority(AActor* TargetActor);

	/** 仅服务器/攻击 GA 使用的纯校验；Range 与 Team/Death 均在此处统一复核。 */
	ESWMinionAttackAttemptResult ValidateMinionAttackTargetAuthority(const AActor* TargetActor) const;

	float GetMinionAttackRange() const { return AttackRange; }

	/** 小兵攻击 GA 在命中时点读取的攻击等级；不依赖 PlayerState。 */
	int32 GetMinionCombatLevel() const { return CombatLevel; }

	/**
	 * 仅 Mass Actor Sync Processor 在服务器调用。CharacterMovement 不负责驱动位置，
	 * 但它保存当前权威表现速度，供 Actor Movement Replication 与客户端 AnimBP 使用。
	 */
	void SetMassVisualVelocityAuthority(const FVector& InVelocity);

	/** 仅服务器诊断：客户端不持有 Mass Entity。 */
	const FMassEntityHandle& GetMassEntityHandleAuthority() const { return MassEntityHandle; }

	virtual void BeginPlay() override;

	/**
	 * 小兵复用基类的死亡、奖励与表现链后，将结果桥接为 Mass Dead Tag。
	 * 仅服务器调用；Mass 生命周期仍由死亡 Observer 与 Cleanup Processor 接管。
	 */
	virtual bool TryCommitDeathAuthority(const FSWDeathContext& DeathContext) override;

private:
	/** 仅服务器：在 Actor 死亡已提交后标记对应 Entity，绝不反向决定 Actor 是否死亡。 */
	void MarkMassEntityDeadAuthority();

	FName MinionUnitId = NAME_None;
	int32 MinionWaveIndex = INDEX_NONE;
	FMassEntityHandle MassEntityHandle;
	TSubclassOf<USWMinionAttackGameplayAbility> MinionAttackAbilityClass;
	FGameplayAbilitySpecHandle MinionAttackAbilityHandle;
	float AttackRange = 0.f;

	bool bHasDeferredInitialization = false;
};
