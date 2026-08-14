// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManagerTypes.h"
#include "UI/WidgetController/SWWidgetController.h"
#include "SWShopWidgetController.generated.h"

class UTexture2D;

/** 商店商品格在 UMG 中的只读快照。购买可用性只是表现预测，最终交易仍由服务器重新校验。 */
USTRUCT(BlueprintType)
struct FSWShopItemSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Shop")
	FPrimaryAssetId ItemDefinitionId;

	UPROPERTY(BlueprintReadOnly, Category = "Shop")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Shop")
	FText Description;

	UPROPERTY(BlueprintReadOnly, Category = "Shop")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(BlueprintReadOnly, Category = "Shop")
	int32 PurchasePrice = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Shop")
	int32 OwnedCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Shop")
	int32 MaxOwnedCount = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Shop")
	bool bHasEnoughGold = false;

	UPROPERTY(BlueprintReadOnly, Category = "Shop")
	bool bCanPurchase = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSWOnShopItemStateChanged, const FSWShopItemSnapshot&, Item);
// 与 PlayerState 的原生委托同名会在全局命名空间冲突；此类型仅供 UMG 绑定。
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSWOnShopTradeAccessStateChanged, bool, bCanTrade);

/** 本地商店 UI 的只读数据适配器：目录来自 GameState，私有金币/装备栏/权限来自本地 PlayerState。 */
UCLASS(BlueprintType, Blueprintable)
class POLYGONSCIFIWORLDS_API USWShopWidgetController : public USWWidgetController
{
	GENERATED_BODY()

public:
	virtual void BroadcastInitialValues() override;
	virtual void BindCallbacksToDependencies() override;
	virtual void BeginDestroy() override;

	UFUNCTION(BlueprintCallable, Category = "Shop")
	void RefreshShopState();

	/** 供任意固定位置的商品 Widget 查询自身动态状态；未知或非目录商品会返回空快照。 */
	UFUNCTION(BlueprintPure, Category = "Shop")
	FSWShopItemSnapshot GetItemSnapshot(const FPrimaryAssetId& ItemDefinitionId) const;

	UPROPERTY(BlueprintAssignable, Category = "Shop")
	FSWOnShopItemStateChanged OnShopItemStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Shop")
	FSWOnShopTradeAccessStateChanged OnShopTradeAccessChanged;

private:
	void UnbindCallbacks();
	/** 原生金币委托携带数值；此处只负责转发到统一的快照刷新。 */
	void HandleGoldChanged(int32 NewGold);
	/** 原生交易权限委托携带布尔值；此处只负责转发到统一的快照刷新。 */
	void HandleTradeAccessChanged(bool bCanTrade);
	void HandleShopStateChanged();
	FSWShopItemSnapshot BuildItemSnapshot(const class USWEquipmentItemDefinition* ItemDefinition) const;

	FDelegateHandle GoldChangedHandle;
	FDelegateHandle EquipmentSlotsChangedHandle;
	FDelegateHandle TradeAccessChangedHandle;
	bool bCallbacksBound = false;
};
