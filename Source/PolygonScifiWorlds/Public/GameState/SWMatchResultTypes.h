// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Team/SWTeamTypes.h"
#include "SWMatchResultTypes.generated.h"

/** 本局尚未裁决、某队获胜或同帧双水晶摧毁后的平局。 */
UENUM(BlueprintType)
enum class ESWMatchOutcome : uint8
{
	Undecided UMETA(DisplayName = "未裁决"),
	TeamAWin UMETA(DisplayName = "Team A 胜利"),
	TeamBWin UMETA(DisplayName = "Team B 胜利"),
	Draw UMETA(DisplayName = "平局")
};

/** 记录结束结果的可信玩法原因；M13 首版只支持水晶摧毁。 */
UENUM(BlueprintType)
enum class ESWMatchEndReason : uint8
{
	None UMETA(DisplayName = "无"),
	CrystalDestroyed UMETA(DisplayName = "水晶被摧毁")
};

/**
 * 由服务器一次性写入并复制给所有端的比赛结果快照。
 * Outcome 是是否已结算的唯一真值；Draw 与 Undecided 都使用 WinningTeam=None。
 */
USTRUCT(BlueprintType)
struct POLYGONSCIFIWORLDS_API FSWMatchResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Match")
	ESWMatchOutcome Outcome = ESWMatchOutcome::Undecided;

	UPROPERTY(BlueprintReadOnly, Category = "Match")
	ESWMatchEndReason EndReason = ESWMatchEndReason::None;

	UPROPERTY(BlueprintReadOnly, Category = "Match")
	ESWTeamId WinningTeam = ESWTeamId::None;

	/** 服务器同步时钟上的裁决时刻；未裁决时为 0。 */
	UPROPERTY(BlueprintReadOnly, Category = "Match")
	double ResolvedServerTime = 0.0;

	bool IsResolved() const { return Outcome != ESWMatchOutcome::Undecided; }
};
