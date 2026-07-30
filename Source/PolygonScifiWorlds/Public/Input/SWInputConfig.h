// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "SWInputConfig.generated.h"

class UInputAction;
class UInputMappingContext;

/** 单个技能输入动作与原生输入 Tag 的配置映射。 */
USTRUCT(BlueprintType)
struct FSWAbilityInputAction
{
	GENERATED_BODY()

	/** 触发技能输入的 Enhanced Input Action。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> InputAction = nullptr;

	/** 输入路由到 GAS 时使用的原生 Gameplay Tag。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (Categories = "Ability.Input"))
	FGameplayTag InputTag;
};

/**
 * 单个本地玩家的不可变输入配置。
 *
 * 资产由 PlayerController 蓝图默认值唯一持有；Pawn 只读取配置，避免重生后产生第二份输入定义。
 */
UCLASS(BlueprintType)
class POLYGONSCIFIWORLDS_API USWInputConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	/** 本地玩家进入游戏时添加的 Gameplay 输入映射上下文。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Mapping")
	TObjectPtr<UInputMappingContext> DefaultMappingContext = nullptr;

	/** 由 Character 直接消费的移动输入。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Direct")
	TObjectPtr<UInputAction> MoveAction = nullptr;

	/** 由 Character 或 Controller 直接消费的视角输入。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Direct")
	TObjectPtr<UInputAction> LookAction = nullptr;

	/** 由 Character 直接消费的跳跃输入。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Direct")
	TObjectPtr<UInputAction> JumpAction = nullptr;

	/** 由 Character 直接消费的下蹲输入。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Direct")
	TObjectPtr<UInputAction> CrouchAction = nullptr;

	/** 交由 GAS 输入路由处理的技能输入。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Ability")
	TArray<FSWAbilityInputAction> AbilityInputActions;

	/** 按原生输入 Tag 查询对应的 Input Action；未配置时返回 nullptr。 */
	UFUNCTION(BlueprintPure, Category = "Input|Ability")
	const UInputAction* FindAbilityInputActionForTag(FGameplayTag InputTag) const;

	/** 返回所有技能输入映射，供 Pawn 在每次获得 InputComponent 时绑定。 */
	const TArray<FSWAbilityInputAction>& GetAbilityInputActions() const { return AbilityInputActions; }
};
