// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "SWAttributeSet.generated.h"

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

	UPROPERTY(BlueprintReadOnly, Category = "SW|Attributes|Penetration", ReplicatedUsing = OnRep_PhysicalPenetration)
	FGameplayAttributeData PhysicalPenetration;
	ATTRIBUTE_ACCESSORS(USWAttributeSet, PhysicalPenetration);

	UPROPERTY(BlueprintReadOnly, Category = "SW|Attributes|Penetration", ReplicatedUsing = OnRep_MagicalPenetration)
	FGameplayAttributeData MagicalPenetration;
	ATTRIBUTE_ACCESSORS(USWAttributeSet, MagicalPenetration);

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

	// --- Ability modifiers ---
	// 0 means no modifier. Abilities read a snapshot and apply the unified formulas:
	//   EffectiveRange    = BaseRange    * (1 + AbilityRangeMultiplier)
	//   EffectiveDuration = BaseDuration * (1 + AbilityDurationMultiplier)
	//   EffectiveCooldown = BaseCooldown * (1 - CooldownReductionMultiplier)

	UPROPERTY(BlueprintReadOnly, Category = "SW|Attributes|AbilityMod", ReplicatedUsing = OnRep_AbilityRangeMultiplier)
	FGameplayAttributeData AbilityRangeMultiplier;
	ATTRIBUTE_ACCESSORS(USWAttributeSet, AbilityRangeMultiplier);

	UPROPERTY(BlueprintReadOnly, Category = "SW|Attributes|AbilityMod", ReplicatedUsing = OnRep_AbilityDurationMultiplier)
	FGameplayAttributeData AbilityDurationMultiplier;
	ATTRIBUTE_ACCESSORS(USWAttributeSet, AbilityDurationMultiplier);

	UPROPERTY(BlueprintReadOnly, Category = "SW|Attributes|AbilityMod", ReplicatedUsing = OnRep_CooldownReductionMultiplier)
	FGameplayAttributeData CooldownReductionMultiplier;
	ATTRIBUTE_ACCESSORS(USWAttributeSet, CooldownReductionMultiplier);

	// --- Weapon modifiers ---

	/** 对武器基础弹匣容量的百分比加成；0 表示不加成。 */
	UPROPERTY(BlueprintReadOnly, Category = "SW|Attributes|WeaponMod", ReplicatedUsing = OnRep_MagazineCapacityBonusPercent)
	FGameplayAttributeData MagazineCapacityBonusPercent;
	ATTRIBUTE_ACCESSORS(USWAttributeSet, MagazineCapacityBonusPercent);

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
	void OnRep_AttackPower(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_SpellPower(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_PhysicalArmor(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_MagicalArmor(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_PhysicalPenetration(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_MagicalPenetration(const FGameplayAttributeData& OldValue) const;

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
	void OnRep_AbilityRangeMultiplier(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_AbilityDurationMultiplier(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_CooldownReductionMultiplier(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_MagazineCapacityBonusPercent(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_FireIntervalReductionPercent(const FGameplayAttributeData& OldValue) const;
};
