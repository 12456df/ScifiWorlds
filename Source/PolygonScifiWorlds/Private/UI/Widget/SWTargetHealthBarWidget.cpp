// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "UI/Widget/SWTargetHealthBarWidget.h"

void USWTargetHealthBarWidget::SetHealthSnapshot(const float InCurrentHealth, const float InMaximumHealth)
{
	CurrentHealth = FMath::Max(0.f, InCurrentHealth);
	MaximumHealth = FMath::Max(0.f, InMaximumHealth);
	HealthPercent = MaximumHealth > 0.f ? FMath::Clamp(CurrentHealth / MaximumHealth, 0.f, 1.f) : 0.f;
	BP_OnHealthSnapshotChanged(CurrentHealth, MaximumHealth, HealthPercent);
}
