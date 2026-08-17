// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "Team/SWTeamTypes.h"
#include "TimerManager.h"
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
	virtual void RestartPlayer(AController* NewPlayer) override;

	/** 仅在比赛进行中记录服务器确认的队伍击杀。 */
	void ReportTeamKill(ESWTeamId TeamId);

	/** 仅在比赛进行中记录服务器确认的队伍推塔。 */
	void ReportTowerDestroyed(ESWTeamId TeamId);

	/** 服务器确认水晶摧毁后结算比赛；已产生胜方时忽略重复报告。 */
	void ReportCrystalDestroyed(ESWTeamId DestroyedTeamId);

protected:
	/** 在 GameState 已存在后发布本局的全局成长配置。 */
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

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

	/** 仅服务器调用：消费玩家 Pawn 的死亡通知，并为该 Controller 安排唯一的重生计时器。 */
	void HandlePlayerDeathAuthority(const struct FSWDeathContext& DeathContext);

	/** 仅服务器调用：计时结束后销毁旧 Pawn、重建新 Pawn 并恢复 ASC 的可用状态。 */
	void RespawnPlayerAuthority(TWeakObjectPtr<class APlayerController> PlayerController);

	/** 为新生成的玩家 Pawn 绑定死亡通知；重复绑定会先移除旧绑定。 */
	void BindPlayerDeathDelegate(class ASWCharacter_Player* PlayerCharacter);

	/** 准备期和正式比赛均允许玩家重生；结束阶段不再生成新 Pawn。 */
	bool IsPlayerRespawnAllowed() const;

	/** 仅当准备期到期且人数满足规则时允许引擎开始比赛。 */
	virtual bool ReadyToStartMatch_Implementation() override;

	/** 比赛进入 InProgress 后记录全体客户端可读取的开局时间。 */
	virtual void HandleMatchHasStarted() override;

	/** 比赛结束后停止被动金币 Timer，避免结束状态下保留无效回调。 */
	virtual void HandleMatchHasEnded() override;

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

	/** 仅服务器调用：开始每秒一次的被动金币结算。 */
	void StartPassiveGoldIncomeAuthority();

	/** 仅服务器调用：按当前等级向全部有效队伍玩家结算一次被动金币。 */
	void GrantPassiveGoldIncomeAuthority();

	/** 准备期长度；首名有效玩家加入后开始计时。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "比赛规则", meta = (ClampMin = "0.0"))
	float WarmupDurationSeconds = 120.0f;

	/** 单支队伍可容纳的最大对局玩家数。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "比赛规则", meta = (ClampMin = "1"))
	int32 MaxPlayersPerTeam = 5;

	/** 准备期结束后允许正式开局的最低玩家数。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "比赛规则", meta = (ClampMin = "1"))
	int32 MinimumPlayersToStart = 1;

	/** 本局使用的全局成长数据；由服务器写入 GameState，供权威升级和客户端 UI 共用。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Progression")
	TObjectPtr<class USWProgressionData> ProgressionData;

	/** 本局初始金币、每秒收入曲线与金币上限；由 GameState 复制为只读配置。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Economy")
	TObjectPtr<class USWEconomyData> EconomyData;

	/** 本局固定商品目录；仅由服务器发布给 GameState，客户端只读浏览。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shop")
	TObjectPtr<class USWShopCatalogData> ShopCatalogData;

	/** 本局三路兵线的静态波次配方；仅服务器的 LaneWaveSubsystem 消费，不复制给客户端。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Minion Waves")
	TObjectPtr<class USWMinionWaveData> MinionWaveData;

	/** 仅服务器维护；每名玩家至多保留一个重生计时器，不复制给客户端。 */
	TMap<TWeakObjectPtr<class APlayerController>, FTimerHandle> PlayerRespawnTimers;

	/** 服务器唯一被动金币计时器；每秒结算一次，不使用 Tick。 */
	FTimerHandle PassiveGoldIncomeTimer;
};
