// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ScalableFloat.h"
#include "SWCombatantDefinition.generated.h"

class UGameplayEffect;

/** 每类可战斗单位的静态配置；运行时生命、等级与队伍状态不存放于此。 */
UCLASS(BlueprintType)
class POLYGONSCIFIWORLDS_API USWCombatantDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	/** 首次生成、升级和重生时覆盖等级相关基础属性的 GE。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combatant|Initialization")
	TSubclassOf<UGameplayEffect> LevelAttributesEffect;

	/** 首次生成和重生时回满当前资源的 GE。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combatant|Initialization")
	TSubclassOf<UGameplayEffect> VitalAttributesEffect;

	/** 以死亡时战斗等级求值的击杀经验。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combatant|Rewards")
	FScalableFloat XPRewardByLevel;

	/** 玩家重生后应用的短暂无敌 GE；非玩家单位可为空。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combatant|Respawn")
	TSubclassOf<UGameplayEffect> RespawnInvulnerabilityEffect;

	/** 非玩家尸体的建议保留时间；死亡表现最终由蓝图决定。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combatant|Death", meta = (ClampMin = "0.0"))
	float CorpseLifetimeSeconds = 0.f;
};
