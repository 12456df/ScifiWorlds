// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Structures/SWStructureAttackGameplayAbility.h"
#include "Engine/DataAsset.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif
#include "SWStructureDefinition.generated.h"

class UBehaviorTree;
class USWCombatantDefinition;

/**
 * 塔与水晶共用的静态战斗配置。
 * 该资产不保存 Team、生命、易伤或当前目标等局内状态；它们唯一由关卡结构实例和服务器运行时系统持有。
 */
UCLASS(BlueprintType)
class POLYGONSCIFIWORLDS_API USWStructureDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	/** 复用既有战斗单位初始化、资源与奖励定义；结构首版应使用零奖励配置。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Structure|Initialization")
	TObjectPtr<USWCombatantDefinition> CombatantDefinition;

	/** 固定结构的初始化等级；结构不会在局内升级。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Structure|Initialization", meta = (ClampMin = "1"))
	int32 CombatLevel = 1;

	/** 同时用于索敌与结构受击来源距离校验的二维半径，单位为厘米。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Structure|Combat", meta = (ClampMin = "1.0"))
	float CombatRadius = 1000.f;

	/** 物理护甲、穿透与暴击结算后的静态减伤比例。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Structure|Combat", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DamageReductionPercent = 0.f;

	/** 结构攻击的统一 C++ Ability 基类；具体射速、弹丸和表现由蓝图子类配置。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Structure|Attack")
	TSubclassOf<USWStructureAttackGameplayAbility> AttackAbilityClass;

	/** 仅服务器 AI 使用的行为树；首版塔和水晶可共享同一资产。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Structure|AI")
	TObjectPtr<UBehaviorTree> BehaviorTree;

#if WITH_EDITOR
	/** 仅编辑器验证静态资产完整性；关卡 ID、队伍与前置关系由后续 Objective Subsystem 验证。 */
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
