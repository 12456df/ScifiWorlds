// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManagerTypes.h"
#include "UI/WidgetController/SWWidgetController.h"
#include "SWEquipmentOverlayWidgetController.generated.h"

class UTexture2D;

/** 单个固定装备槽位在 UMG 中所需的只读显示数据。 */
USTRUCT(BlueprintType)
struct FSWEquipmentSlotSnapshot
{
	GENERATED_BODY()

	/** 固定槽位身份，范围为 0..5；2×3 只是 UMG 的布局方式。 */
	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	int32 SlotIndex = INDEX_NONE;

	/** 空槽时无效；后续详情、出售意图均以该槽位索引为入口。 */
	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	FPrimaryAssetId ItemDefinitionId;

	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	bool bIsOccupied = false;

	UPROPERTY(BlueprintReadOnly, Category = "Equipment|Display")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Equipment|Display")
	FText Description;

	/** 仅供本地 UMG 按需加载；Dedicated Server 不会创建本控制器。 */
	UPROPERTY(BlueprintReadOnly, Category = "Equipment|Display")
	TSoftObjectPtr<UTexture2D> Icon;

	/** 当前用于详情预览的目录标价；并非未来出售退款所依据的实付价格。 */
	UPROPERTY(BlueprintReadOnly, Category = "Equipment|Economy")
	int32 PurchasePrice = 0;
};

/** 初次建立装备栏时一次性广播固定六个槽位的完整快照。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSWOnEquipmentBarInitialized, const TArray<FSWEquipmentSlotSnapshot>&, EquipmentSlots);

/** 装备栏发生变化时广播受影响的槽位快照。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSWOnEquipmentSlotChanged, const FSWEquipmentSlotSnapshot&, EquipmentSlot);

/**
 * 本地玩家装备栏的只读 UI 数据控制器。
 *
 * 该类只监听 PlayerState 的 OwnerOnly 装备栏复制结果，并解析静态装备定义；
 * 不负责交易、出售、输入模式或创建任何 Widget。
 */
UCLASS(BlueprintType, Blueprintable)
class POLYGONSCIFIWORLDS_API USWEquipmentOverlayWidgetController : public USWWidgetController
{
	GENERATED_BODY()

public:
	virtual void BroadcastInitialValues() override;
	virtual void BindCallbacksToDependencies() override;
	virtual void BeginDestroy() override;

	/** 蓝图完成委托绑定后可主动请求一次完整六槽快照，避免错过 HUD 创建阶段的首帧广播。 */
	UFUNCTION(BlueprintCallable, Category = "Overlay|Equipment")
	void RefreshEquipmentBar();

	UPROPERTY(BlueprintAssignable, Category = "Overlay|Equipment")
	FSWOnEquipmentBarInitialized OnEquipmentBarInitialized;

	UPROPERTY(BlueprintAssignable, Category = "Overlay|Equipment")
	FSWOnEquipmentSlotChanged OnEquipmentSlotChanged;

private:
	void UnbindCallbacks();
	void HandleEquipmentSlotsChanged();
	FSWEquipmentSlotSnapshot BuildEquipmentSlotSnapshot(int32 SlotIndex) const;

	FDelegateHandle EquipmentSlotsChangedHandle;
	bool bCallbacksBound = false;
};
