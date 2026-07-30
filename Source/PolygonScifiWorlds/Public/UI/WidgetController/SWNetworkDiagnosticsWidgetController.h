// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameState/SWGameState.h"
#include "UI/WidgetController/SWWidgetController.h"
#include "SWNetworkDiagnosticsWidgetController.generated.h"

/** 当前本机到服务器的网络摘要，仅在拥有该 UI 的本地客户端采样。 */
USTRUCT(BlueprintType)
struct FSWClientNetworkSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Network Diagnostics")
	float PingMilliseconds = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Network Diagnostics")
	float JitterMilliseconds = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Network Diagnostics")
	float InKilobitsPerSecond = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Network Diagnostics")
	float OutKilobitsPerSecond = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Network Diagnostics")
	int32 InPacketsPerSecond = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Network Diagnostics")
	int32 OutPacketsPerSecond = 0;

	/** 当前统计周期内连接检测到的入站丢包数量。 */
	UPROPERTY(BlueprintReadOnly, Category = "Network Diagnostics")
	int32 InPacketsLost = 0;

	/** 当前统计周期内连接检测到的出站丢包数量。 */
	UPROPERTY(BlueprintReadOnly, Category = "Network Diagnostics")
	int32 OutPacketsLost = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Network Diagnostics")
	float FrameTimeMilliseconds = 0.0f;
};

/** 网络诊断 UI 的完整只读快照。 */
USTRUCT(BlueprintType)
struct FSWNetworkDiagnosticsSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Network Diagnostics")
	FSWClientNetworkSnapshot Client;

	UPROPERTY(BlueprintReadOnly, Category = "Network Diagnostics")
	FSWServerNetworkSnapshot Server;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSWOnNetworkDiagnosticsChanged, const FSWNetworkDiagnosticsSnapshot&, Snapshot);

/**
 * 将本机连接统计与 GameState 复制的服务器总览汇总给网络诊断 Widget。
 * 不复制自身、不发送 RPC，且只在本地 HUD 存在时低频刷新。
 */
UCLASS(BlueprintType, Blueprintable)
class POLYGONSCIFIWORLDS_API USWNetworkDiagnosticsWidgetController : public USWWidgetController
{
	GENERATED_BODY()

public:
	virtual void BroadcastInitialValues() override;
	virtual void BindCallbacksToDependencies() override;
	virtual void BeginDestroy() override;

	/** 读取最后一次采样结果，供蓝图在首次绑定时拉取。 */
	UFUNCTION(BlueprintPure, Category = "Network Diagnostics")
	FSWNetworkDiagnosticsSnapshot GetNetworkDiagnosticsSnapshot() const { return CachedSnapshot; }

	/** 快照刷新或服务器摘要复制到达时广播；WBP 仅订阅该事件更新文本。 */
	UPROPERTY(BlueprintAssignable, Category = "Network Diagnostics")
	FSWOnNetworkDiagnosticsChanged OnNetworkDiagnosticsChanged;

private:
	void RefreshNetworkDiagnostics();

	UFUNCTION()
	void HandleServerNetworkSnapshotChanged(const FSWServerNetworkSnapshot& NewSnapshot);

	UPROPERTY(BlueprintReadOnly, Category = "Network Diagnostics", meta = (AllowPrivateAccess = "true"))
	FSWNetworkDiagnosticsSnapshot CachedSnapshot;

	FTimerHandle RefreshTimer;
	bool bCallbacksBound = false;

	static constexpr float RefreshIntervalSeconds = 0.25f;
};
