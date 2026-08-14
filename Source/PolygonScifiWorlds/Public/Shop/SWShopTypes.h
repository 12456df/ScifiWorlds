// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "SWShopTypes.generated.h"

/** 交易请求被服务器拒绝的原因；仅用于所属客户端提示，状态仍以复制结果为准。 */
UENUM(BlueprintType)
enum class ESWShopTransactionFailure : uint8
{
	None,
	NotInShopZone,
	Dead,
	InvalidItem,
	InsufficientGold,
	InventoryFull,
	OwnershipLimitReached,
	EffectApplicationFailed,
	InvalidSlot
};
