// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "UI/WidgetController/Overlay/SWSkillOverlayWidgetController.h"

#include "AbilitySystem/Abilities/Active/SWActiveGameplayAbility.h"
#include "AbilitySystem/SWAbilitySystemComponent.h"
#include "AbilitySystem/SWAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "GameplayTags/SWGameplayTags.h"
#include "GameFramework/GameStateBase.h"
#include "Player/SWPlayerState.h"
#include "GameState/SWGameState.h"

namespace
{
	FGameplayTag GetSkillInputTag(const FGameplayAbilitySpec& AbilitySpec)
	{
		const FGameplayTagContainer& SpecTags = AbilitySpec.GetDynamicSpecSourceTags();
		for (const FGameplayTag& Tag : SpecTags)
		{
			if (Tag == SWGameplayTags::Ability_Input_Skill1
				|| Tag == SWGameplayTags::Ability_Input_Skill2
				|| Tag == SWGameplayTags::Ability_Input_Skill3)
			{
				return Tag;
			}
		}

		return FGameplayTag();
	}

	bool IsSkillInputTag(const FGameplayTag InputTag)
	{
		return InputTag == SWGameplayTags::Ability_Input_Skill1
			|| InputTag == SWGameplayTags::Ability_Input_Skill2
			|| InputTag == SWGameplayTags::Ability_Input_Skill3;
	}
}

void USWSkillOverlayWidgetController::BroadcastInitialValues()
{
	TArray<FSWSkillSlotSnapshot> SkillSlots;
	SkillSlots.Reserve(3);
	SkillSlots.Add(MakeEmptySkillSlotSnapshot(SWGameplayTags::Ability_Input_Skill1));
	SkillSlots.Add(MakeEmptySkillSlotSnapshot(SWGameplayTags::Ability_Input_Skill2));
	SkillSlots.Add(MakeEmptySkillSlotSnapshot(SWGameplayTags::Ability_Input_Skill3));

	if (const USWAbilitySystemComponent* const AbilitySystemComponent = BoundAbilitySystemComponent.Get())
	{
		for (const FGameplayAbilitySpec& AbilitySpec : AbilitySystemComponent->GetActivatableAbilities())
		{
			FSWSkillSlotSnapshot Snapshot;
			if (!BuildSkillSlotSnapshot(AbilitySpec, Snapshot))
			{
				continue;
			}

			for (FSWSkillSlotSnapshot& SkillSlot : SkillSlots)
			{
				if (SkillSlot.InputTag == Snapshot.InputTag)
				{
					SkillSlot = MoveTemp(Snapshot);
					break;
				}
			}
		}
	}

	OnSkillBarInitialized.Broadcast(SkillSlots);
}

void USWSkillOverlayWidgetController::RefreshSkillBar()
{
	BroadcastInitialValues();
}

void USWSkillOverlayWidgetController::BindCallbacksToDependencies()
{
	UnbindCallbacks();

	USWAbilitySystemComponent* const AbilitySystemComponent = PlayerState
		? Cast<USWAbilitySystemComponent>(PlayerState->GetAbilitySystemComponent())
		: nullptr;
	if (!AbilitySystemComponent)
	{
		return;
	}

	BoundAbilitySystemComponent = AbilitySystemComponent;
	ActivatableAbilitySpecChangedHandle = AbilitySystemComponent->OnActivatableAbilitySpecChanged.AddUObject(this, &ThisClass::HandleActivatableAbilitySpecChanged);
	AbilitySpecDirtiedHandle = AbilitySystemComponent->AbilitySpecDirtiedCallbacks.AddUObject(this, &ThisClass::HandleAbilitySpecDirtied);
	ActiveGameplayEffectAddedHandle = AbilitySystemComponent->OnActiveGameplayEffectAddedDelegateToSelf.AddUObject(this, &ThisClass::HandleActiveGameplayEffectAdded);
	ActiveGameplayEffectRemovedHandle = AbilitySystemComponent->OnAnyGameplayEffectRemovedDelegate().AddUObject(this, &ThisClass::HandleActiveGameplayEffectRemoved);
	AbilityChargeBonusChangedHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(USWAttributeSet::GetAbilityChargeBonusAttribute()).AddUObject(this, &ThisClass::HandleAbilityChargeBonusChanged);
	AbilityPointsChangedHandle = PlayerState->OnAbilityPointsChanged.AddUObject(this, &ThisClass::HandleAbilityPointsChanged);

	// HUD 晚于 ASC 创建时，已存在的冷却 GE 不会再次触发 Added 委托，必须主动补登记其 Stack 回调。
	for (const FGameplayAbilitySpec& AbilitySpec : AbilitySystemComponent->GetActivatableAbilities())
	{
		const USWActiveGameplayAbility* const ActiveAbility = Cast<USWActiveGameplayAbility>(AbilitySpec.Ability);
		if (!ActiveAbility || !ActiveAbility->GetCooldownTag().IsValid())
		{
			continue;
		}

		const FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(ActiveAbility->GetCooldownTag().GetSingleTagContainer());
		for (const FActiveGameplayEffectHandle& Handle : AbilitySystemComponent->GetActiveEffects(Query))
		{
			TrackCooldownEffect(Handle, ActiveAbility->GetCooldownTag());
		}
	}
}

void USWSkillOverlayWidgetController::BeginDestroy()
{
	UnbindCallbacks();
	Super::BeginDestroy();
}

void USWSkillOverlayWidgetController::UnbindCallbacks()
{
	if (USWAbilitySystemComponent* const AbilitySystemComponent = BoundAbilitySystemComponent.Get(); AbilitySystemComponent && ActivatableAbilitySpecChangedHandle.IsValid())
	{
		AbilitySystemComponent->OnActivatableAbilitySpecChanged.Remove(ActivatableAbilitySpecChangedHandle);
	}
	if (USWAbilitySystemComponent* const AbilitySystemComponent = BoundAbilitySystemComponent.Get(); AbilitySystemComponent && AbilitySpecDirtiedHandle.IsValid())
	{
		AbilitySystemComponent->AbilitySpecDirtiedCallbacks.Remove(AbilitySpecDirtiedHandle);
	}
	if (USWAbilitySystemComponent* const AbilitySystemComponent = BoundAbilitySystemComponent.Get(); AbilitySystemComponent && ActiveGameplayEffectAddedHandle.IsValid())
	{
		AbilitySystemComponent->OnActiveGameplayEffectAddedDelegateToSelf.Remove(ActiveGameplayEffectAddedHandle);
	}
	if (USWAbilitySystemComponent* const AbilitySystemComponent = BoundAbilitySystemComponent.Get(); AbilitySystemComponent && ActiveGameplayEffectRemovedHandle.IsValid())
	{
		AbilitySystemComponent->OnAnyGameplayEffectRemovedDelegate().Remove(ActiveGameplayEffectRemovedHandle);
	}
	if (USWAbilitySystemComponent* const AbilitySystemComponent = BoundAbilitySystemComponent.Get(); AbilitySystemComponent && AbilityChargeBonusChangedHandle.IsValid())
	{
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(USWAttributeSet::GetAbilityChargeBonusAttribute()).Remove(AbilityChargeBonusChangedHandle);
	}
	if (PlayerState && AbilityPointsChangedHandle.IsValid())
	{
		PlayerState->OnAbilityPointsChanged.Remove(AbilityPointsChangedHandle);
	}

	if (USWAbilitySystemComponent* const AbilitySystemComponent = BoundAbilitySystemComponent.Get())
	{
		for (const TPair<FActiveGameplayEffectHandle, FDelegateHandle>& Pair : CooldownStackChangeHandles)
		{
			if (FOnActiveGameplayEffectStackChange* const StackChangeDelegate = AbilitySystemComponent->OnGameplayEffectStackChangeDelegate(Pair.Key))
			{
				StackChangeDelegate->Remove(Pair.Value);
			}
		}
	}

	ActivatableAbilitySpecChangedHandle.Reset();
	AbilitySpecDirtiedHandle.Reset();
	ActiveGameplayEffectAddedHandle.Reset();
	ActiveGameplayEffectRemovedHandle.Reset();
	AbilityChargeBonusChangedHandle.Reset();
	AbilityPointsChangedHandle.Reset();
	TrackedCooldownEffects.Reset();
	CooldownStackChangeHandles.Reset();
	BoundAbilitySystemComponent.Reset();
}

void USWSkillOverlayWidgetController::HandleActivatableAbilitySpecChanged(
	const FGameplayAbilitySpec& AbilitySpec,
	const ESWActivatableAbilitySpecChangeType ChangeType)
{
	const FGameplayTag InputTag = GetSkillInputTag(AbilitySpec);
	if (!IsSkillInputTag(InputTag))
	{
		return;
	}

	if (ChangeType == ESWActivatableAbilitySpecChangeType::Removed)
	{
		OnSkillSlotChanged.Broadcast(MakeEmptySkillSlotSnapshot(InputTag));
		return;
	}

	FSWSkillSlotSnapshot Snapshot;
	if (BuildSkillSlotSnapshot(AbilitySpec, Snapshot))
	{
		OnSkillSlotChanged.Broadcast(Snapshot);
	}
}

void USWSkillOverlayWidgetController::HandleAbilitySpecDirtied(const FGameplayAbilitySpec& AbilitySpec)
{
	FSWSkillSlotSnapshot Snapshot;
	if (BuildSkillSlotSnapshot(AbilitySpec, Snapshot))
	{
		OnSkillSlotChanged.Broadcast(Snapshot);
	}
}

void USWSkillOverlayWidgetController::HandleActiveGameplayEffectAdded(
	UAbilitySystemComponent* const TargetAbilitySystemComponent,
	const FGameplayEffectSpec& AppliedEffectSpec,
	const FActiveGameplayEffectHandle ActiveEffectHandle)
{
	if (TargetAbilitySystemComponent != BoundAbilitySystemComponent.Get())
	{
		return;
	}

	FGameplayTag CooldownTag;
	if (GetCooldownTagFromEffectSpec(AppliedEffectSpec, CooldownTag))
	{
		TrackCooldownEffect(ActiveEffectHandle, CooldownTag);
		RefreshSkillSlotForCooldownTag(CooldownTag);
	}
}

void USWSkillOverlayWidgetController::HandleActiveGameplayEffectRemoved(const FActiveGameplayEffect& ActiveEffect)
{
	FGameplayTag CooldownTag;
	if (!GetCooldownTagFromEffectSpec(ActiveEffect.Spec, CooldownTag))
	{
		return;
	}

	TrackedCooldownEffects.Remove(ActiveEffect.Handle);
	CooldownStackChangeHandles.Remove(ActiveEffect.Handle);
	RefreshSkillSlotForCooldownTag(CooldownTag);
}

void USWSkillOverlayWidgetController::HandleCooldownEffectStackChanged(
	const FActiveGameplayEffectHandle ActiveEffectHandle,
	const int32 NewStackCount,
	const int32 PreviousStackCount)
{
	if (const FGameplayTag* const CooldownTag = TrackedCooldownEffects.Find(ActiveEffectHandle))
	{
		RefreshSkillSlotForCooldownTag(*CooldownTag);
	}
}

void USWSkillOverlayWidgetController::HandleAbilityChargeBonusChanged(const FOnAttributeChangeData& ChangeData)
{
	// 充能上限是所有允许该修正的技能共享的 AttributeSet 聚合值，因此必须刷新这些既有槽位。
	for (const FGameplayAbilitySpec& AbilitySpec : BoundAbilitySystemComponent->GetActivatableAbilities())
	{
		FSWSkillSlotSnapshot Snapshot;
		if (BuildSkillSlotSnapshot(AbilitySpec, Snapshot))
		{
			OnSkillSlotChanged.Broadcast(Snapshot);
		}
	}
}

void USWSkillOverlayWidgetController::HandleAbilityPointsChanged(const int32 NewAbilityPoints)
{
	(void)NewAbilityPoints;

	// 技能点可用性会同时影响多个槽位；仅刷新已授予的固定主动技能，不重建 Widget。
	const USWAbilitySystemComponent* const AbilitySystemComponent = BoundAbilitySystemComponent.Get();
	if (!AbilitySystemComponent)
	{
		return;
	}

	for (const FGameplayAbilitySpec& AbilitySpec : AbilitySystemComponent->GetActivatableAbilities())
	{
		FSWSkillSlotSnapshot Snapshot;
		if (BuildSkillSlotSnapshot(AbilitySpec, Snapshot))
		{
			OnSkillSlotChanged.Broadcast(Snapshot);
		}
	}
}

bool USWSkillOverlayWidgetController::BuildSkillSlotSnapshot(const FGameplayAbilitySpec& AbilitySpec, FSWSkillSlotSnapshot& OutSnapshot) const
{
	const FGameplayTag InputTag = GetSkillInputTag(AbilitySpec);
	const USWActiveGameplayAbility* const ActiveAbility = Cast<USWActiveGameplayAbility>(AbilitySpec.Ability);
	if (!IsSkillInputTag(InputTag) || !ActiveAbility || !ActiveAbility->GetAbilityIdTag().IsValid())
	{
		return false;
	}

	OutSnapshot.InputTag = InputTag;
	OutSnapshot.bHasAssignedAbility = true;
	OutSnapshot.AbilityIdTag = ActiveAbility->GetAbilityIdTag();
	OutSnapshot.AbilityLevel = FMath::Max(1, AbilitySpec.Level);
	OutSnapshot.MaxAbilityLevel = ActiveAbility->GetMaxAbilityLevel();
	OutSnapshot.bUpgradeable = ActiveAbility->IsUpgradeable();
	OutSnapshot.bCanUpgrade = OutSnapshot.bUpgradeable
		&& PlayerState
		&& PlayerState->GetAbilityPoints() > 0
		&& AbilitySpec.Level < OutSnapshot.MaxAbilityLevel
		&& !AbilitySpec.IsActive();
	OutSnapshot.DisplayName = ActiveAbility->DisplayName;
	OutSnapshot.Icon = ActiveAbility->Icon;
	PopulateCooldownSnapshot(AbilitySpec, *ActiveAbility, OutSnapshot);
	return true;
}

FSWSkillSlotSnapshot USWSkillOverlayWidgetController::MakeEmptySkillSlotSnapshot(const FGameplayTag InputTag) const
{
	FSWSkillSlotSnapshot Snapshot;
	Snapshot.InputTag = InputTag;
	return Snapshot;
}

bool USWSkillOverlayWidgetController::GetCooldownTagFromEffectSpec(const FGameplayEffectSpec& EffectSpec, FGameplayTag& OutCooldownTag) const
{
	FGameplayTagContainer EffectTags;
	EffectSpec.GetAllGrantedTags(EffectTags);
	EffectSpec.GetAllAssetTags(EffectTags);

	if (const USWAbilitySystemComponent* const AbilitySystemComponent = BoundAbilitySystemComponent.Get())
	{
		for (const FGameplayAbilitySpec& AbilitySpec : AbilitySystemComponent->GetActivatableAbilities())
		{
			const USWActiveGameplayAbility* const ActiveAbility = Cast<USWActiveGameplayAbility>(AbilitySpec.Ability);
			if (ActiveAbility && ActiveAbility->GetCooldownTag().IsValid() && EffectTags.HasTagExact(ActiveAbility->GetCooldownTag()))
			{
				OutCooldownTag = ActiveAbility->GetCooldownTag();
				return true;
			}
		}
	}

	return false;
}

void USWSkillOverlayWidgetController::TrackCooldownEffect(const FActiveGameplayEffectHandle ActiveEffectHandle, const FGameplayTag& CooldownTag)
{
	USWAbilitySystemComponent* const AbilitySystemComponent = BoundAbilitySystemComponent.Get();
	if (!AbilitySystemComponent || !ActiveEffectHandle.IsValid() || !CooldownTag.IsValid() || TrackedCooldownEffects.Contains(ActiveEffectHandle))
	{
		return;
	}

	TrackedCooldownEffects.Add(ActiveEffectHandle, CooldownTag);
	if (FOnActiveGameplayEffectStackChange* const StackChangeDelegate = AbilitySystemComponent->OnGameplayEffectStackChangeDelegate(ActiveEffectHandle))
	{
		CooldownStackChangeHandles.Add(ActiveEffectHandle, StackChangeDelegate->AddUObject(this, &ThisClass::HandleCooldownEffectStackChanged));
	}
}

void USWSkillOverlayWidgetController::RefreshSkillSlotForCooldownTag(const FGameplayTag& CooldownTag)
{
	if (!CooldownTag.IsValid())
	{
		return;
	}

	if (const USWAbilitySystemComponent* const AbilitySystemComponent = BoundAbilitySystemComponent.Get())
	{
		for (const FGameplayAbilitySpec& AbilitySpec : AbilitySystemComponent->GetActivatableAbilities())
		{
			const USWActiveGameplayAbility* const ActiveAbility = Cast<USWActiveGameplayAbility>(AbilitySpec.Ability);
			if (!ActiveAbility || ActiveAbility->GetCooldownTag() != CooldownTag)
			{
				continue;
			}

			FSWSkillSlotSnapshot Snapshot;
			if (BuildSkillSlotSnapshot(AbilitySpec, Snapshot))
			{
				OnSkillSlotChanged.Broadcast(Snapshot);
			}
			return;
		}
	}
}

void USWSkillOverlayWidgetController::PopulateCooldownSnapshot(
	const FGameplayAbilitySpec& AbilitySpec,
	const USWActiveGameplayAbility& ActiveAbility,
	FSWSkillSlotSnapshot& InOutSnapshot) const
{
	const USWAbilitySystemComponent* const AbilitySystemComponent = BoundAbilitySystemComponent.Get();
	InOutSnapshot.CooldownTag = ActiveAbility.GetCooldownTag();
	InOutSnapshot.MaxCharges = ActiveAbility.GetMaxChargesForLevel(InOutSnapshot.AbilityLevel, AbilitySystemComponent);
	InOutSnapshot.CurrentCharges = InOutSnapshot.MaxCharges;

	if (!AbilitySystemComponent || !InOutSnapshot.CooldownTag.IsValid())
	{
		return;
	}

	const FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(InOutSnapshot.CooldownTag.GetSingleTagContainer());
	const TArray<FActiveGameplayEffectHandle> ActiveEffects = AbilitySystemComponent->GetActiveEffects(Query);
	int32 SpentCharges = 0;
	for (const FActiveGameplayEffectHandle& ActiveEffectHandle : ActiveEffects)
	{
		SpentCharges += FMath::Max(0, AbilitySystemComponent->GetCurrentStackCount(ActiveEffectHandle));
	}

	InOutSnapshot.bIsCooldownActive = SpentCharges > 0;
	InOutSnapshot.CurrentCharges = FMath::Clamp(InOutSnapshot.MaxCharges - SpentCharges, 0, InOutSnapshot.MaxCharges);
	if (!InOutSnapshot.bIsCooldownActive)
	{
		return;
	}

	float ShortestTimeRemaining = TNumericLimits<float>::Max();
	float DurationForShortestTime = 0.f;
	for (const TPair<float, float>& TimeAndDuration : AbilitySystemComponent->GetActiveEffectsTimeRemainingAndDuration(Query))
	{
		if (TimeAndDuration.Key >= 0.f && TimeAndDuration.Key < ShortestTimeRemaining)
		{
			ShortestTimeRemaining = TimeAndDuration.Key;
			DurationForShortestTime = FMath::Max(0.f, TimeAndDuration.Value);
		}
	}

	if (ShortestTimeRemaining == TNumericLimits<float>::Max())
	{
		return;
	}

	InOutSnapshot.NextChargeRemainingSeconds = ShortestTimeRemaining;
	InOutSnapshot.NextChargeDurationSeconds = DurationForShortestTime;
	if (const UWorld* const World = GetWorld())
	{
		if (const AGameStateBase* const GameStateBase = World->GetGameState())
		{
			InOutSnapshot.NextChargeEndServerTimeSeconds = static_cast<float>(GameStateBase->GetServerWorldTimeSeconds() + ShortestTimeRemaining);
		}
	}
}
