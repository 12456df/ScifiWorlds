// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/WidgetController/SWWidgetController.h"
#include "SWAttributeOverlayWidgetController.generated.h"

class UAbilitySystemComponent;
struct FOnAttributeChangeData;

/** 一项当前资源及其上限的 UI 快照。 */
USTRUCT(BlueprintType)
struct FSWOverlayResourceSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Overlay")
	float Current = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Overlay")
	float Maximum = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Overlay")
	float Percent = 0.f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSWOnOverlayResourceChanged, const FSWOverlayResourceSnapshot&, Snapshot);

/** 一项每秒恢复数值的 UI 快照。 */
USTRUCT(BlueprintType)
struct FSWOverlayRegenerationSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Overlay")
	float RatePerSecond = 0.f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSWOnOverlayRegenerationChanged, const FSWOverlayRegenerationSnapshot&, Snapshot);

/**
 * 订阅 PlayerState ASC 上的生命、蓝量与体力；只读取复制后的属性快照。
 */
UCLASS(BlueprintType, Blueprintable)
class POLYGONSCIFIWORLDS_API USWAttributeOverlayWidgetController : public USWWidgetController
{
	GENERATED_BODY()

public:
	virtual void BroadcastInitialValues() override;
	virtual void BindCallbacksToDependencies() override;
	virtual void BeginDestroy() override;

	UPROPERTY(BlueprintAssignable, Category = "Overlay|Attributes")
	FSWOnOverlayResourceChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Overlay|Attributes")
	FSWOnOverlayResourceChanged OnManaChanged;

	UPROPERTY(BlueprintAssignable, Category = "Overlay|Attributes")
	FSWOnOverlayResourceChanged OnStaminaChanged;

	UPROPERTY(BlueprintAssignable, Category = "Overlay|Attributes")
	FSWOnOverlayRegenerationChanged OnHealthRegenerationChanged;

	UPROPERTY(BlueprintAssignable, Category = "Overlay|Attributes")
	FSWOnOverlayRegenerationChanged OnManaRegenerationChanged;

	UPROPERTY(BlueprintAssignable, Category = "Overlay|Attributes")
	FSWOnOverlayRegenerationChanged OnStaminaRegenerationChanged;

private:
	void BroadcastHealth();
	void BroadcastMana();
	void BroadcastStamina();
	void BroadcastHealthRegeneration();
	void BroadcastManaRegeneration();
	void BroadcastStaminaRegeneration();
	void UnbindCallbacks();
	FSWOverlayResourceSnapshot MakeResourceSnapshot(float Current, float Maximum) const;
	FSWOverlayRegenerationSnapshot MakeRegenerationSnapshot(float RatePerSecond) const;
	void HandleHealthChanged(const FOnAttributeChangeData& ChangeData);
	void HandleMaxHealthChanged(const FOnAttributeChangeData& ChangeData);
	void HandleManaChanged(const FOnAttributeChangeData& ChangeData);
	void HandleMaxManaChanged(const FOnAttributeChangeData& ChangeData);
	void HandleStaminaChanged(const FOnAttributeChangeData& ChangeData);
	void HandleMaxStaminaChanged(const FOnAttributeChangeData& ChangeData);
	void HandleHealthRegenerationChanged(const FOnAttributeChangeData& ChangeData);
	void HandleManaRegenerationChanged(const FOnAttributeChangeData& ChangeData);
	void HandleStaminaRegenerationChanged(const FOnAttributeChangeData& ChangeData);

	TWeakObjectPtr<UAbilitySystemComponent> BoundAbilitySystemComponent;
	FDelegateHandle HealthChangedHandle;
	FDelegateHandle MaxHealthChangedHandle;
	FDelegateHandle ManaChangedHandle;
	FDelegateHandle MaxManaChangedHandle;
	FDelegateHandle StaminaChangedHandle;
	FDelegateHandle MaxStaminaChangedHandle;
	FDelegateHandle HealthRegenerationChangedHandle;
	FDelegateHandle ManaRegenerationChangedHandle;
	FDelegateHandle StaminaRegenerationChangedHandle;
};
