// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/SWCharacter_Base.h"
#include "SWCharacter_Player.generated.h"

/**
 * 玩家可操控角色。
 *
 * ASC 与 AttributeSet 归属 ASWPlayerState（跨重生存活），本类只作为 ASC 的 Avatar。
 * 绑定在两端触发：服务器在 PossessedBy 后、拥有者客户端在 OnRep_PlayerState 后各绑定一次。
 */
UCLASS()
class POLYGONSCIFIWORLDS_API ASWCharacter_Player : public ASWCharacter_Base
{
	GENERATED_BODY()

public:
	//~ Begin AActor/APawn interface
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	//~ End AActor/APawn interface

protected:
	virtual void InitAbilityActorInfo() override;
};
