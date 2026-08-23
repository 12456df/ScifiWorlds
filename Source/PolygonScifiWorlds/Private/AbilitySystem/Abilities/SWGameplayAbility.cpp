// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "AbilitySystem/Abilities/SWGameplayAbility.h"

#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "GameState/SWGameState.h"
#include "GameFramework/Actor.h"
#include "GameFramework/GameMode.h"
#include "GameplayTags/SWGameplayTags.h"

bool USWGameplayAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	// 不依赖每个具体 Ability 的蓝图都重复配置 ActivationBlockedTags，死亡是所有项目 Ability 的统一门槛。
	if (!ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid()
		|| ActorInfo->AbilitySystemComponent->HasMatchingGameplayTag(SWGameplayTags::State_Dead))
	{
		return false;
	}

	if (!bRequiresMatchInProgress)
	{
		return true;
	}

	// 预测端读取复制后的 GameState，服务器也读取同一个 MatchState；没有可信比赛状态时保守拒绝战斗激活。
	const AActor* const AvatarActor = ActorInfo->AvatarActor.Get();
	const UWorld* const World = AvatarActor ? AvatarActor->GetWorld() : nullptr;
	const ASWGameState* const SWGameState = World ? World->GetGameState<ASWGameState>() : nullptr;
	return SWGameState && SWGameState->GetMatchState() == MatchState::InProgress;
}

float USWGameplayAbility::GetOwnerAttributeValue(const FGameplayAttribute& Attribute, float DefaultValue) const
{
	if (const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		bool bFound = false;
		const float Value = ASC->GetGameplayAttributeValue(Attribute, bFound);
		if (bFound)
		{
			return Value;
		}
	}
	return DefaultValue;
}
