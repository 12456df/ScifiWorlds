// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "GameMode/SWGameMode.h"

#include "Character/SWCharacter_Base.h"
#include "EngineUtils.h"
#include "GameState/SWGameState.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "Player/SWPlayerController.h"
#include "Player/SWPlayerState.h"

ASWGameMode::ASWGameMode()
{
	// 比赛由准备期结束后的服务器显式启动，而不是在首名玩家进入时立即开始。
	bDelayedStart = true;

	// 统一注册 M02 的 Gameplay Framework 类型。
	DefaultPawnClass = ASWCharacter_Base::StaticClass();
	GameStateClass = ASWGameState::StaticClass();
	PlayerControllerClass = ASWPlayerController::StaticClass();
	PlayerStateClass = ASWPlayerState::StaticClass();
}

void ASWGameMode::ReportTeamKill(const ESWTeamId TeamId)
{
	if (GetMatchState() != MatchState::InProgress)
	{
		return;
	}

	ASWGameState* SWGameState = GetGameState<ASWGameState>();
	if (!SWGameState || (TeamId != ESWTeamId::TeamA && TeamId != ESWTeamId::TeamB))
	{
		UE_LOG(LogTemp, Warning, TEXT("Ignoring a team kill report with invalid match state or team."));
		return;
	}

	SWGameState->RecordTeamKill(TeamId);
}

void ASWGameMode::ReportTowerDestroyed(const ESWTeamId TeamId)
{
	if (GetMatchState() != MatchState::InProgress)
	{
		return;
	}

	ASWGameState* SWGameState = GetGameState<ASWGameState>();
	if (!SWGameState || (TeamId != ESWTeamId::TeamA && TeamId != ESWTeamId::TeamB))
	{
		UE_LOG(LogTemp, Warning, TEXT("Ignoring a tower destruction report with invalid match state or team."));
		return;
	}

	SWGameState->RecordTowerDestroyed(TeamId);
}

void ASWGameMode::ReportCrystalDestroyed(const ESWTeamId DestroyedTeamId)
{
	if (GetMatchState() != MatchState::InProgress)
	{
		return;
	}

	ASWGameState* SWGameState = GetGameState<ASWGameState>();
	if (!SWGameState || (DestroyedTeamId != ESWTeamId::TeamA && DestroyedTeamId != ESWTeamId::TeamB))
	{
		UE_LOG(LogTemp, Warning, TEXT("Ignoring a crystal destruction report with invalid match state or team."));
		return;
	}

	if (SWGameState->GetWinningTeam() != ESWTeamId::None)
	{
		return;
	}

	const ESWTeamId WinningTeamId = DestroyedTeamId == ESWTeamId::TeamA ? ESWTeamId::TeamB : ESWTeamId::TeamA;
	SWGameState->SetWinningTeam(WinningTeamId);
	EndMatch();
}

FString ASWGameMode::InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId,

	const FString& Options, const FString& Portal)
{
	// M02 不接受比赛正式开始后的新对局玩家；观战与重连规则留待后续模块定义。
	if (GetMatchState() != MatchState::WaitingToStart)
	{
		return TEXT("比赛已开始，无法加入。");
	}

	ESWTeamId RequestedTeamId = ESWTeamId::None;
	if (!TryParseRequestedTeamId(Options, RequestedTeamId))
	{
		return TEXT("请选择有效阵营。");
	}

	ASWGameState* SWGameState = GetGameState<ASWGameState>();
	ASWPlayerState* SWPlayerState = NewPlayerController ? NewPlayerController->GetPlayerState<ASWPlayerState>() : nullptr;
	if (!SWGameState || !SWPlayerState)
	{
		return TEXT("无法加入该对局。");
	}

	if (SWGameState->GetTeamPlayerCount(RequestedTeamId) >= MaxPlayersPerTeam)
	{
		return TEXT("所选阵营已满。");
	}

	// 仅服务器的 GameMode 能写入 TeamId；之后引擎继续执行标准初始化流程。
	SWPlayerState->SetTeamId(RequestedTeamId);
	return Super::InitNewPlayer(NewPlayerController, UniqueId, Options, Portal);
}

bool ASWGameMode::TryParseRequestedTeamId(const FString& Options, ESWTeamId& OutTeamId) const
{
	const FString RequestedTeam = UGameplayStatics::ParseOption(Options, TEXT("Team"));

	if (RequestedTeam.Equals(TEXT("TeamA"), ESearchCase::IgnoreCase))
	{
		OutTeamId = ESWTeamId::TeamA;
		return true;
	}

	if (RequestedTeam.Equals(TEXT("TeamB"), ESearchCase::IgnoreCase))
	{
		OutTeamId = ESWTeamId::TeamB;
		return true;
	}

	OutTeamId = ESWTeamId::None;
	return false;
}

void ASWGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	// InitNewPlayer 已完成选队验证；首名有效玩家据此启动准备期。
	StartWarmupIfNeeded();
}

void ASWGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	// 父类已把退出玩家标为 Inactive，队伍计数会自动忽略该状态。
	CancelWarmupIfNoActivePlayers();
}

bool ASWGameMode::ReadyToStartMatch_Implementation()
{
	const ASWGameState* SWGameState = GetGameState<ASWGameState>();
	if (!SWGameState || SWGameState->WarmupEndServerTime <= 0.0)
	{
		return false;
	}

	const bool bWarmupExpired = SWGameState->GetServerWorldTimeSeconds() >= SWGameState->WarmupEndServerTime;
	const bool bHasEnoughPlayers = GetActivePlayerCount() >= MinimumPlayersToStart;
	return bWarmupExpired && bHasEnoughPlayers;
}

void ASWGameMode::HandleMatchHasStarted()
{
	ASWGameState* SWGameState = GetGameState<ASWGameState>();
	if (SWGameState)
	{
		// 开局后准备期不再有效，正式计时从同一份服务器同步时钟起算。
		SWGameState->SetWarmupEndServerTime(0.0);
		SWGameState->SetMatchStartServerTime(SWGameState->GetServerWorldTimeSeconds());
	}

	Super::HandleMatchHasStarted();
}

AActor* ASWGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	const ASWPlayerState* SWPlayerState = Player ? Player->GetPlayerState<ASWPlayerState>() : nullptr;
	if (!SWPlayerState)
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot choose a team PlayerStart because the controller has no SWPlayerState."));
		return nullptr;
	}

	const ESWTeamId TeamId = SWPlayerState->GetTeamId();
	const FName TeamStartTag = TeamId == ESWTeamId::TeamA ? FName(TEXT("TeamA"))
		: TeamId == ESWTeamId::TeamB ? FName(TEXT("TeamB")) : NAME_None;
	if (TeamStartTag.IsNone())
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot choose a team PlayerStart for a player without a valid team."));
		return nullptr;
	}

	UWorld* World = GetWorld();
	UClass* PawnClass = GetDefaultPawnClassForController(Player);
	APawn* PawnToFit = PawnClass ? PawnClass->GetDefaultObject<APawn>() : nullptr;
	if (!World || !PawnToFit)
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot choose a team PlayerStart because the world or default pawn class is unavailable."));
		return nullptr;
	}

	TArray<APlayerStart*> UnoccupiedStarts;
	TArray<APlayerStart*> AdjustableStarts;
	for (TActorIterator<APlayerStart> It(World); It; ++It)
	{
		APlayerStart* PlayerStart = *It;
		if (!PlayerStart || PlayerStart->PlayerStartTag != TeamStartTag)
		{
			continue;
		}

		FVector SpawnLocation = PlayerStart->GetActorLocation();
		const FRotator SpawnRotation = PlayerStart->GetActorRotation();
		if (!World->EncroachingBlockingGeometry(PawnToFit, SpawnLocation, SpawnRotation))
		{
			UnoccupiedStarts.Add(PlayerStart);
		}
		else if (World->FindTeleportSpot(PawnToFit, SpawnLocation, SpawnRotation))
		{
			AdjustableStarts.Add(PlayerStart);
		}
	}

	const TArray<APlayerStart*>& CandidateStarts = UnoccupiedStarts.Num() > 0 ? UnoccupiedStarts : AdjustableStarts;
	if (CandidateStarts.Num() > 0)
	{
		return CandidateStarts[FMath::RandRange(0, CandidateStarts.Num() - 1)];
	}

	UE_LOG(LogTemp, Error, TEXT("No usable PlayerStart with tag '%s' exists for this player's team. Falling back to the engine default selection."), *TeamStartTag.ToString());
	return Super::ChoosePlayerStart_Implementation(Player);
}

int32 ASWGameMode::GetActivePlayerCount() const
{
	const ASWGameState* SWGameState = GetGameState<ASWGameState>();
	if (!SWGameState)
	{
		return 0;
	}

	return SWGameState->GetTeamPlayerCount(ESWTeamId::TeamA)
		+ SWGameState->GetTeamPlayerCount(ESWTeamId::TeamB);
}

void ASWGameMode::StartWarmupIfNeeded()
{
	ASWGameState* SWGameState = GetGameState<ASWGameState>();
	if (!SWGameState || GetActivePlayerCount() == 0 || SWGameState->WarmupEndServerTime > 0.0)
	{
		return;
	}

	SWGameState->SetWarmupEndServerTime(SWGameState->GetServerWorldTimeSeconds() + WarmupDurationSeconds);
}

void ASWGameMode::CancelWarmupIfNoActivePlayers()
{
	ASWGameState* SWGameState = GetGameState<ASWGameState>();
	if (SWGameState && GetActivePlayerCount() == 0)
	{
		SWGameState->SetWarmupEndServerTime(0.0);
	}
}
