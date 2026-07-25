// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "Team/SWTeamTypes.h"
#include "SWGameMode.generated.h"

/**
 * ScifiWorlds 的服务器权威比赛裁判。
 *
 * 客户端可见的计时、比分和胜负信息均由 ASWGameState 保存并复制，
 * 本类只负责规则判定与状态写入。
 */
UCLASS()
class POLYGONSCIFIWORLDS_API ASWGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	ASWGameMode();

	/** 仅在比赛进行中记录服务器确认的队伍击杀。 */
	void ReportTeamKill(ESWTeamId TeamId);

	/** 仅在比赛进行中记录服务器确认的队伍推塔。 */
	void ReportTowerDestroyed(ESWTeamId TeamId);

	/** 服务器确认水晶摧毁后结算比赛；已产生胜方时忽略重复报告。 */
	void ReportCrystalDestroyed(ESWTeamId DestroyedTeamId);

protected:
	/**
	 * 在玩家控制器与 PlayerState 已创建后、后续出生流程前验证选队参数。
	 * 连接地址需包含 ?Team=TeamA 或 ?Team=TeamB。
	 */
	virtual FString InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId,
		const FString& Options, const FString& Portal = TEXT("")) override;

	/** 有效玩家登录后按需开启准备期。 */
	virtual void PostLogin(APlayerController* NewPlayer) override;

	/** 准备期内也让已完成分队的玩家出生，避免 AGameMode 默认仅在 InProgress 阶段生成 Pawn。 */
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

	/** 玩家退出后检查是否应取消无人准备期。 */
	virtual void Logout(AController* Exiting) override;

	/** 仅当准备期到期且人数满足规则时允许引擎开始比赛。 */
	virtual bool ReadyToStartMatch_Implementation() override;

	/** 比赛进入 InProgress 后记录全体客户端可读取的开局时间。 */
	virtual void HandleMatchHasStarted() override;

	/** 仅在服务器 GameMode 中按玩家已分配的队伍标签选择出生点。 */
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	/** 从连接参数解析玩家请求的阵营。 */
	bool TryParseRequestedTeamId(const FString& Options, ESWTeamId& OutTeamId) const;

	/** 返回当前已完成选队的活动玩家总数。 */
	int32 GetActivePlayerCount() const;

	/** 在首名有效玩家进入时创建准备期截止时间。 */
	void StartWarmupIfNeeded();

	/** 当所有活动玩家离开时清除准备期截止时间。 */
	void CancelWarmupIfNoActivePlayers();

	/** 准备期长度；首名有效玩家加入后开始计时。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "比赛规则", meta = (ClampMin = "0.0"))
	float WarmupDurationSeconds = 120.0f;

	/** 单支队伍可容纳的最大对局玩家数。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "比赛规则", meta = (ClampMin = "1"))
	int32 MaxPlayersPerTeam = 5;

	/** 准备期结束后允许正式开局的最低玩家数。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "比赛规则", meta = (ClampMin = "1"))
	int32 MinimumPlayersToStart = 1;
};
