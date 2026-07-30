// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "UI/WidgetController/SWNetworkDiagnosticsWidgetController.h"

#include "Engine/NetConnection.h"
#include "Engine/NetDriver.h"
#include "Engine/World.h"
#include "GameFramework/PlayerState.h"
#include "Misc/App.h"
#include "Player/SWPlayerController.h"
#include "TimerManager.h"

void USWNetworkDiagnosticsWidgetController::BroadcastInitialValues()
{
	RefreshNetworkDiagnostics();
}

void USWNetworkDiagnosticsWidgetController::BindCallbacksToDependencies()
{
	if (bCallbacksBound || !PlayerController || !PlayerController->IsLocalController())
	{
		return;
	}

	bCallbacksBound = true;

	if (GameState)
	{
		GameState->OnServerNetworkSnapshotChanged.AddDynamic(this, &ThisClass::HandleServerNetworkSnapshotChanged);
	}

	if (UWorld* World = PlayerController->GetWorld())
	{
		World->GetTimerManager().SetTimer(
			RefreshTimer,
			this,
			&ThisClass::RefreshNetworkDiagnostics,
			RefreshIntervalSeconds,
			true);
	}
}

void USWNetworkDiagnosticsWidgetController::BeginDestroy()
{
	if (GameState)
	{
		GameState->OnServerNetworkSnapshotChanged.RemoveDynamic(this, &ThisClass::HandleServerNetworkSnapshotChanged);
	}

	if (PlayerController)
	{
		if (UWorld* World = PlayerController->GetWorld())
		{
			World->GetTimerManager().ClearTimer(RefreshTimer);
		}
	}

	Super::BeginDestroy();
}

void USWNetworkDiagnosticsWidgetController::RefreshNetworkDiagnostics()
{
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		return;
	}

	FSWNetworkDiagnosticsSnapshot NewSnapshot;
	NewSnapshot.Client.FrameTimeMilliseconds = FApp::GetDeltaTime() * 1000.0f;

	if (const APlayerState* LocalPlayerState = PlayerController->GetPlayerState<APlayerState>())
	{
		NewSnapshot.Client.PingMilliseconds = LocalPlayerState->GetPingInMilliseconds();
	}

	if (const UWorld* World = PlayerController->GetWorld())
	{
		if (const UNetDriver* NetDriver = World->GetNetDriver())
		{
			if (const UNetConnection* ServerConnection = NetDriver->ServerConnection)
			{
				NewSnapshot.Client.JitterMilliseconds = ServerConnection->GetAverageJitterInMS();
				NewSnapshot.Client.InKilobitsPerSecond = static_cast<float>(ServerConnection->InBytesPerSecond) * 8.0f / 1000.0f;
				NewSnapshot.Client.OutKilobitsPerSecond = static_cast<float>(ServerConnection->OutBytesPerSecond) * 8.0f / 1000.0f;
				NewSnapshot.Client.InPacketsPerSecond = ServerConnection->InPacketsPerSecond;
				NewSnapshot.Client.OutPacketsPerSecond = ServerConnection->OutPacketsPerSecond;
				NewSnapshot.Client.InPacketsLost = ServerConnection->InPacketsLost;
				NewSnapshot.Client.OutPacketsLost = ServerConnection->OutPacketsLost;
			}
		}
	}

	if (GameState)
	{
		NewSnapshot.Server = GameState->GetServerNetworkSnapshot();
	}

	CachedSnapshot = NewSnapshot;
	OnNetworkDiagnosticsChanged.Broadcast(CachedSnapshot);
}

void USWNetworkDiagnosticsWidgetController::HandleServerNetworkSnapshotChanged(const FSWServerNetworkSnapshot& NewSnapshot)
{
	CachedSnapshot.Server = NewSnapshot;
	OnNetworkDiagnosticsChanged.Broadcast(CachedSnapshot);
}
