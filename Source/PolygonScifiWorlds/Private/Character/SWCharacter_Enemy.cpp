// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "Character/SWCharacter_Enemy.h"

#include "AbilitySystem/SWAbilitySystemComponent.h"
#include "AbilitySystem/SWAttributeSet.h"
#include "Net/UnrealNetwork.h"

ASWCharacter_Enemy::ASWCharacter_Enemy()
{
	// AI 自身持有 ASC 与 AttributeSet。
	AbilitySystemComponent = CreateDefaultSubobject<USWAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	// AI 使用 Minimal：GameplayEffect 不复制给模拟客户端，仅复制 Tag/Cue 等必要表现信息。
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	AttributeSet = CreateDefaultSubobject<USWAttributeSet>(TEXT("AttributeSet"));
}

void ASWCharacter_Enemy::BeginPlay()
{
	Super::BeginPlay();

	// ASC 就在自身，服务器与客户端都在 BeginPlay 完成 Owner=Avatar=this 的绑定。
	InitAbilityActorInfo();
}

void ASWCharacter_Enemy::InitAbilityActorInfo()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	// AI 的 Owner 与 Avatar 均为自身。
	AbilitySystemComponent->InitAbilityActorInfo(this, this);

	if (HasAuthority())
	{
		SetTeamIdAuthority(TeamId);
		ApplyCombatantInitializationEffectsAuthority(CombatLevel, true);
	}

	// AI 的启动能力会在后续能力模块按独立的服务器入口授予。
}

int32 ASWCharacter_Enemy::GetCombatLevel_Implementation() const
{
	return CombatLevel;
}

ESWTeamId ASWCharacter_Enemy::GetTeamId() const
{
	return TeamId;
}

void ASWCharacter_Enemy::SetTeamIdAuthority(const ESWTeamId NewTeamId)
{
	check(HasAuthority());

	if (NewTeamId != ESWTeamId::None && NewTeamId != ESWTeamId::TeamA && NewTeamId != ESWTeamId::TeamB)
	{
		ensureMsgf(false, TEXT("敌方单位 %s 收到了无效 TeamId。"), *GetName());
		return;
	}

	TeamId = NewTeamId;
	if (USWAbilitySystemComponent* const SWAbilitySystemComponent = Cast<USWAbilitySystemComponent>(AbilitySystemComponent))
	{
		SWAbilitySystemComponent->SetTeamIdTagAuthority(TeamId);
	}

	ForceNetUpdate();
}

void ASWCharacter_Enemy::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASWCharacter_Enemy, TeamId);
}
