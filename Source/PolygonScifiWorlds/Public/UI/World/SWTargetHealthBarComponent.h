// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "SWTargetHealthBarComponent.generated.h"

class UAbilitySystemComponent;
struct FOnAttributeChangeData;

/**
 * 挂在可战斗角色头顶的本地血条组件。
 *
 * 它不复制显示状态：服务器只向实际攻击者的 Controller 发送一次可丢失的表现通知，
 * 该客户端随后用目标已复制的 GAS Health/MaxHealth 本地刷新，并在超时后隐藏。
 */
UCLASS(ClassGroup = (UI), meta = (BlueprintSpawnableComponent))
class POLYGONSCIFIWORLDS_API USWTargetHealthBarComponent : public UWidgetComponent
{
	GENERATED_BODY()

public:
	USWTargetHealthBarComponent();

	/**
	 * 仅攻击者所属客户端调用。只有目标处于本地攻击者的血条显示范围时，
	 * 才记录本次受击并显示血条。
	 */
	void ShowForLocalAttacker();

	/**
	 * 仅本地玩家范围球的 Overlap 事件调用。该状态不复制；离开范围会立即取消本次受击显示资格。
	 * InSizeScale 用于离散距离档位，避免以 Tick 连续计算屏幕血条尺寸。
	 */
	void SetLocalAttackerDisplayRangeState(bool bInDisplayRange, float InSizeScale = 1.f);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 最后一次由本地玩家造成实际伤害后，血条保持可见的秒数。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Target Health Bar", meta = (ClampMin = "0.1", Units = "s"))
	float VisibleDurationSeconds = 4.f;

private:
	void BindToOwnerAbilitySystem();
	void UnbindFromOwnerAbilitySystem();
	void RefreshWidgetHealth() const;
	void HideForLocalAttacker();
	void RefreshLocalVisibility();
	void HandleHealthChanged(const FOnAttributeChangeData& ChangeData);
	void HandleMaxHealthChanged(const FOnAttributeChangeData& ChangeData);

	TWeakObjectPtr<UAbilitySystemComponent> BoundAbilitySystemComponent;
	FDelegateHandle HealthChangedHandle;
	FDelegateHandle MaxHealthChangedHandle;
	FTimerHandle HideTimerHandle;
	/** 仅当前客户端记录的“曾在范围内受到本地攻击”状态。 */
	bool bRecentlyDamagedByLocalAttacker = false;
	/** 仅当前客户端由本地玩家范围球维护的状态。 */
	bool bInsideLocalAttackerDisplayRange = false;
	/** BeginPlay 缓存蓝图最终配置的基础尺寸，距离档位只相对它缩放。 */
	FVector2D BaseDrawSize = FVector2D(180.f, 24.f);
};
