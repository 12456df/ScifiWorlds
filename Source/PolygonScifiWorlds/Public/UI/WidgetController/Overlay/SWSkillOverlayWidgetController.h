// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "GameplayTagContainer.h"
#include "UI/WidgetController/SWWidgetController.h"
#include "SWSkillOverlayWidgetController.generated.h"

class USWAbilitySystemComponent;
class UAbilitySystemComponent;
class USWActiveGameplayAbility;
class UTexture2D;
enum class ESWActivatableAbilitySpecChangeType : uint8;
struct FActiveGameplayEffect;
struct FGameplayAbilitySpec;
struct FGameplayEffectSpec;
struct FOnAttributeChangeData;

/** 单个固定主动技能槽位在 UI 中所需的只读快照。 */
USTRUCT(BlueprintType)
struct FSWSkillSlotSnapshot
{
	GENERATED_BODY()

	/** 固定槽位身份：Ability.Input.Skill1、Skill2 或 Skill3。 */
	UPROPERTY(BlueprintReadOnly, Category = "Skill")
	FGameplayTag InputTag;

	/** 槽位当前是否已授予一个可展示的主动技能。 */
	UPROPERTY(BlueprintReadOnly, Category = "Skill")
	bool bHasAssignedAbility = false;

	/** 技能的稳定身份 Tag，供升级 UI、提示和配置查询使用。 */
	UPROPERTY(BlueprintReadOnly, Category = "Skill")
	FGameplayTag AbilityIdTag;

	UPROPERTY(BlueprintReadOnly, Category = "Skill")
	int32 AbilityLevel = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Skill")
	int32 MaxAbilityLevel = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Skill")
	bool bUpgradeable = false;

	/** 当前拥有技能点、未达等级上限且技能未激活时为 true；UI 以此显示升级加号。 */
	UPROPERTY(BlueprintReadOnly, Category = "Skill")
	bool bCanUpgrade = false;

	UPROPERTY(BlueprintReadOnly, Category = "Skill")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Skill")
	TSoftObjectPtr<UTexture2D> Icon;

	/** 该技能用于查询自身冷却/充能状态的唯一 Gameplay Tag。 */
	UPROPERTY(BlueprintReadOnly, Category = "Skill|Cooldown")
	FGameplayTag CooldownTag;

	/** 当前可立即使用的充能数；无冷却技能始终等于 MaxCharges。 */
	UPROPERTY(BlueprintReadOnly, Category = "Skill|Cooldown")
	int32 CurrentCharges = 0;

	/** 当前等级和 AttributeSet 修正共同决定的最大充能数。 */
	UPROPERTY(BlueprintReadOnly, Category = "Skill|Cooldown")
	int32 MaxCharges = 0;

	/** 是否存在至少一层尚未恢复的充能冷却。 */
	UPROPERTY(BlueprintReadOnly, Category = "Skill|Cooldown")
	bool bIsCooldownActive = false;

	/** 下一层充能恢复还剩多少秒；没有冷却时为 0。 */
	UPROPERTY(BlueprintReadOnly, Category = "Skill|Cooldown")
	float NextChargeRemainingSeconds = 0.f;

	/** 下一层充能对应的完整冷却时长；供 UMG 计算遮罩百分比。 */
	UPROPERTY(BlueprintReadOnly, Category = "Skill|Cooldown")
	float NextChargeDurationSeconds = 0.f;

	/** 下一层充能的服务器世界时间终点；UMG 以 GameState 服务器时间驱动本地视觉倒计时。 */
	UPROPERTY(BlueprintReadOnly, Category = "Skill|Cooldown")
	float NextChargeEndServerTimeSeconds = 0.f;
};

/** 初次建立技能栏时一次广播固定三槽的快照。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSWOnSkillBarInitialized, const TArray<FSWSkillSlotSnapshot>&, SkillSlots);

/** 技能 Spec 变化时仅广播受影响的一个固定槽位。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSWOnSkillSlotChanged, const FSWSkillSlotSnapshot&, SkillSlot);

/**
 * 本地玩家主动技能栏的只读数据控制器。
 * 首次只初始化 Skill1/Skill2/Skill3 三个固定槽位；之后只推送发生变化的槽位。
 */
UCLASS(BlueprintType, Blueprintable)
class POLYGONSCIFIWORLDS_API USWSkillOverlayWidgetController : public USWWidgetController
{
	GENERATED_BODY()

public:
	virtual void BroadcastInitialValues() override;
	virtual void BindCallbacksToDependencies() override;
	virtual void BeginDestroy() override;

	/** 蓝图完成事件绑定后请求当前固定三槽快照，避免错过 HUD 创建阶段的首个广播。 */
	UFUNCTION(BlueprintCallable, Category = "Overlay|Skill")
	void RefreshSkillBar();

	UPROPERTY(BlueprintAssignable, Category = "Overlay|Skill")
	FSWOnSkillBarInitialized OnSkillBarInitialized;

	UPROPERTY(BlueprintAssignable, Category = "Overlay|Skill")
	FSWOnSkillSlotChanged OnSkillSlotChanged;

private:
	void UnbindCallbacks();
	void HandleActivatableAbilitySpecChanged(const FGameplayAbilitySpec& AbilitySpec, ESWActivatableAbilitySpecChangeType ChangeType);
	void HandleAbilitySpecDirtied(const FGameplayAbilitySpec& AbilitySpec);
	void HandleActiveGameplayEffectAdded(UAbilitySystemComponent* TargetAbilitySystemComponent, const FGameplayEffectSpec& AppliedEffectSpec, FActiveGameplayEffectHandle ActiveEffectHandle);
	void HandleActiveGameplayEffectRemoved(const FActiveGameplayEffect& ActiveEffect);
	void HandleCooldownEffectStackChanged(FActiveGameplayEffectHandle ActiveEffectHandle, int32 NewStackCount, int32 PreviousStackCount);
	void HandleAbilityChargeBonusChanged(const FOnAttributeChangeData& ChangeData);
	void HandleAbilityPointsChanged(int32 NewAbilityPoints);
	bool BuildSkillSlotSnapshot(const FGameplayAbilitySpec& AbilitySpec, FSWSkillSlotSnapshot& OutSnapshot) const;
	FSWSkillSlotSnapshot MakeEmptySkillSlotSnapshot(FGameplayTag InputTag) const;
	bool GetCooldownTagFromEffectSpec(const FGameplayEffectSpec& EffectSpec, FGameplayTag& OutCooldownTag) const;
	void TrackCooldownEffect(FActiveGameplayEffectHandle ActiveEffectHandle, const FGameplayTag& CooldownTag);
	void RefreshSkillSlotForCooldownTag(const FGameplayTag& CooldownTag);
	void PopulateCooldownSnapshot(const FGameplayAbilitySpec& AbilitySpec, const USWActiveGameplayAbility& ActiveAbility, FSWSkillSlotSnapshot& InOutSnapshot) const;

	TWeakObjectPtr<USWAbilitySystemComponent> BoundAbilitySystemComponent;
	FDelegateHandle ActivatableAbilitySpecChangedHandle;
	FDelegateHandle AbilitySpecDirtiedHandle;
	FDelegateHandle ActiveGameplayEffectAddedHandle;
	FDelegateHandle ActiveGameplayEffectRemovedHandle;
	FDelegateHandle AbilityChargeBonusChangedHandle;
	FDelegateHandle AbilityPointsChangedHandle;
	TMap<FActiveGameplayEffectHandle, FGameplayTag> TrackedCooldownEffects;
	TMap<FActiveGameplayEffectHandle, FDelegateHandle> CooldownStackChangeHandles;
};
