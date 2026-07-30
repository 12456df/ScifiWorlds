// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "SWCharacter_Base.generated.h"

class UAbilitySystemComponent;
class USWAttributeSet;

/**
 * ScifiWorlds 所有角色的抽象基类。
 *
 * 统一提供 ASC 与 AttributeSet 的查询入口（IAbilitySystemInterface），但不规定它们的归属：
 *   - 玩家角色（ASWCharacter_Player）的 ASC 位于 ASWPlayerState，Character 仅作 Avatar；
 *   - AI 角色（ASWCharacter_Enemy）自身持有 ASC。
 * 具体的 Owner/Avatar 绑定由子类覆写 InitAbilityActorInfo 完成。
 */
UCLASS(Abstract)
class POLYGONSCIFIWORLDS_API ASWCharacter_Base : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ASWCharacter_Base(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~ Begin IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	//~ End IAbilitySystemInterface

	USWAttributeSet* GetAttributeSet() const { return AttributeSet; }

protected:
	virtual void BeginPlay() override;

	/** 完成 ASC 的 Owner/Avatar 绑定。基类为空，由玩家/AI 子类分别实现。可重复调用。 */
	virtual void InitAbilityActorInfo();

	// 由子类填充：玩家从 PlayerState 缓存，AI 在构造时自建。
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<USWAttributeSet> AttributeSet;
};
