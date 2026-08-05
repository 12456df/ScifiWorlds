// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "UI/WidgetController/Overlay/SWAttributeOverlayWidgetController.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/SWAttributeSet.h"
#include "Player/SWPlayerState.h"

void USWAttributeOverlayWidgetController::BroadcastInitialValues()
{
	BroadcastHealth();
	BroadcastMana();
	BroadcastStamina();
	BroadcastHealthRegeneration();
	BroadcastManaRegeneration();
	BroadcastStaminaRegeneration();
}

void USWAttributeOverlayWidgetController::BindCallbacksToDependencies()
{
	UnbindCallbacks();

	UAbilitySystemComponent* AbilitySystemComponent = PlayerState ? PlayerState->GetAbilitySystemComponent() : nullptr;
	if (!AbilitySystemComponent)
	{
		return;
	}

	BoundAbilitySystemComponent = AbilitySystemComponent;
	HealthChangedHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(USWAttributeSet::GetHealthAttribute()).AddUObject(this, &ThisClass::HandleHealthChanged);
	MaxHealthChangedHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(USWAttributeSet::GetMaxHealthAttribute()).AddUObject(this, &ThisClass::HandleMaxHealthChanged);
	ManaChangedHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(USWAttributeSet::GetManaAttribute()).AddUObject(this, &ThisClass::HandleManaChanged);
	MaxManaChangedHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(USWAttributeSet::GetMaxManaAttribute()).AddUObject(this, &ThisClass::HandleMaxManaChanged);
	StaminaChangedHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(USWAttributeSet::GetStaminaAttribute()).AddUObject(this, &ThisClass::HandleStaminaChanged);
	MaxStaminaChangedHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(USWAttributeSet::GetMaxStaminaAttribute()).AddUObject(this, &ThisClass::HandleMaxStaminaChanged);
	HealthRegenerationChangedHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(USWAttributeSet::GetHealthRegenerationAttribute()).AddUObject(this, &ThisClass::HandleHealthRegenerationChanged);
	ManaRegenerationChangedHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(USWAttributeSet::GetManaRegenerationAttribute()).AddUObject(this, &ThisClass::HandleManaRegenerationChanged);
	StaminaRegenerationChangedHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(USWAttributeSet::GetStaminaRegenerationAttribute()).AddUObject(this, &ThisClass::HandleStaminaRegenerationChanged);
}

void USWAttributeOverlayWidgetController::BeginDestroy()
{
	UnbindCallbacks();
	Super::BeginDestroy();
}

void USWAttributeOverlayWidgetController::BroadcastHealth()
{
	if (const UAbilitySystemComponent* AbilitySystemComponent = BoundAbilitySystemComponent.Get())
	{
		OnHealthChanged.Broadcast(MakeResourceSnapshot(
			AbilitySystemComponent->GetNumericAttribute(USWAttributeSet::GetHealthAttribute()),
			AbilitySystemComponent->GetNumericAttribute(USWAttributeSet::GetMaxHealthAttribute())));
	}
}

void USWAttributeOverlayWidgetController::BroadcastMana()
{
	if (const UAbilitySystemComponent* AbilitySystemComponent = BoundAbilitySystemComponent.Get())
	{
		OnManaChanged.Broadcast(MakeResourceSnapshot(
			AbilitySystemComponent->GetNumericAttribute(USWAttributeSet::GetManaAttribute()),
			AbilitySystemComponent->GetNumericAttribute(USWAttributeSet::GetMaxManaAttribute())));
	}
}

void USWAttributeOverlayWidgetController::BroadcastStamina()
{
	if (const UAbilitySystemComponent* AbilitySystemComponent = BoundAbilitySystemComponent.Get())
	{
		OnStaminaChanged.Broadcast(MakeResourceSnapshot(
			AbilitySystemComponent->GetNumericAttribute(USWAttributeSet::GetStaminaAttribute()),
			AbilitySystemComponent->GetNumericAttribute(USWAttributeSet::GetMaxStaminaAttribute())));
	}
}

void USWAttributeOverlayWidgetController::BroadcastHealthRegeneration()
{
	if (const UAbilitySystemComponent* AbilitySystemComponent = BoundAbilitySystemComponent.Get())
	{
		OnHealthRegenerationChanged.Broadcast(MakeRegenerationSnapshot(
			AbilitySystemComponent->GetNumericAttribute(USWAttributeSet::GetHealthRegenerationAttribute())));
	}
}

void USWAttributeOverlayWidgetController::BroadcastManaRegeneration()
{
	if (const UAbilitySystemComponent* AbilitySystemComponent = BoundAbilitySystemComponent.Get())
	{
		OnManaRegenerationChanged.Broadcast(MakeRegenerationSnapshot(
			AbilitySystemComponent->GetNumericAttribute(USWAttributeSet::GetManaRegenerationAttribute())));
	}
}

void USWAttributeOverlayWidgetController::BroadcastStaminaRegeneration()
{
	if (const UAbilitySystemComponent* AbilitySystemComponent = BoundAbilitySystemComponent.Get())
	{
		OnStaminaRegenerationChanged.Broadcast(MakeRegenerationSnapshot(
			AbilitySystemComponent->GetNumericAttribute(USWAttributeSet::GetStaminaRegenerationAttribute())));
	}
}

void USWAttributeOverlayWidgetController::UnbindCallbacks()
{
	if (UAbilitySystemComponent* AbilitySystemComponent = BoundAbilitySystemComponent.Get())
	{
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(USWAttributeSet::GetHealthAttribute()).Remove(HealthChangedHandle);
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(USWAttributeSet::GetMaxHealthAttribute()).Remove(MaxHealthChangedHandle);
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(USWAttributeSet::GetManaAttribute()).Remove(ManaChangedHandle);
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(USWAttributeSet::GetMaxManaAttribute()).Remove(MaxManaChangedHandle);
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(USWAttributeSet::GetStaminaAttribute()).Remove(StaminaChangedHandle);
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(USWAttributeSet::GetMaxStaminaAttribute()).Remove(MaxStaminaChangedHandle);
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(USWAttributeSet::GetHealthRegenerationAttribute()).Remove(HealthRegenerationChangedHandle);
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(USWAttributeSet::GetManaRegenerationAttribute()).Remove(ManaRegenerationChangedHandle);
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(USWAttributeSet::GetStaminaRegenerationAttribute()).Remove(StaminaRegenerationChangedHandle);
	}

	BoundAbilitySystemComponent.Reset();
	HealthChangedHandle.Reset();
	MaxHealthChangedHandle.Reset();
	ManaChangedHandle.Reset();
	MaxManaChangedHandle.Reset();
	StaminaChangedHandle.Reset();
	MaxStaminaChangedHandle.Reset();
	HealthRegenerationChangedHandle.Reset();
	ManaRegenerationChangedHandle.Reset();
	StaminaRegenerationChangedHandle.Reset();
}

FSWOverlayResourceSnapshot USWAttributeOverlayWidgetController::MakeResourceSnapshot(const float Current, const float Maximum) const
{
	FSWOverlayResourceSnapshot Snapshot;
	Snapshot.Current = Current;
	Snapshot.Maximum = Maximum;
	Snapshot.Percent = Maximum > 0.f ? FMath::Clamp(Current / Maximum, 0.f, 1.f) : 0.f;
	return Snapshot;
}

FSWOverlayRegenerationSnapshot USWAttributeOverlayWidgetController::MakeRegenerationSnapshot(const float RatePerSecond) const
{
	FSWOverlayRegenerationSnapshot Snapshot;
	Snapshot.RatePerSecond = RatePerSecond;
	return Snapshot;
}

void USWAttributeOverlayWidgetController::HandleHealthChanged(const FOnAttributeChangeData& ChangeData) { (void)ChangeData; BroadcastHealth(); }
void USWAttributeOverlayWidgetController::HandleMaxHealthChanged(const FOnAttributeChangeData& ChangeData) { (void)ChangeData; BroadcastHealth(); }
void USWAttributeOverlayWidgetController::HandleManaChanged(const FOnAttributeChangeData& ChangeData) { (void)ChangeData; BroadcastMana(); }
void USWAttributeOverlayWidgetController::HandleMaxManaChanged(const FOnAttributeChangeData& ChangeData) { (void)ChangeData; BroadcastMana(); }
void USWAttributeOverlayWidgetController::HandleStaminaChanged(const FOnAttributeChangeData& ChangeData) { (void)ChangeData; BroadcastStamina(); }
void USWAttributeOverlayWidgetController::HandleMaxStaminaChanged(const FOnAttributeChangeData& ChangeData) { (void)ChangeData; BroadcastStamina(); }
void USWAttributeOverlayWidgetController::HandleHealthRegenerationChanged(const FOnAttributeChangeData& ChangeData) { (void)ChangeData; BroadcastHealthRegeneration(); }
void USWAttributeOverlayWidgetController::HandleManaRegenerationChanged(const FOnAttributeChangeData& ChangeData) { (void)ChangeData; BroadcastManaRegeneration(); }
void USWAttributeOverlayWidgetController::HandleStaminaRegenerationChanged(const FOnAttributeChangeData& ChangeData) { (void)ChangeData; BroadcastStaminaRegeneration(); }
