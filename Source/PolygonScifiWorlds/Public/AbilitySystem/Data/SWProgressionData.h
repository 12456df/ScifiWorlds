// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ScalableFloat.h"
#include "SWProgressionData.generated.h"

/** 一条按等级升序排列的玩家成长记录。 */
USTRUCT(BlueprintType)
struct FSWLevelProgressionEntry
{
	GENERATED_BODY()

	/** 达到本等级所需的累计经验。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Progression", meta = (ClampMin = "0"))
	int32 RequiredTotalExperience = 0;

	/** 从上一等级升至本等级时授予的技能点。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Progression", meta = (ClampMin = "0"))
	int32 AbilityPointReward = 0;
};

/** 玩家全局成长数据；具体数值由蓝图 Data Asset 配置。 */
UCLASS(BlueprintType)
class POLYGONSCIFIWORLDS_API USWProgressionData : public UDataAsset
{
	GENERATED_BODY()

public:
	/** 从等级 1 开始、按累计经验严格递增排列的记录。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Progression")
	TArray<FSWLevelProgressionEntry> LevelEntries;

	/** 以死亡时等级求值的服务器重生等待时间。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Progression", meta = (ClampMin = "0.0"))
	FScalableFloat RespawnDelayByLevel;

	/** 根据累计经验返回对应等级；无有效配置时回退为 1 级。 */
	int32 FindLevelForExperience(int32 TotalExperience) const;

	/** 返回进入指定等级时授予的技能点；无效等级或缺失配置返回 0。 */
	int32 GetAbilityPointRewardForLevel(int32 TargetLevel) const;

	/** 返回进入指定等级所需的累计经验；无效等级或配置时返回 0。 */
	int32 GetRequiredTotalExperienceForLevel(int32 TargetLevel) const;

	/** 返回当前配置允许的最大等级；无有效记录时为 1。 */
	int32 GetMaximumLevel() const;

	/** 检查等级记录是否从 1 级的 0 经验起点开始，且累计经验严格递增。 */
	bool HasValidLevelEntries() const;
};
