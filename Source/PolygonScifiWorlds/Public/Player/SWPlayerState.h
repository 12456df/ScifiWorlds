// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "Equipment/SWEquipmentTypes.h"
#include "Interaction/SWPlayerProgressionInterface.h"
#include "Interaction/SWTeamInterface.h"
#include "Team/SWTeamTypes.h"
#include "SWPlayerState.generated.h"

class ASWGameMode;
class UAbilitySystemComponent;
class USWAbilitySystemComponent;
class USWAttributeSet;
class USWEconomyData;
class USWEquipmentItemDefinition;
class ASWShopZone;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSWOnTeamIdChanged, ESWTeamId, PreviousTeamId, ESWTeamId, NewTeamId);

// UI/gameplay listeners for replicated PlayerState integer values. Broadcast on both
// the authority (setter) and receiving client (OnRep) so observers stay in sync.
DECLARE_MULTICAST_DELEGATE_OneParam(FSWOnProgressionValueChanged, int32 /*NewValue*/);

/** 装备栏快照变更时广播；服务器写入与所属客户端 OnRep 均会触发。 */
DECLARE_MULTICAST_DELEGATE(FSWOnEquipmentSlotsChanged);
DECLARE_MULTICAST_DELEGATE_OneParam(FSWOnShopTradeAccessChanged, bool /*bCanTrade*/);

/**
 * Server-authoritative owner of the player's ability system and persistent progression.
 *
 * Contract defined in Docs/Systems/M03_GASCoreFramework.md and ADR-0002. The ASC and
 * attribute set live here so they survive Pawn respawns; the current Pawn only acts as
 * the ASC avatar. The server is the sole writer of Level, Experience and AbilityPoints.
 */
UCLASS()
class POLYGONSCIFIWORLDS_API ASWPlayerState : public APlayerState, public IAbilitySystemInterface, public ISWPlayerProgressionInterface, public ISWTeamInterface
{
	GENERATED_BODY()

public:
	/** M09 固定装备栏容量；运行时不支持扩容。 */
	static constexpr int32 EquipmentSlotCount = 6;

	ASWPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//~ Begin IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	//~ End IAbilitySystemInterface

	USWAttributeSet* GetAttributeSet() const { return AttributeSet; }

	/** 返回服务器分配的队伍；所有端均可安全读取。 */
	UFUNCTION(BlueprintPure, Category = "Team")
	virtual ESWTeamId GetTeamId() const override { return TeamId; }

	/** 队伍变化时在服务器与客户端触发，供蓝图表现层订阅。 */
	UPROPERTY(BlueprintAssignable, Category = "Team")
	FSWOnTeamIdChanged OnTeamIdChanged;

	// --- Progression readers (any client may read) ---

	virtual int32 GetPlayerLevel() const override { return Level; }
	virtual int32 GetExperience() const override { return Experience; }
	virtual int32 GetAbilityPoints() const override { return AbilityPoints; }
	virtual int32 FindLevelForExperience(int32 TotalExperience) const override;

	/** 返回本玩家的对局金币。服务器和所属客户端可读取；其他客户端不接收该值。 */
	UFUNCTION(BlueprintPure, Category = "Economy")
	int32 GetGold() const { return Gold; }

	/** 是否位于至少一个有效商店区域且当前 Pawn 存活；仅复制给所属客户端，用于 UI 只读表现。 */
	UFUNCTION(BlueprintPure, Category = "Shop")
	bool CanTradeAtShop() const { return bCanTradeAtShop; }

	// --- Server-authoritative progression writers ---
	// All mutators no-op off the authority; clients only request intent elsewhere.

	/** 仅服务器调用：增加非负经验，并根据本局 ProgressionData 结算跨级与技能点。 */
	virtual void AddExperienceAuthority(int32 DeltaExperience) override;

	/** 仅服务器调用：直接设置等级；有 ProgressionData 时同时限制到其最大等级。 */
	void SetLevel(int32 NewLevel);

	/** Grants ability points (non-negative delta). */
	void GrantAbilityPoints(int32 DeltaPoints);

	/** Consumes a single ability point if available. Returns true when a point was spent. */
	bool SpendAbilityPoint();

	// --- Server-authoritative economy writers ---
	// These APIs are intentionally not exposed to Blueprint. Reward and transaction
	// systems may submit validated values from C++; clients cannot write Gold directly.

	/** 仅服务器调用：增加正数金币；实际写入统一收敛到私有金币变更入口。 */
	void GrantGoldAuthority(int32 DeltaGold);

	/** 仅服务器调用：金币足够时扣除非负花费；失败时不改变余额。 */
	bool TrySpendGoldAuthority(int32 GoldCost);

	/** 返回指定物品当前占用的装备槽数量。 */
	int32 GetOwnedEquipmentCount(const FPrimaryAssetId& ItemDefinitionId) const;

	/** 服务器原子购买：应用 GE、扣金币并写入空槽；失败时不提交任何持久状态。 */
	bool TryPurchaseEquipmentAuthority(const USWEquipmentItemDefinition* ItemDefinition);

	/** 服务器原子出售：按槽移除 GE、清空槽位并返还实付价格比例。 */
	bool TrySellEquipmentSlotAuthority(int32 SlotIndex);

	/** 仅由服务器 ShopZone Overlap 生命周期调用；多个区域使用集合而非单个布尔值。 */
	void AddShopZoneAuthority(ASWShopZone* ShopZone);
	void RemoveShopZoneAuthority(ASWShopZone* ShopZone);
	/** 仅由死亡/Pawn 销毁流程调用：死亡玩家不得保留交易资格。 */
	void ClearShopTradeAccessAuthority();

	// --- Server-authoritative equipment state ---

	/**
	 * 在 ASC 已完成 Owner/Avatar 绑定后调用。
	 * 首次调用会将蓝图配置的初始装备写入固定六槽，之后仅补齐缺失的派生 GE，
	 * 因而死亡重生时不会重复叠加装备效果。
	 */
	void EnsureEquipmentEffectsAppliedAuthority();

	/**
	 * 仅服务器调用：为指定已占用槽位原子应用对应物品的全部 Infinite GE。
	 * 调用成功后保存每一个 Active GE Handle；任一 GE 失败会回滚本次槽位的全部效果。
	 */
	bool ApplyEquipmentEffectsForSlotAuthority(int32 SlotIndex, const USWEquipmentItemDefinition* ItemDefinition);

	/** 仅服务器调用：移除指定槽位此前派生的全部 GE；重复调用安全。 */
	bool RemoveEquipmentEffectsForSlotAuthority(int32 SlotIndex);

	/** 仅服务器调用：清理全部派生 GE 后，依据当前六槽快照幂等重建。 */
	bool RebuildEquipmentEffectsAuthority();

	/** 验证装备定义是否可作为常驻装备效果应用；失败原因供服务器诊断使用。 */
	bool ValidateEquipmentDefinition(const USWEquipmentItemDefinition* ItemDefinition, FString& OutFailureReason) const;

	const TArray<FSWEquipmentSlot>& GetEquipmentSlots() const { return EquipmentSlots; }

	// --- Change delegates ---

	FSWOnProgressionValueChanged OnLevelChanged;
	FSWOnProgressionValueChanged OnExperienceChanged;
	FSWOnProgressionValueChanged OnAbilityPointsChanged;
	FSWOnProgressionValueChanged OnGoldChanged;
	FSWOnEquipmentSlotsChanged OnEquipmentSlotsChanged;
	FSWOnShopTradeAccessChanged OnShopTradeAccessChanged;

protected:
	/** 只有服务器 GameMode 能写入队伍归属。 */
	friend class ASWGameMode;

	void SetTeamId(ESWTeamId NewTeamId);
	bool IsValidTeamId(ESWTeamId TeamIdToValidate) const;

	/** 返回由已复制 GameState 暴露的本局成长配置；只在服务器写入路径中消费。 */
	const class USWProgressionData* GetProgressionData() const;

	UPROPERTY(VisibleAnywhere, Category = "SW|GAS")
	TObjectPtr<USWAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, Category = "SW|GAS")
	TObjectPtr<USWAttributeSet> AttributeSet;

	/**
	 * 仅作为出生时的内容配置来源。首次有效绑定 ASC 后会被转换为 EquipmentSlots，
	 * 后续交易不会修改它，死亡重生也不会重新读取它。
	 */
	UPROPERTY(EditDefaultsOnly, Category = "SW|Equipment", meta = (TitleProperty = "DisplayName"))
	TArray<TObjectPtr<USWEquipmentItemDefinition>> StartingEquipmentDefinitions;

	/** 六个装备槽的运行时真值；只复制给所属客户端，死亡和 Pawn 重生不会清空。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SW|Equipment", ReplicatedUsing = OnRep_EquipmentSlots, meta = (AllowPrivateAccess = "true"))
	TArray<FSWEquipmentSlot> EquipmentSlots;

	/** 仅服务端的派生状态：每个槽位所应用的 GE Handle，用于精确移除和幂等检查。 */
	TMap<int32, TArray<FActiveGameplayEffectHandle>> AppliedEquipmentEffectHandles;

	/** 防止正常重生流程重复把默认配置写回装备栏。 */
	bool bStartingEquipmentInitialized = false;

	// --- Replicated progression state (server writes, all clients read) ---

	UPROPERTY(VisibleAnywhere, Category = "SW|Progression", ReplicatedUsing = OnRep_Level)
	int32 Level = 1;

	UPROPERTY(VisibleAnywhere, Category = "SW|Progression", ReplicatedUsing = OnRep_Experience)
	int32 Experience = 0;

	UPROPERTY(VisibleAnywhere, Category = "SW|Progression", ReplicatedUsing = OnRep_AbilityPoints)
	int32 AbilityPoints = 0;

	/** 本局金币真值；仅服务器写入，仅复制给所属客户端，死亡与重生不重置。 */
	UPROPERTY(VisibleAnywhere, Category = "SW|Economy", ReplicatedUsing = OnRep_Gold)
	int32 Gold = 0;

	/** 当前拥有者的服务器交易资格；客户端仅用于禁用按钮，服务器交易时仍重新验证。 */
	UPROPERTY(VisibleAnywhere, Category = "SW|Shop", ReplicatedUsing = OnRep_CanTradeAtShop)
	bool bCanTradeAtShop = false;

	/** 队伍初始为 None，仅由 GameMode 在加入流程中写入。 */
	UPROPERTY(ReplicatedUsing = OnRep_TeamId, BlueprintReadOnly, Category = "Team", meta = (AllowPrivateAccess = "true"))
	ESWTeamId TeamId = ESWTeamId::None;

	UFUNCTION()
	void OnRep_TeamId(ESWTeamId PreviousTeamId);

	UFUNCTION()
	void OnRep_Level(int32 OldLevel);

	UFUNCTION()
	void OnRep_Experience(int32 OldExperience);

	UFUNCTION()
	void OnRep_AbilityPoints(int32 OldAbilityPoints);

	UFUNCTION()
	void OnRep_Gold(int32 OldGold);

	UFUNCTION()
	void OnRep_CanTradeAtShop();

	UFUNCTION()
	void OnRep_EquipmentSlots();

	void InitializeStartingEquipmentAuthority();

	/** 仅由服务器 GameMode 在首次有效登录时调用；重复调用不会重复授予初始金币。 */
	void InitializeEconomyAuthority();

	/** 仅由服务器 GameMode 的一秒结算调用；支持小数收入累积，金币真值仍保持整数。 */
	void GrantPassiveGoldIncomeAuthority(float GoldPerSecond);

	/** 从已复制/发布的本局经济配置读取金币上限；缺失配置时维持 int32 安全上限。 */
	int32 GetMaximumGold() const;
	const USWEconomyData* GetEconomyData() const;

	/**
	 * 金币唯一写入入口：所有加减必须经过此处完成 Clamp 与服务器 UI 通知。
	 * 负数余额会被拒绝；状态未变化时不产生伪通知。
	 */
	bool ApplyGoldDeltaAuthority(int32 DeltaGold);

	/** 将服务器写入与所属客户端 OnRep 收敛为同一份 UI 通知。 */
	void BroadcastGoldChanged();
	void RefreshShopTradeAccessAuthority();
	const USWEquipmentItemDefinition* ResolveEquipmentDefinitionAuthority(const FPrimaryAssetId& ItemDefinitionId) const;

	/** 防止 PostLogin、重生或辅助流程重复授予初始金币。仅服务器保存。 */
	bool bEconomyInitialized = false;

	/** 被动收入的小数残余；仅服务器保存，避免低于 1 金/秒的等级曲线被截断丢失。 */
	double PassiveGoldFraction = 0.0;

	/** 只存在服务器：有效 ShopZone 集合是交易资格的权威来源。 */
	TSet<TWeakObjectPtr<ASWShopZone>> ActiveShopZones;
};
