// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "Team/SWTeamTypes.h"
#include "SWGameState.generated.h"

class ASWGameMode;
class USWEconomyData;
class USWProgressionData;
class USWShopCatalogData;

/**
 * Dedicated Server 采集并复制给所有客户端的网络摘要。
 * 数值为诊断用途，不参与任何玩法判定。
 */
USTRUCT(BlueprintType)
struct FSWServerNetworkSnapshot
{
	GENERATED_BODY()

	/** 当前服务器 NetDriver 上的已连接客户端数量。 */
	UPROPERTY(BlueprintReadOnly, Category = "Network Diagnostics")
	int32 ConnectedClientCount = 0;

	/** 服务器总入站带宽，单位为 kbit/s（十进制）。 */
	UPROPERTY(BlueprintReadOnly, Category = "Network Diagnostics")
	float InKilobitsPerSecond = 0.0f;

	/** 服务器总出站带宽，单位为 kbit/s（十进制）。 */
	UPROPERTY(BlueprintReadOnly, Category = "Network Diagnostics")
	float OutKilobitsPerSecond = 0.0f;

	/** 服务器每秒收到的数据包数量。 */
	UPROPERTY(BlueprintReadOnly, Category = "Network Diagnostics")
	int32 InPacketsPerSecond = 0;

	/** 服务器每秒发出的数据包数量。 */
	UPROPERTY(BlueprintReadOnly, Category = "Network Diagnostics")
	int32 OutPacketsPerSecond = 0;

	/** 服务器检测到的入站丢包百分比。 */
	UPROPERTY(BlueprintReadOnly, Category = "Network Diagnostics")
	float InPacketLossPercent = 0.0f;

	/** 服务器检测到的出站丢包百分比。 */
	UPROPERTY(BlueprintReadOnly, Category = "Network Diagnostics")
	float OutPacketLossPercent = 0.0f;

	/** 服务器当前帧耗时，单位为毫秒。 */
	UPROPERTY(BlueprintReadOnly, Category = "Network Diagnostics")
	float FrameTimeMilliseconds = 0.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSWOnServerNetworkSnapshotChanged, const FSWServerNetworkSnapshot&, Snapshot);

/** Public, replicated match statistics for one team. */
USTRUCT(BlueprintType)
struct FSWTeamMatchStats
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Match")
	int32 KillCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Match")
	int32 TowerDestroyCount = 0;
};

/**
 * Replicated public state of one match.
 *
 * ASWGameMode is the server-side referee and is the sole writer. Clients read
 * this class for timers, team statistics, and match result information.
 */
UCLASS()
class POLYGONSCIFIWORLDS_API ASWGameState : public AGameState
{
	GENERATED_BODY()

public:
	ASWGameState();

	/** Counts active, assigned players using the engine-owned PlayerArray. */
	UFUNCTION(BlueprintPure, Category = "Team")
	int32 GetTeamPlayerCount(ESWTeamId TeamId) const;

	/** Returns a copy of one team's public match statistics. */
	UFUNCTION(BlueprintPure, Category = "Match")
	FSWTeamMatchStats GetTeamMatchStats(ESWTeamId TeamId) const;

	/** Remaining warmup seconds, calculated from the synchronized server clock. */
	UFUNCTION(BlueprintPure, Category = "Match|Time")
	float GetWarmupSecondsRemaining() const;

	/** Elapsed match seconds, calculated from the synchronized server clock. */
	UFUNCTION(BlueprintPure, Category = "Match|Time")
	float GetMatchElapsedSeconds() const;

	UFUNCTION(BlueprintPure, Category = "Match")
	ESWTeamId GetWinningTeam() const { return WinningTeam; }

	/** 返回本局使用的全局成长配置；GameMode 只在服务器设定，GameState 将其复制给客户端 UI。 */
	UFUNCTION(BlueprintPure, Category = "Progression")
	const USWProgressionData* GetProgressionData() const { return ProgressionData; }

	/** 返回本局经济配置；服务器写入、客户端只读，用于 UI 与诊断展示。 */
	UFUNCTION(BlueprintPure, Category = "Economy")
	const USWEconomyData* GetEconomyData() const { return EconomyData; }

	/** 本局固定商店目录；服务器配置，所有客户端只读，用于本地浏览与生成 UI。 */
	UFUNCTION(BlueprintPure, Category = "Shop")
	const USWShopCatalogData* GetShopCatalogData() const { return ShopCatalogData; }

	/** 返回最近一次由服务器采样并复制的网络摘要。 */
	UFUNCTION(BlueprintPure, Category = "Network Diagnostics")
	FSWServerNetworkSnapshot GetServerNetworkSnapshot() const { return ServerNetworkSnapshot; }

	/** 网络摘要发生复制更新时通知本地 UI 数据控制器。 */
	UPROPERTY(BlueprintAssignable, Category = "Network Diagnostics")
	FSWOnServerNetworkSnapshotChanged OnServerNetworkSnapshotChanged;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	/** ASWGameMode is the only class allowed to update authoritative match state. */
	friend class ASWGameMode;

	void SetWarmupEndServerTime(double NewWarmupEndServerTime);
	void SetMatchStartServerTime(double NewMatchStartServerTime);
	void SetWinningTeam(ESWTeamId NewWinningTeam);
	void RecordTeamKill(ESWTeamId TeamId);
	void RecordTowerDestroyed(ESWTeamId TeamId);
	void SetProgressionDataAuthority(USWProgressionData* NewProgressionData);
	void SetEconomyDataAuthority(USWEconomyData* NewEconomyData);
	void SetShopCatalogDataAuthority(USWShopCatalogData* NewShopCatalogData);

	FSWTeamMatchStats* GetMutableTeamMatchStats(ESWTeamId TeamId);

	/** 仅服务器调用；以低频率采样 NetDriver，避免诊断 UI 产生额外逐帧同步。 */
	void UpdateServerNetworkSnapshot();

	UFUNCTION()
	void OnRep_ServerNetworkSnapshot();

	/** Zero means that no warmup window is currently active. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match|Time", meta = (AllowPrivateAccess = "true"))
	double WarmupEndServerTime = 0.0;

	/** Zero means that the match has not formally started. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match|Time", meta = (AllowPrivateAccess = "true"))
	double MatchStartServerTime = 0.0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match", meta = (AllowPrivateAccess = "true"))
	ESWTeamId WinningTeam = ESWTeamId::None;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match", meta = (AllowPrivateAccess = "true"))
	FSWTeamMatchStats TeamAStats;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match", meta = (AllowPrivateAccess = "true"))
	FSWTeamMatchStats TeamBStats;

	/** 本局全局成长配置的已复制只读入口；具体升级规则在 M05 后续步骤消费它。 */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Progression", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USWProgressionData> ProgressionData;

	/** 本局经济规则的只读复制入口；金币真值仍属于各自 PlayerState。 */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Economy", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USWEconomyData> EconomyData;

	/** 不含玩家私有状态的固定商品目录；客户端仅供浏览，服务器交易仍重新验证。 */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Shop", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USWShopCatalogData> ShopCatalogData;

	/** 服务器每秒更新一次并复制；所有字段均为只读诊断信息。 */
	UPROPERTY(ReplicatedUsing = OnRep_ServerNetworkSnapshot, BlueprintReadOnly, Category = "Network Diagnostics", meta = (AllowPrivateAccess = "true"))
	FSWServerNetworkSnapshot ServerNetworkSnapshot;

	/** 服务器网络摘要的低频采样计时器。 */
	FTimerHandle ServerNetworkSnapshotTimer;

	static constexpr float ServerNetworkSnapshotIntervalSeconds = 1.0f;
};
