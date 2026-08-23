// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "SWStructureTypes.generated.h"

/** 固定防御结构的玩法身份；None 仅用于发现未完成的关卡实例配置。 */
UENUM(BlueprintType)
enum class ESWStructureKind : uint8
{
	None UMETA(DisplayName = "None"),
	Tower UMETA(DisplayName = "Tower"),
	Crystal UMETA(DisplayName = "Crystal"),
};
