#pragma once

#include "CoreMinimal.h"
#include "MassEntityTypes.h"
#include "SWMassSmokeTestFragments.generated.h"

/**
 * M10 Mass 技术冒烟 Entity 的最小运行时数据。
 * 仅用于验证批量创建后的独立 Fragment 写入，不属于正式小兵状态。
 */
USTRUCT()
struct FSWMassSmokeTestFragment : public FMassFragment
{
	GENERATED_BODY()

	/** 本批次中的创建顺序，从 0 开始。 */
	int32 SpawnOrdinal = INDEX_NONE;
};

/** 标记 M10 Mass 技术冒烟 Entity；不承载任何运行时数据。 */
USTRUCT()
struct FSWMassSmokeTestTag : public FMassTag
{
	GENERATED_BODY()
};
