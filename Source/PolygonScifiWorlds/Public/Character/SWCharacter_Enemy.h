// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/SWCharacter_Base.h"
#include "SWCharacter_Enemy.generated.h"

/**
 * AI 敌方角色基类（小兵、野怪等的共同父类）。
 *
 * 与玩家不同，AI 没有 PlayerState，ASC 与 AttributeSet 直接由 Character 自身持有，
 * 使用 Minimal 复制模式（AI 无需向拥有者客户端复制完整 GameplayEffect）。
 */
UCLASS()
class POLYGONSCIFIWORLDS_API ASWCharacter_Enemy : public ASWCharacter_Base
{
	GENERATED_BODY()

public:
	ASWCharacter_Enemy();

	virtual int32 GetCombatLevel_Implementation() const override;
	virtual ESWTeamId GetTeamId() const override;

	/**
	 * 仅服务器调用：为运行时生成的 AI 更新队伍，并同步 ASC 的派生 Team Tag。
	 * 蓝图只能配置初始值，不能在客户端改写队伍真值。
	 */
	void SetTeamIdAuthority(ESWTeamId NewTeamId);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
	virtual void InitAbilityActorInfo() override;

	/** AI 的战斗等级属于其自身；玩家等级仍由 PlayerState 唯一持有。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "1"))
	int32 CombatLevel = 1;

	/**
	 * 敌方单位的队伍真值。TeamA/TeamB 用于双方小兵；None 专门表示中立野怪。
	 * 具体 Enemy 蓝图只配置该初始值，服务器生成器可通过 SetTeamIdAuthority 覆盖。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Replicated, Category = "Team")
	ESWTeamId TeamId = ESWTeamId::None;
};
