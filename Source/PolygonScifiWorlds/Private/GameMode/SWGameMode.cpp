// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "GameMode/SWGameMode.h"

#include "AbilitySystem/Data/SWProgressionData.h"
#include "AbilitySystem/SWAbilitySystemComponent.h"
#include "Character/SWCharacter_Base.h"
#include "Character/SWCharacter_Player.h"
#include "Interaction/SWDeathTypes.h"
#include "Engine/World.h"
#include "Economy/SWEconomyData.h"
#include "Shop/SWShopCatalogData.h"
#include "EngineUtils.h"
#include "GameState/SWGameState.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "Player/SWPlayerController.h"
#include "Player/SWPlayerState.h"
#include "TimerManager.h"

namespace
{
	bool IsPIEWorld(const UWorld* World)
	{
		return World && World->WorldType == EWorldType::PIE;
	}

	ESWTeamId SelectBalancedPIETeam(const ASWGameState& GameState)
	{
		return GameState.GetTeamPlayerCount(ESWTeamId::TeamA) <= GameState.GetTeamPlayerCount(ESWTeamId::TeamB)
			? ESWTeamId::TeamA
			: ESWTeamId::TeamB;
	}
}

ASWGameMode::ASWGameMode()
{
	// 比赛由准备期结束后的服务器显式启动，而不是在首名玩家进入时立即开始。
	bDelayedStart = true;

	// 统一注册 M02 的 Gameplay Framework 类型。
	DefaultPawnClass = ASWCharacter_Player::StaticClass();
	GameStateClass = ASWGameState::StaticClass();
	PlayerControllerClass = ASWPlayerController::StaticClass();
	PlayerStateClass = ASWPlayerState::StaticClass();
}

void ASWGameMode::RestartPlayer(AController* NewPlayer)
{
	Super::RestartPlayer(NewPlayer);

	if (ASWCharacter_Player* const PlayerCharacter = NewPlayer ? Cast<ASWCharacter_Player>(NewPlayer->GetPawn()) : nullptr)
	{
		BindPlayerDeathDelegate(PlayerCharacter);
	}
}

void ASWGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (ASWGameState* SWGameState = GetGameState<ASWGameState>())
	{
		SWGameState->SetProgressionDataAuthority(ProgressionData);
		SWGameState->SetEconomyDataAuthority(EconomyData);
		SWGameState->SetShopCatalogDataAuthority(ShopCatalogData);
	}
}

void ASWGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(PassiveGoldIncomeTimer);
	Super::EndPlay(EndPlayReason);
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
	const bool bIsPIE = IsPIEWorld(GetWorld());

	// 打包版本保持正式对局规则；PIE 允许在开局后加入，以避免编辑器多客户端启动时序干扰快速验证。
	if (!bIsPIE && GetMatchState() != MatchState::WaitingToStart)
	{
		return TEXT("比赛已开始，无法加入。");
	}

	ESWTeamId RequestedTeamId = ESWTeamId::None;

	ASWGameState* SWGameState = GetGameState<ASWGameState>();
	ASWPlayerState* SWPlayerState = NewPlayerController ? NewPlayerController->GetPlayerState<ASWPlayerState>() : nullptr;
	if (!SWGameState || !SWPlayerState)
	{
		return TEXT("无法加入该对局。");
	}

	if (bIsPIE)
	{
		// PIE 不依赖每个客户端各自的 URL 参数；服务器按当前人数平衡分配队伍。
		RequestedTeamId = SelectBalancedPIETeam(*SWGameState);
	}
	else if (!TryParseRequestedTeamId(Options, RequestedTeamId))
	{
		return TEXT("请选择有效阵营。");
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

	if (ASWPlayerState* const PlayerState = NewPlayer ? NewPlayer->GetPlayerState<ASWPlayerState>() : nullptr)
	{
		PlayerState->InitializeEconomyAuthority();
	}

	// InitNewPlayer 已完成选队验证；首名有效玩家据此启动准备期。
	StartWarmupIfNeeded();
}

void ASWGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);

	// AGameMode 默认只在 InProgress 阶段生成 Pawn；本项目允许玩家在准备期内出生并移动。
	if (GetMatchState() != MatchState::WaitingToStart || !NewPlayer || NewPlayer->GetPawn())
	{
		return;
	}

	const ASWPlayerState* SWPlayerState = NewPlayer->GetPlayerState<ASWPlayerState>();
	if (!SWPlayerState || (SWPlayerState->GetTeamId() != ESWTeamId::TeamA && SWPlayerState->GetTeamId() != ESWTeamId::TeamB))
	{
		UE_LOG(LogTemp, Error, TEXT("准备期玩家没有有效队伍，无法生成 Pawn。"));
		return;
	}

	RestartPlayer(NewPlayer);
}

void ASWGameMode::Logout(AController* Exiting)
{
	if (APlayerController* const PlayerController = Cast<APlayerController>(Exiting))
	{
		const TWeakObjectPtr<APlayerController> ControllerKey(PlayerController);
		if (FTimerHandle* const RespawnTimer = PlayerRespawnTimers.Find(ControllerKey))
		{
			GetWorldTimerManager().ClearTimer(*RespawnTimer);
			PlayerRespawnTimers.Remove(ControllerKey);
		}
	}

	Super::Logout(Exiting);

	// 父类已把退出玩家标为 Inactive，队伍计数会自动忽略该状态。
	CancelWarmupIfNoActivePlayers();
}

void ASWGameMode::BindPlayerDeathDelegate(ASWCharacter_Player* const PlayerCharacter)
{
	if (!PlayerCharacter)
	{
		return;
	}

	FSWOnDeath& OnDeath = PlayerCharacter->GetOnDeathDelegate();
	OnDeath.RemoveAll(this);
	OnDeath.AddUObject(this, &ThisClass::HandlePlayerDeathAuthority);
}

void ASWGameMode::HandlePlayerDeathAuthority(const FSWDeathContext& DeathContext)
{
	if (!IsPlayerRespawnAllowed())
	{
		return;
	}

	ASWCharacter_Player* const DeadPlayerCharacter = Cast<ASWCharacter_Player>(DeathContext.VictimActor);
	APlayerController* const PlayerController = DeadPlayerCharacter ? Cast<APlayerController>(DeadPlayerCharacter->GetController()) : nullptr;
	ASWPlayerState* const PlayerState = PlayerController ? PlayerController->GetPlayerState<ASWPlayerState>() : nullptr;
	if (!DeadPlayerCharacter || !DeadPlayerCharacter->IsDeadCommitted() || !PlayerController || !PlayerState)
	{
		return;
	}

	// 死亡后立即取消交易资格；重生时须重新进入商店区域才可交易。
	PlayerState->ClearShopTradeAccessAuthority();

	const ESWTeamId TeamId = PlayerState->GetTeamId();
	if (TeamId != ESWTeamId::TeamA && TeamId != ESWTeamId::TeamB)
	{
		return;
	}

	const float RespawnDelaySeconds = ProgressionData
		? FMath::Max(0.f, ProgressionData->RespawnDelayByLevel.GetValueAtLevel(PlayerState->GetPlayerLevel()))
		: 0.f;
	const TWeakObjectPtr<APlayerController> ControllerKey(PlayerController);
	FTimerHandle& RespawnTimer = PlayerRespawnTimers.FindOrAdd(ControllerKey);
	GetWorldTimerManager().ClearTimer(RespawnTimer);
	GetWorldTimerManager().SetTimer(
		RespawnTimer,
		FTimerDelegate::CreateUObject(this, &ThisClass::RespawnPlayerAuthority, ControllerKey),
		RespawnDelaySeconds,
		false);
}

void ASWGameMode::RespawnPlayerAuthority(const TWeakObjectPtr<APlayerController> PlayerController)
{
	PlayerRespawnTimers.Remove(PlayerController);

	APlayerController* const Controller = PlayerController.Get();
	ASWPlayerState* const PlayerState = Controller ? Controller->GetPlayerState<ASWPlayerState>() : nullptr;
	ASWCharacter_Player* const DeadPlayerCharacter = Controller ? Cast<ASWCharacter_Player>(Controller->GetPawn()) : nullptr;
	if (!IsPlayerRespawnAllowed() || !Controller || !PlayerState || !DeadPlayerCharacter || !DeadPlayerCharacter->IsDeadCommitted())
	{
		return;
	}

	const ESWTeamId TeamId = PlayerState->GetTeamId();
	if (TeamId != ESWTeamId::TeamA && TeamId != ESWTeamId::TeamB)
	{
		return;
	}

	Controller->UnPossess();
	DeadPlayerCharacter->Destroy();

	if (USWAbilitySystemComponent* const AbilitySystemComponent = Cast<USWAbilitySystemComponent>(PlayerState->GetAbilitySystemComponent()))
	{
		AbilitySystemComponent->SetDeadStateTagAuthority(false);
	}

	RestartPlayer(Controller);

	if (ASWCharacter_Player* const RespawnedPlayerCharacter = Cast<ASWCharacter_Player>(Controller->GetPawn()))
	{
		RespawnedPlayerCharacter->ApplyRespawnInvulnerabilityEffectAuthority();
	}
}

bool ASWGameMode::IsPlayerRespawnAllowed() const
{
	const FName CurrentMatchState = GetMatchState();
	return CurrentMatchState == MatchState::WaitingToStart || CurrentMatchState == MatchState::InProgress;
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
	StartPassiveGoldIncomeAuthority();
}

void ASWGameMode::HandleMatchHasEnded()
{
	GetWorldTimerManager().ClearTimer(PassiveGoldIncomeTimer);
	Super::HandleMatchHasEnded();
}

void ASWGameMode::StartPassiveGoldIncomeAuthority()
{
	if (!HasAuthority() || !EconomyData)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(PassiveGoldIncomeTimer);
	GetWorldTimerManager().SetTimer(
		PassiveGoldIncomeTimer,
		this,
		&ThisClass::GrantPassiveGoldIncomeAuthority,
		1.0f,
		true);
}

void ASWGameMode::GrantPassiveGoldIncomeAuthority()
{
	if (!HasAuthority() || GetMatchState() != MatchState::InProgress || !EconomyData)
	{
		return;
	}

	ASWGameState* const SWGameState = GetGameState<ASWGameState>();
	if (!SWGameState)
	{
		return;
	}

	for (APlayerState* const PlayerState : SWGameState->PlayerArray)
	{
		ASWPlayerState* const SWPlayerState = Cast<ASWPlayerState>(PlayerState);
		if (!SWPlayerState || SWPlayerState->IsInactive()
			|| (SWPlayerState->GetTeamId() != ESWTeamId::TeamA && SWPlayerState->GetTeamId() != ESWTeamId::TeamB))
		{
			continue;
		}

		// 不检查 Pawn 或死亡状态：金币属于 PlayerState，死亡期间仍按当前等级增长。
		SWPlayerState->GrantPassiveGoldIncomeAuthority(EconomyData->GetPassiveGoldPerSecondAtLevel(SWPlayerState->GetPlayerLevel()));
	}
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
