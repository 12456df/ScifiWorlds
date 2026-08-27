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
class UMaterialInterface;
class UMeshComponent;

/** 本地高亮系统接管前的 Mesh Overlay 状态；退出或不再是敌人时必须原样恢复。 */
struct FSWLocalEnemyHighlightComponentState
{
	TWeakObjectPtr<UMaterialInterface> OverlayMaterial;
	float OverlayMaterialMaxDrawDistance = 0.f;
};

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
	virtual void PlayerTick(float DeltaTime) override;
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

	/**
	 * 仅服务器在本 Controller 的 Pawn 实际造成伤害后调用。
	 * 此为可丢失的本地表现事件；血条可见性不复制，也不承载战斗真值。
	 */
	UFUNCTION(Client, Unreliable)
	void ClientShowDamagedTargetHealthBar(AActor* TargetActor);

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

	/**
	 * 仅所属客户端的敌方 Mesh Overlay 高亮开关。不复制、不写入玩法状态；所有材质选择由 Controller 蓝图配置。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Enemy Highlight", meta = (AllowPrivateAccess = "true"))
	bool bEnableEnemyHighlights = true;

	/** 常规敌方单位使用的半透明 Overlay 材质实例。缺失时高亮系统安全停用。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Enemy Highlight", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMaterialInterface> EnemyHighlightOverlayMaterial = nullptr;

	/** 准星首个命中敌人使用的 Overlay 材质实例。未配置时回退至常规敌方材质。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Enemy Highlight", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMaterialInterface> FocusedEnemyHighlightOverlayMaterial = nullptr;

	/** 高亮的最大显示距离（cm）。同时限制本地 CPU 投影与 Mesh Overlay 渲染。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Enemy Highlight", meta = (AllowPrivateAccess = "true", ClampMin = "1.0", Units = "cm"))
	float EnemyHighlightMaxDisplayDistance = 2500.f;

	/** 高亮刷新只服务本地可视反馈；20Hz 足以跟随准星，同时避免每帧遍历所有战斗单位。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Enemy Highlight", meta = (AllowPrivateAccess = "true", ClampMin = "0.016"))
	float EnemyHighlightRefreshIntervalSeconds = 0.05f;

	/** Pawn/Weapon 尚未复制完成时使用的保守射线距离。正常情况下优先读取当前武器的 MaxAimDistance。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Enemy Highlight", meta = (AllowPrivateAccess = "true", ClampMin = "1.0"))
	float FallbackEnemyHighlightTraceDistance = 20000.f;

	/** 仅本地 Controller 的 IMC 生命周期入口；重生 Pawn 不会重复添加映射。 */
	void ApplyGameplayMappingContext();
	void RemoveGameplayMappingContext();
	void RefreshOverlayWidgetControllers();

	/** 只在拥有此视口的 Controller 上运行的渲染投影；不发送 RPC，也不写服务器状态。 */
	void UpdateEnemyHighlightPresentation();
	void ClearEnemyHighlightPresentation();
	bool IsHostileHighlightTarget(AActor* CandidateActor) const;
	AActor* FindCrosshairEnemyHighlightTarget() const;
	float GetEnemyHighlightTraceDistance() const;
	bool IsWithinEnemyHighlightDistance(const AActor& CandidateActor, const FVector& ViewLocation) const;
	void ApplyLocalHighlightToActor(AActor& TargetActor, UMaterialInterface& OverlayMaterial, TSet<TWeakObjectPtr<UMeshComponent>>& OutTouchedComponents);

	/** 仅保存本地渲染覆写前的值，确保重生、切图和关闭 Controller 后不污染其他表现系统。 */
	TMap<TWeakObjectPtr<UMeshComponent>, FSWLocalEnemyHighlightComponentState> OriginalEnemyHighlightComponentStates;
	float EnemyHighlightRefreshAccumulator = 0.f;
};
