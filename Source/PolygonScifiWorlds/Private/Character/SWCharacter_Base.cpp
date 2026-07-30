// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "Character/SWCharacter_Base.h"

ASWCharacter_Base::ASWCharacter_Base(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;
}

void ASWCharacter_Base::BeginPlay()
{
	Super::BeginPlay();
}

UAbilitySystemComponent* ASWCharacter_Base::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ASWCharacter_Base::InitAbilityActorInfo()
{
	// 基类不做任何绑定；玩家与 AI 子类分别覆写。
}
