// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManagerTypes.h"
#include "GameplayTagContainer.h"
#include "GameFramework/PlayerController.h"
#include "UI/DamageNumber/SWDamageNumberTypes.h"
#include "Shop/SWShopTypes.h"
#include "SWPlayerController.generated.h"

class USWInputConfig;

/**
 * ScifiWorlds 的每名玩家控制边界。
 *
 * Controller 仅在服务器与所属客户端存在；M04 起由其持有唯一输入配置，后续再按各自系统加入
 * 本地 UI 与客户端请求职责。
 */
UCLASS()
class POLYGONSCIFIWORLDS_API ASWPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	//~ Begin APlayerController interface
	virtual void BeginPlay() override;
	virtual void BeginPlayingState() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnRep_PlayerState() override;
	virtual void PostProcessInput(const float DeltaTime, const bool bGamePaused) override;
	//~ End APlayerController interface

	/** 返回唯一输入配置；Pawn 只能读取，不能持有第二份配置。 */
	UFUNCTION(BlueprintPure, Category = "Input")
	const USWInputConfig* GetInputConfig() const { return InputConfig; }

	/**
	 * 仅服务器调用：将已结算的伤害数字发送给本 Controller 的所属客户端。
	 * 这是高频、可丢失的纯表现事件，不能承载或修改任何权威战斗状态。
	 */
	UFUNCTION(Client, Unreliable)
	void ClientShowDamageNumber(const FSWDamageNumberPayload& Payload);

	/** 仅由所属客户端输入调用：请求服务器升级指定的固定主动技能槽位。 */
	void RequestActiveAbilityUpgrade(FGameplayTag InputTag);

	/** 所属客户端的商店购买意图；只上传稳定物品 Id，不上传价格、属性或槽位。 */
	UFUNCTION(BlueprintCallable, Category = "Shop")
	void RequestPurchaseItem(const FPrimaryAssetId& ItemDefinitionId);

	/** 所属客户端的商店出售意图；只上传已经拥有的固定装备槽索引。 */
	UFUNCTION(BlueprintCallable, Category = "Shop")
	void RequestSellEquipmentSlot(int32 SlotIndex);

	/** 仅本地客户端表现入口；由蓝图创建并驱动 WBP_DamageNumber。 */
	UFUNCTION(BlueprintImplementableEvent, Category = "UI|Damage Number", meta = (DisplayName = "显示伤害数字"))
	void BP_ShowDamageNumber(const FSWDamageNumberPayload& Payload);

	/** 服务器拒绝交易时仅向所属客户端发送表现原因；金币和装备栏仍以复制状态为准。 */
	UFUNCTION(BlueprintImplementableEvent, Category = "UI|Shop", meta = (DisplayName = "商店交易失败"))
	void BP_OnShopTransactionFailed(ESWShopTransactionFailure Failure);

protected:
	/** 所属客户端的升级意图 RPC；服务器只接受既有 Skill1/Skill2/Skill3 输入 Tag。 */
	UFUNCTION(Server, Reliable)
	void ServerRequestActiveAbilityUpgrade(FGameplayTag InputTag);

	/** 服务器侧执行升级事务的共享入口，供本地服务器和 RPC 实现复用。 */
	void ProcessActiveAbilityUpgradeRequestAuthority(FGameplayTag InputTag);

	UFUNCTION(Server, Reliable)
	void ServerRequestPurchaseItem(FPrimaryAssetId ItemDefinitionId);

	UFUNCTION(Server, Reliable)
	void ServerRequestSellEquipmentSlot(int32 SlotIndex);

	UFUNCTION(Client, Reliable)
	void ClientShopTransactionFailed(ESWShopTransactionFailure Failure);

	void ProcessPurchaseRequestAuthority(const FPrimaryAssetId& ItemDefinitionId);
	void ProcessSellRequestAuthority(int32 SlotIndex);

	/** 由 PlayerController 蓝图默认值指定的唯一输入数据资产。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USWInputConfig> InputConfig = nullptr;

	/** 仅本地 Controller 的 IMC 生命周期入口；重生 Pawn 不会重复添加映射。 */
	void ApplyGameplayMappingContext();
	void RemoveGameplayMappingContext();
	void RefreshOverlayWidgetControllers();
};
