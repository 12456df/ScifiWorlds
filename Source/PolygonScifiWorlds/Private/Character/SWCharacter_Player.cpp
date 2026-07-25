// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "Character/SWCharacter_Player.h"

#include "AbilitySystemComponent.h"
#include "Player/SWPlayerState.h"

void ASWCharacter_Player::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// 服务器端：PlayerState 此时已就绪，完成 Owner/Avatar 绑定。
	InitAbilityActorInfo();
}

void ASWCharacter_Player::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// 拥有者客户端：PlayerState 复制到位后，执行与服务器一致的绑定。
	InitAbilityActorInfo();
}

void ASWCharacter_Player::InitAbilityActorInfo()
{
	ASWPlayerState* SWPlayerState = GetPlayerState<ASWPlayerState>();
	if (!SWPlayerState)
	{
		return;
	}

	UAbilitySystemComponent* PlayerASC = SWPlayerState->GetAbilitySystemComponent();
	if (!PlayerASC)
	{
		return;
	}

	// Owner = PlayerState（持久所有者），Avatar = 当前 Pawn。Avatar 改变时重复调用只更新绑定，
	// InitAbilityActorInfo 本身是幂等的。
	PlayerASC->InitAbilityActorInfo(SWPlayerState, this);

	// 缓存到基类指针，供 GetAbilitySystemComponent / GetAttributeSet 转发。
	AbilitySystemComponent = PlayerASC;
	AttributeSet = SWPlayerState->GetAttributeSet();

	// 注意：基础能力授予与初始化 Gameplay Effect 只应由服务器在“首次有效绑定”后执行一次，
	// 且需显式的已初始化保护。相关逻辑依赖数据驱动的进度/初始属性配置（M03 实现顺序第 4 项 /
	// 能力授予 M07），此处暂不实现。
}
