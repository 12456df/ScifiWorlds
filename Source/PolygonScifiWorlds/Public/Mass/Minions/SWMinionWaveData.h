// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SWMinionWaveData.generated.h"

class USWMinionDefinition;

/** 一种小兵在单条路线、单支队伍、单个波次中的组成与局部阵型。 */
USTRUCT(BlueprintType)
struct FSWMinionWaveCompositionEntry
{
	GENERATED_BODY()

	/** 本组生成的小兵类型。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave")
	TObjectPtr<USWMinionDefinition> MinionDefinition;

	/** 此组小兵的数量；必须与 FormationOffsets 数量一致，避免隐式重用位置。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave", meta = (ClampMin = "1"))
	int32 Count = 1;

	/**
	 * 相对于路线出生 Transform 的局部位置偏移。
	 * X 为路线前方、Y 为右侧、Z 为上方；TeamB 因出生 Transform 已反向，阵型会自然镜像。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave")
	TArray<FVector> FormationOffsets;
};

/**
 * 一局比赛共用的静态波次规则。
 * 它只描述节奏与组成，不保存当前 WaveIndex、Timer 或存活 Entity Handle。
 */
UCLASS(BlueprintType)
class POLYGONSCIFIWORLDS_API USWMinionWaveData : public UDataAsset
{
	GENERATED_BODY()

public:
	/** 比赛进入 InProgress 后至第一波生成前的等待时间。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wave|Timing", meta = (ClampMin = "0.0"))
	float InitialWaveDelaySeconds = 10.f;

	/** 第一波之后的固定波次间隔。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wave|Timing", meta = (ClampMin = "0.1"))
	float WaveIntervalSeconds = 30.f;

	/** 当前有效 Mass 小兵达到此上限时，服务器拒绝整波生成以保持双方和三路对称。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wave|Safety", meta = (ClampMin = "1"))
	int32 ActiveMinionHardCap = 120;

	/** 每条路线、每支队伍都使用同一份组成配置。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wave|Composition")
	TArray<FSWMinionWaveCompositionEntry> Composition;
};
