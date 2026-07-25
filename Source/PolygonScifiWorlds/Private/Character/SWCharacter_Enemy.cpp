// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "Character/SWCharacter_Enemy.h"

#include "AbilitySystem/SWAbilitySystemComponent.h"
#include "AbilitySystem/SWAttributeSet.h"

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

	// 注意：AI 的基础能力授予与初始属性初始化只应由服务器执行一次，依赖后续数据驱动配置
	//（M03 实现顺序第 4 项 / 能力授予 M07），此处暂不实现。
}
