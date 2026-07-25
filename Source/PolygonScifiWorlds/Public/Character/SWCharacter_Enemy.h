// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/SWCharacter_Base.h"
#include "SWCharacter_Enemy.generated.h"

/**
 * AI 敌方角色基类（小兵、野怪等的共同父类）。
 *
 * 与玩家不同，AI 没有 PlayerState，ASC 与 AttributeSet 直接由 Character 自身持有，
 * 使用 Minimal 复制模式（AI 无需向拥有者客户端复制完整 GameplayEffect）。
 */
UCLASS()
class POLYGONSCIFIWORLDS_API ASWCharacter_Enemy : public ASWCharacter_Base
{
	GENERATED_BODY()

public:
	ASWCharacter_Enemy();

protected:
	virtual void BeginPlay() override;
	virtual void InitAbilityActorInfo() override;
};
