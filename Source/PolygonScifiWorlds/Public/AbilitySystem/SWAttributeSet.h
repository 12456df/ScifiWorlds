// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "SWAttributeSet.generated.h"

struct FGameplayEffectModCallbackData;

// Standard GAS accessor bundle: property getter, value getter/setter and initter.
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/**
 * Server-authoritative combat and ability-modifier attributes for ScifiWorlds.
 *
 * The attribute contract is defined in Docs/Systems/M03_GASCoreFramework.md.
 * This set only declares attributes, replicates them and maintains value
 * invariants (current resources clamped to their maximums). Balance numbers,
 * caps and damage formulas are data-driven or belong to later modules.
 */
UCLASS()
class POLYGONSCIFIWORLDS_API USWAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	USWAttributeSet();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	// --- Resources ---

	UPROPERTY(BlueprintReadOnly, Category = "SW|Attributes|Resources", ReplicatedUsing = OnRep_Health)
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(USWAttributeSet, Health);

	UPROPERTY(BlueprintReadOnly, Category = "SW|Attributes|Resources", ReplicatedUsing = OnRep_MaxHealth)
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(USWAttributeSet, MaxHealth);

	UPROPERTY(BlueprintReadOnly, Category = "SW|Attributes|Resources", ReplicatedUsing = OnRep_Mana)
	FGameplayAttributeData Mana;
	ATTRIBUTE_ACCESSORS(USWAttributeSet, Mana);

	UPROPERTY(BlueprintReadOnly, Category = "SW|Attributes|Resources", ReplicatedUsing = OnRep_MaxMana)
	FGameplayAttributeData MaxMana;
	ATTRIBUTE_ACCESSORS(USWAttributeSet, MaxMana);

	/** 疾跑等移动行为消耗的当前体力值。 */
	UPROPERTY(BlueprintReadOnly, Category = "SW|Attributes|Resources", ReplicatedUsing = OnRep_Stamina)
	FGameplayAttributeData Stamina;
	ATTRIBUTE_ACCESSORS(USWAttributeSet, Stamina);

	/** 当前体力值的上限。 */
	UPROPERTY(BlueprintReadOnly, Category = "SW|Attributes|Resources", ReplicatedUsing = OnRep_MaxStamina)
	FGameplayAttributeData MaxStamina;
	ATTRIBUTE_ACCESSORS(USWAttributeSet, MaxStamina);

	/** 移动速度乘数；1 表示基础速度，后续 GE 可通过聚合修改此值。 */
	UPROPERTY(BlueprintReadOnly, Category = "SW|Attributes|Movement", ReplicatedUsing = OnRep_MovementSpeedMultiplier)
	FGameplayAttributeData MovementSpeedMultiplier;
	ATTRIBUTE_ACCESSORS(USWAttributeSet, MovementSpeedMultiplier);

	// --- Offense ---

	UPROPERTY(BlueprintReadOnly, Category = "SW|Attributes|Offense", ReplicatedUsing = OnRep_AttackPower)
	FGameplayAttributeData AttackPower;
	ATTRIBUTE_ACCESSORS(USWAttributeSet, AttackPower);

	UPROPERTY(BlueprintReadOnly, Category = "SW|Attributes|Offense", ReplicatedUsing = OnRep_SpellPower)
	FGameplayAttributeData SpellPower;
	ATTRIBUTE_ACCESSORS(USWAttributeSet, SpellPower);

	// --- Defense ---

	UPROPERTY(BlueprintReadOnly, Category = "SW|Attributes|Defense", ReplicatedUsing = OnRep_PhysicalArmor)
	FGameplayAttributeData PhysicalArmor;
	ATTRIBUTE_ACCESSORS(USWAttributeSet, PhysicalArmor);

	UPROPERTY(BlueprintReadOnly, Category = "SW|Attributes|Defense", ReplicatedUsing = OnRep_MagicalArmor)
	FGameplayAttributeData MagicalArmor;
	ATTRIBUTE_ACCESSORS(USWAttributeSet, MagicalArmor);

	// --- Penetration ---

	/** 物理伤害忽略目标物理护甲的比例，合法范围为 0..1。 */
	UPROPERTY(BlueprintReadOnly, Category = "SW|Attributes|Penetration", ReplicatedUsing = OnRep_PhysicalPenetrationPercent)
	FGameplayAttributeData PhysicalPenetrationPercent;
	ATTRIBUTE_ACCESSORS(USWAttributeSet, PhysicalPenetrationPercent);

	/** 法术伤害忽略目标魔法护甲的比例，合法范围为 0..1。 */
	UPROPERTY(BlueprintReadOnly, Category = "SW|Attributes|Penetration", ReplicatedUsing = OnRep_MagicalPenetrationPercent)
	FGameplayAttributeData MagicalPenetrationPercent;
	ATTRIBUTE_ACCESSORS(USWAttributeSet, MagicalPenetrationPercent);

	/** 百分比穿透结算后扣减的固定物理护甲值；不在 AttributeSet 层施加数值约束。 */
	UPROPERTY(BlueprintReadOnly, Category = "SW|Attributes|Penetration", ReplicatedUsing = OnRep_PhysicalPenetrationFlat)
	FGameplayAttributeData PhysicalPenetrationFlat;
	ATTRIBUTE_ACCESSORS(USWAttributeSet, PhysicalPenetrationFlat);

	/** 百分比穿透结算后扣减的固定魔法护甲值；不在 AttributeSet 层施加数值约束。 */
	UPROPERTY(BlueprintReadOnly, Category = "SW|Attributes|Penetration", ReplicatedUsing = OnRep_MagicalPenetrationFlat)
	FGameplayAttributeData MagicalPenetrationFlat;
	ATTRIBUTE_ACCESSORS(USWAttributeSet, MagicalPenetrationFlat);

	// --- Critical ---

	UPROPERTY(BlueprintReadOnly, Category = "SW|Attributes|Critical", ReplicatedUsing = OnRep_CriticalChance)
	FGameplayAttributeData CriticalChance;
	ATTRIBUTE_ACCESSORS(USWAttributeSet, CriticalChance);

	UPROPERTY(BlueprintReadOnly, Category = "SW|Attributes|Critical", ReplicatedUsing = OnRep_CriticalDamage)
	FGameplayAttributeData CriticalDamage;
	ATTRIBUTE_ACCESSORS(USWAttributeSet, CriticalDamage);

	// --- Survivability ---

	UPROPERTY(BlueprintReadOnly, Category = "SW|Attributes|Survivability", ReplicatedUsing = OnRep_Tenacity)
	FGameplayAttributeData Tenacity;
	ATTRIBUTE_ACCESSORS(USWAttributeSet, Tenacity);

	UPROPERTY(BlueprintReadOnly, Category = "SW|Attributes|Survivability", ReplicatedUsing = OnRep_PhysicalLifesteal)
	FGameplayAttributeData PhysicalLifesteal;
	ATTRIBUTE_ACCESSORS(USWAttributeSet, PhysicalLifesteal);

	// --- Regeneration ---

	UPROPERTY(BlueprintReadOnly, Category = "SW|Attributes|Regeneration", ReplicatedUsing = OnRep_ManaRegeneration)
	FGameplayAttributeData ManaRegeneration;
	ATTRIBUTE_ACCESSORS(USWAttributeSet, ManaRegeneration);

	UPROPERTY(BlueprintReadOnly, Category = "SW|Attributes|Regeneration", ReplicatedUsing = OnRep_HealthRegeneration)
	FGameplayAttributeData HealthRegeneration;
	ATTRIBUTE_ACCESSORS(USWAttributeSet, HealthRegeneration);

	/** 每秒自然恢复的体力值；恢复时序由后续 Gameplay Effect 或能力逻辑消费。 */
	UPROPERTY(BlueprintReadOnly, Category = "SW|Attributes|Regeneration", ReplicatedUsing = OnRep_StaminaRegeneration)
	FGameplayAttributeData StaminaRegeneration;
	ATTRIBUTE_ACCESSORS(USWAttributeSet, StaminaRegeneration);

	// --- Ability modifiers ---
	// 百分比修正属性以 0 表示无修正。Ability 读取快照后按统一公式换算有效值。
	//   EffectiveRange    = BaseRange    * (1 + AbilityRangeBonusPercent)
	//   EffectiveArea     = BaseArea     * (1 + AbilityAreaBonusPercent)
	//   EffectiveDuration = BaseDuration * (1 + AbilityDurationBonusPercent)
	//   EffectiveCooldown = BaseCooldown * (1 - CooldownReductionPercent)

	UPROPERTY(BlueprintReadOnly, Category = "SW|Attributes|AbilityMod", ReplicatedUsing = OnRep_AbilityRangeBonusPercent)
	FGameplayAttributeData AbilityRangeBonusPercent;
	ATTRIBUTE_ACCESSORS(USWAttributeSet, AbilityRangeBonusPercent);

	/** 对技能作用半径、碰撞体和对应视觉尺寸的百分比加成；具体技能选择性读取，0 表示不修正。 */
	UPROPERTY(BlueprintReadOnly, Category = "SW|Attributes|AbilityMod", ReplicatedUsing = OnRep_AbilityAreaBonusPercent)
	FGameplayAttributeData AbilityAreaBonusPercent;
	ATTRIBUTE_ACCESSORS(USWAttributeSet, AbilityAreaBonusPercent);

	UPROPERTY(BlueprintReadOnly, Category = "SW|Attributes|AbilityMod", ReplicatedUsing = OnRep_AbilityDurationBonusPercent)
	FGameplayAttributeData AbilityDurationBonusPercent;
	ATTRIBUTE_ACCESSORS(USWAttributeSet, AbilityDurationBonusPercent);

	UPROPERTY(BlueprintReadOnly, Category = "SW|Attributes|AbilityMod", ReplicatedUsing = OnRep_CooldownReductionPercent)
	FGameplayAttributeData CooldownReductionPercent;
	ATTRIBUTE_ACCESSORS(USWAttributeSet, CooldownReductionPercent);

	/** 对主动技能基础最大充能提供的整数加成；具体取整与下限由 Ability 统一处理。 */
	UPROPERTY(BlueprintReadOnly, Category = "SW|Attributes|AbilityMod", ReplicatedUsing = OnRep_AbilityChargeBonus)
	FGameplayAttributeData AbilityChargeBonus;
	ATTRIBUTE_ACCESSORS(USWAttributeSet, AbilityChargeBonus);

	// --- Weapon modifiers ---

	/** 对武器基础弹匣容量的百分比加成；0 表示不加成。 */
	UPROPERTY(BlueprintReadOnly, Category = "SW|Attributes|WeaponMod", ReplicatedUsing = OnRep_MagazineCapacityMultiplier)
	FGameplayAttributeData MagazineCapacityMultiplier;
	ATTRIBUTE_ACCESSORS(USWAttributeSet, MagazineCapacityMultiplier);

	/** 对射击间隔的百分比减免；0 表示不减免。 */
	UPROPERTY(BlueprintReadOnly, Category = "SW|Attributes|WeaponMod", ReplicatedUsing = OnRep_FireIntervalReductionPercent)
	FGameplayAttributeData FireIntervalReductionPercent;
	ATTRIBUTE_ACCESSORS(USWAttributeSet, FireIntervalReductionPercent);

	// --- Meta attributes ---

	/** 瞬时伤害结算载体；执行后转化为生命变化并立即清零，不复制。 */
	UPROPERTY(BlueprintReadOnly, Category = "SW|Attributes|Meta")
	FGameplayAttributeData IncomingDamage;
	ATTRIBUTE_ACCESSORS(USWAttributeSet, IncomingDamage);

	/** 瞬时经验结算载体；执行后写入 PlayerState 并立即清零，不复制。 */
	UPROPERTY(BlueprintReadOnly, Category = "SW|Attributes|Meta")
	FGameplayAttributeData IncomingXP;
	ATTRIBUTE_ACCESSORS(USWAttributeSet, IncomingXP);

protected:
	/** 仅服务器消费伤害 Meta Attribute，并将结果写回真实生命值。 */
	void ConsumeIncomingDamage(const FGameplayEffectModCallbackData& Data);

	/** 仅服务器消费经验 Meta Attribute，并将结果提交给 PlayerState。 */
	void ConsumeIncomingXP(const FGameplayEffectModCallbackData& Data);

	// --- Replication callbacks ---

	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_Mana(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_MaxMana(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_Stamina(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_MaxStamina(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_MovementSpeedMultiplier(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_AttackPower(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_SpellPower(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_PhysicalArmor(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_MagicalArmor(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_PhysicalPenetrationPercent(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_MagicalPenetrationPercent(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_PhysicalPenetrationFlat(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_MagicalPenetrationFlat(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_CriticalChance(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_CriticalDamage(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_Tenacity(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_PhysicalLifesteal(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_ManaRegeneration(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_HealthRegeneration(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_StaminaRegeneration(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_AbilityRangeBonusPercent(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_AbilityAreaBonusPercent(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_AbilityDurationBonusPercent(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_CooldownReductionPercent(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_AbilityChargeBonus(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_MagazineCapacityMultiplier(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_FireIntervalReductionPercent(const FGameplayAttributeData& OldValue) const;
};
