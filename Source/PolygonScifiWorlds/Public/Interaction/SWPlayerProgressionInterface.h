// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SWPlayerProgressionInterface.generated.h"

/** 玩家持久成长状态的最小 C++ 契约。 */
UINTERFACE()
class POLYGONSCIFIWORLDS_API USWPlayerProgressionInterface : public UInterface
{
	GENERATED_BODY()
};

class POLYGONSCIFIWORLDS_API ISWPlayerProgressionInterface
{
	GENERATED_BODY()

public:
	/** 返回当前已复制的玩家等级。 */
	virtual int32 GetPlayerLevel() const = 0;

	/** 返回当前已复制的累计经验。 */
	virtual int32 GetExperience() const = 0;

	/** 返回当前已复制的可用技能点。 */
	virtual int32 GetAbilityPoints() const = 0;

	/** 按本局成长数据查询累计经验对应的等级。 */
	virtual int32 FindLevelForExperience(int32 TotalExperience) const = 0;

	/** 仅服务器调用：提交正经验并结算跨级与技能点。 */
	virtual void AddExperienceAuthority(int32 DeltaExperience) = 0;
};
