// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Team/SWTeamTypes.h"
#include "UObject/Interface.h"
#include "SWTeamInterface.generated.h"

/** 提供唯一队伍真值的只读查询接口。 */
UINTERFACE(BlueprintType)
class POLYGONSCIFIWORLDS_API USWTeamInterface : public UInterface
{
	GENERATED_BODY()
};

class POLYGONSCIFIWORLDS_API ISWTeamInterface
{
	GENERATED_BODY()

public:
	/** None 表示中立、未分队或暂未实现队伍真值的单位。 */
	virtual ESWTeamId GetTeamId() const = 0;
};
