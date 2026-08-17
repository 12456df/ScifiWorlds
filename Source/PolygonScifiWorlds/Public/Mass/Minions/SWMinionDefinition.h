// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SWMinionDefinition.generated.h"

class ASWCharacter_Minion;
class UMassEntityConfigAsset;
class USWCombatantDefinition;

/**
 * 单一小兵原型的静态配方。
 * 波次系统只选择此定义与数量；运行时队伍、路线、波次和生命等状态不写回本资产。
 */
UCLASS(BlueprintType)
class POLYGONSCIFIWORLDS_API USWMinionDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	/** 跨波次稳定的小兵类型标识；Factory 写入 Mass Identity Fragment。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Minion", meta = (MustBeValid))
	FName UnitId = NAME_None;

	/** 组成该小兵 ECS Archetype 的正式 EntityConfig。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Minion|Mass")
	TObjectPtr<UMassEntityConfigAsset> EntityConfig;

	/** M10-5 Actor Bridge 生成的可复制战斗表现类，必须继承 ASWCharacter_Minion。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Minion|Actor")
	TSubclassOf<ASWCharacter_Minion> MinionActorClass;

	/** Actor/ASC 初始化时使用的静态战斗配置。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Minion|Combat")
	TObjectPtr<USWCombatantDefinition> CombatantDefinition;

	/** 小兵自身的战斗等级；不使用 PlayerState 等级。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Minion|Combat", meta = (ClampMin = "1"))
	int32 CombatLevel = 1;
};
