// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Interaction/SWDeathTypes.h"
#include "UObject/Interface.h"
#include "SWCombatInterface.generated.h"

/** 可参与战斗结算的 Actor 所提供的最小只读能力。 */
UINTERFACE(BlueprintType)
class POLYGONSCIFIWORLDS_API USWCombatInterface : public UInterface
{
	GENERATED_BODY()
};

class POLYGONSCIFIWORLDS_API ISWCombatInterface
{
	GENERATED_BODY()

public:
	/** 返回用于创建伤害 Spec 的当前战斗等级。 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat")
	int32 GetCombatLevel() const;

	/** 返回当前是否已死亡；死亡真值由实现者的服务器权威状态维护。 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat")
	bool IsDead() const;

	/**
	 * 仅服务器调用的死亡提交契约。
	 * 接口只声明能力；幂等状态、Tag、Ability 取消及表现通知由具体 Combatant 实现。
	 */
	virtual bool TryCommitDeathAuthority(const FSWDeathContext& DeathContext) = 0;

	/** 仅服务器使用的死亡提交通知入口。 */
	virtual FSWOnDeath& GetOnDeathDelegate() = 0;
};
