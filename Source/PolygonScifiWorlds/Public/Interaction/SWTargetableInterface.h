// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SWTargetableInterface.generated.h"

class AActor;

/**
 * 可选的服务器锁定门槛。
 * 未实现该接口的现有角色和小兵维持既有 Target Registry 规则；实现者可附加易伤、隐身或阶段等专属限制。
 */
UINTERFACE(NotBlueprintable)
class POLYGONSCIFIWORLDS_API USWTargetableInterface : public UInterface
{
	GENERATED_BODY()
};

class POLYGONSCIFIWORLDS_API ISWTargetableInterface
{
	GENERATED_BODY()

public:
	/** 仅服务器目标选择调用；只读判断，不得在此修改自身或请求者的运行时状态。 */
	virtual bool IsTargetableBy(const AActor* Requestor) const = 0;
};
