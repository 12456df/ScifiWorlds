// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/SWAbilityTypes.h"
#include "AbilitySystemComponent.h"
#include "SWAbilitySystemComponent.generated.h"

/**
 * Project ability system component for ScifiWorlds.
 *
 * Owns ability, effect, tag and attribute aggregation as described in
 * Docs/Systems/M03_GASCoreFramework.md. This module establishes the type; ability
 * granting, input binding and effect helpers land with later delivery steps.
 */
UCLASS()
class POLYGONSCIFIWORLDS_API USWAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	/** 记录一个本地技能输入按下事件；实际激活统一延后到本帧输入处理阶段。 */
	void AbilityInputTagPressed(FGameplayTag InputTag);

	/** 记录一个本地技能输入释放事件。 */
	void AbilityInputTagReleased(FGameplayTag InputTag);

	/** 在 PlayerController::PostProcessInput 中调用，处理按下、持续和释放的 Ability Spec 输入。 */
	void ProcessAbilityInput(float DeltaTime, bool bGamePaused);

	/** 仅服务器调用：授予蓝图默认值配置的启动技能，并用输入 Tag 写入对应 Ability Spec。 */
	void GrantStartupAbilities(const TArray<FSWStartupAbility>& StartupAbilities);

private:
	/** 本帧按下和释放的 Ability Spec；仅本地输入状态，不复制。 */
	TArray<FGameplayAbilitySpecHandle> InputPressedSpecHandles;
	TArray<FGameplayAbilitySpecHandle> InputReleasedSpecHandles;
};
