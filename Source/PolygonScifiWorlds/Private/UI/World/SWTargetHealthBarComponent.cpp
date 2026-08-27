// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "UI/World/SWTargetHealthBarComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/SWAttributeSet.h"
#include "AbilitySystemInterface.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "UI/Widget/SWTargetHealthBarWidget.h"

USWTargetHealthBarComponent::USWTargetHealthBarComponent()
{
	SetWidgetSpace(EWidgetSpace::Screen);
	SetDrawSize(FVector2D(180.f, 24.f));
	SetPivot(FVector2D(0.5f, 0.5f));
	SetVisibility(false);
}

void USWTargetHealthBarComponent::BeginPlay()
{
	Super::BeginPlay();
	BaseDrawSize = GetDrawSize();

	// Dedicated Server 不创建或更新任何 UMG 表现。
	if (GetNetMode() != NM_DedicatedServer)
	{
		SetVisibility(false);
	}
}

void USWTargetHealthBarComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* const World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HideTimerHandle);
	}

	UnbindFromOwnerAbilitySystem();
	Super::EndPlay(EndPlayReason);
}

void USWTargetHealthBarComponent::ShowForLocalAttacker()
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	// 受击发生在显示范围外时不保留资格；之后走进范围也必须再次受到本地攻击才会出现血条。
	if (!bInsideLocalAttackerDisplayRange)
	{
		return;
	}

	BindToOwnerAbilitySystem();
	RefreshWidgetHealth();

	bRecentlyDamagedByLocalAttacker = true;
	RefreshLocalVisibility();

	if (UWorld* const World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			HideTimerHandle,
			this,
			&ThisClass::HideForLocalAttacker,
			FMath::Max(0.1f, VisibleDurationSeconds),
			false);
	}
}

void USWTargetHealthBarComponent::SetLocalAttackerDisplayRangeState(const bool bInDisplayRange, const float InSizeScale)
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	bInsideLocalAttackerDisplayRange = bInDisplayRange;
	if (bInsideLocalAttackerDisplayRange)
	{
		SetDrawSize(BaseDrawSize * FMath::Clamp(InSizeScale, 0.1f, 1.f));
		// 进入范围本身不会显示血条；必须由实际伤害通知再次触发。
		RefreshLocalVisibility();
		return;
	}

	// 离开范围后必须重新受击才能显示，避免玩家离开又返回时复用旧的显示资格。
	bRecentlyDamagedByLocalAttacker = false;
	if (UWorld* const World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HideTimerHandle);
	}
	RefreshLocalVisibility();
}

void USWTargetHealthBarComponent::BindToOwnerAbilitySystem()
{
	if (BoundAbilitySystemComponent.IsValid())
	{
		return;
	}

	IAbilitySystemInterface* const AbilitySystemInterface = Cast<IAbilitySystemInterface>(GetOwner());
	UAbilitySystemComponent* const AbilitySystemComponent = AbilitySystemInterface
		? AbilitySystemInterface->GetAbilitySystemComponent()
		: nullptr;
	if (!AbilitySystemComponent)
	{
		return;
	}

	BoundAbilitySystemComponent = AbilitySystemComponent;
	HealthChangedHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(USWAttributeSet::GetHealthAttribute())
		.AddUObject(this, &ThisClass::HandleHealthChanged);
	MaxHealthChangedHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(USWAttributeSet::GetMaxHealthAttribute())
		.AddUObject(this, &ThisClass::HandleMaxHealthChanged);
}

void USWTargetHealthBarComponent::UnbindFromOwnerAbilitySystem()
{
	if (UAbilitySystemComponent* const AbilitySystemComponent = BoundAbilitySystemComponent.Get())
	{
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(USWAttributeSet::GetHealthAttribute()).Remove(HealthChangedHandle);
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(USWAttributeSet::GetMaxHealthAttribute()).Remove(MaxHealthChangedHandle);
	}

	BoundAbilitySystemComponent.Reset();
	HealthChangedHandle.Reset();
	MaxHealthChangedHandle.Reset();
}

void USWTargetHealthBarComponent::RefreshWidgetHealth() const
{
	const UAbilitySystemComponent* const AbilitySystemComponent = BoundAbilitySystemComponent.Get();
	USWTargetHealthBarWidget* const HealthBarWidget = Cast<USWTargetHealthBarWidget>(GetUserWidgetObject());
	if (!AbilitySystemComponent || !HealthBarWidget)
	{
		return;
	}

	HealthBarWidget->SetHealthSnapshot(
		AbilitySystemComponent->GetNumericAttribute(USWAttributeSet::GetHealthAttribute()),
		AbilitySystemComponent->GetNumericAttribute(USWAttributeSet::GetMaxHealthAttribute()));
}

void USWTargetHealthBarComponent::HideForLocalAttacker()
{
	bRecentlyDamagedByLocalAttacker = false;
	RefreshLocalVisibility();
}

void USWTargetHealthBarComponent::RefreshLocalVisibility()
{
	SetVisibility(bRecentlyDamagedByLocalAttacker && bInsideLocalAttackerDisplayRange);
}

void USWTargetHealthBarComponent::HandleHealthChanged(const FOnAttributeChangeData& ChangeData)
{
	(void)ChangeData;
	if (bRecentlyDamagedByLocalAttacker && bInsideLocalAttackerDisplayRange)
	{
		RefreshWidgetHealth();
	}
}

void USWTargetHealthBarComponent::HandleMaxHealthChanged(const FOnAttributeChangeData& ChangeData)
{
	(void)ChangeData;
	if (bRecentlyDamagedByLocalAttacker && bInsideLocalAttackerDisplayRange)
	{
		RefreshWidgetHealth();
	}
}
