// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "Interaction/SWPlayerProgressionInterface.h"
#include "Interaction/SWTeamInterface.h"
#include "Team/SWTeamTypes.h"
#include "SWPlayerState.generated.h"

class ASWGameMode;
class UAbilitySystemComponent;
class USWAbilitySystemComponent;
class USWAttributeSet;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSWOnTeamIdChanged, ESWTeamId, PreviousTeamId, ESWTeamId, NewTeamId);

// UI/gameplay listeners for replicated progression changes. Broadcast on both the
// authority (setter) and simulated proxies (OnRep) so observers stay in sync.
DECLARE_MULTICAST_DELEGATE_OneParam(FSWOnProgressionValueChanged, int32 /*NewValue*/);

/**
 * Server-authoritative owner of the player's ability system and persistent progression.
 *
 * Contract defined in Docs/Systems/M03_GASCoreFramework.md and ADR-0002. The ASC and
 * attribute set live here so they survive Pawn respawns; the current Pawn only acts as
 * the ASC avatar. The server is the sole writer of Level, Experience and AbilityPoints.
 */
UCLASS()
class POLYGONSCIFIWORLDS_API ASWPlayerState : public APlayerState, public IAbilitySystemInterface, public ISWPlayerProgressionInterface, public ISWTeamInterface
{
	GENERATED_BODY()

public:
	ASWPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//~ Begin IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	//~ End IAbilitySystemInterface

	USWAttributeSet* GetAttributeSet() const { return AttributeSet; }

	/** 返回服务器分配的队伍；所有端均可安全读取。 */
	UFUNCTION(BlueprintPure, Category = "Team")
	virtual ESWTeamId GetTeamId() const override { return TeamId; }

	/** 队伍变化时在服务器与客户端触发，供蓝图表现层订阅。 */
	UPROPERTY(BlueprintAssignable, Category = "Team")
	FSWOnTeamIdChanged OnTeamIdChanged;

	// --- Progression readers (any client may read) ---

	virtual int32 GetPlayerLevel() const override { return Level; }
	virtual int32 GetExperience() const override { return Experience; }
	virtual int32 GetAbilityPoints() const override { return AbilityPoints; }
	virtual int32 FindLevelForExperience(int32 TotalExperience) const override;

	// --- Server-authoritative progression writers ---
	// All mutators no-op off the authority; clients only request intent elsewhere.

	/** 仅服务器调用：增加非负经验，并根据本局 ProgressionData 结算跨级与技能点。 */
	virtual void AddExperienceAuthority(int32 DeltaExperience) override;

	/** 仅服务器调用：直接设置等级；有 ProgressionData 时同时限制到其最大等级。 */
	void SetLevel(int32 NewLevel);

	/** Grants ability points (non-negative delta). */
	void GrantAbilityPoints(int32 DeltaPoints);

	/** Consumes a single ability point if available. Returns true when a point was spent. */
	bool SpendAbilityPoint();

	// --- Change delegates ---

	FSWOnProgressionValueChanged OnLevelChanged;
	FSWOnProgressionValueChanged OnExperienceChanged;
	FSWOnProgressionValueChanged OnAbilityPointsChanged;

protected:
	/** 只有服务器 GameMode 能写入队伍归属。 */
	friend class ASWGameMode;

	void SetTeamId(ESWTeamId NewTeamId);
	bool IsValidTeamId(ESWTeamId TeamIdToValidate) const;

	/** 返回由已复制 GameState 暴露的本局成长配置；只在服务器写入路径中消费。 */
	const class USWProgressionData* GetProgressionData() const;

	UPROPERTY(VisibleAnywhere, Category = "SW|GAS")
	TObjectPtr<USWAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, Category = "SW|GAS")
	TObjectPtr<USWAttributeSet> AttributeSet;

	// --- Replicated progression state (server writes, all clients read) ---

	UPROPERTY(VisibleAnywhere, Category = "SW|Progression", ReplicatedUsing = OnRep_Level)
	int32 Level = 1;

	UPROPERTY(VisibleAnywhere, Category = "SW|Progression", ReplicatedUsing = OnRep_Experience)
	int32 Experience = 0;

	UPROPERTY(VisibleAnywhere, Category = "SW|Progression", ReplicatedUsing = OnRep_AbilityPoints)
	int32 AbilityPoints = 0;

	/** 队伍初始为 None，仅由 GameMode 在加入流程中写入。 */
	UPROPERTY(ReplicatedUsing = OnRep_TeamId, BlueprintReadOnly, Category = "Team", meta = (AllowPrivateAccess = "true"))
	ESWTeamId TeamId = ESWTeamId::None;

	UFUNCTION()
	void OnRep_TeamId(ESWTeamId PreviousTeamId);

	UFUNCTION()
	void OnRep_Level(int32 OldLevel);

	UFUNCTION()
	void OnRep_Experience(int32 OldExperience);

	UFUNCTION()
	void OnRep_AbilityPoints(int32 OldAbilityPoints);
};
