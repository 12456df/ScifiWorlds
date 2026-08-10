// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "AbilitySystem/Abilities/Active/SWActiveGameplayAbility.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/SWAttributeSet.h"
#include "GameplayEffect.h"
#include "GameplayTags/SWGameplayTags.h"

USWActiveGameplayAbility::USWActiveGameplayAbility()
{
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(SWGameplayTags::Ability_Type_ActiveSkill);
	SetAssetTags(AssetTags);

	// 主动技能默认允许升级；具体内容可在蓝图 Class Defaults 中显式关闭。
	bUpgradeable = true;
	MaxChargesByLevel.Value = 1.f;
}

float USWActiveGameplayAbility::GetEffectiveRange(const float BaseRange) const
{
	const float SafeBaseRange = FMath::IsFinite(BaseRange) ? FMath::Max(0.f, BaseRange) : 0.f;
	const float BonusPercent = GetOwnerAttributeValue(USWAttributeSet::GetAbilityRangeBonusPercentAttribute());
	return FMath::Max(0.f, SafeBaseRange * (1.f + BonusPercent));
}

float USWActiveGameplayAbility::GetEffectiveAreaScale() const
{
	const float BonusPercent = GetOwnerAttributeValue(USWAttributeSet::GetAbilityAreaBonusPercentAttribute());
	return FMath::Max(KINDA_SMALL_NUMBER, 1.f + BonusPercent);
}

float USWActiveGameplayAbility::GetEffectiveArea(const float BaseArea) const
{
	const float SafeBaseArea = FMath::IsFinite(BaseArea) ? FMath::Max(0.f, BaseArea) : 0.f;
	return SafeBaseArea * GetEffectiveAreaScale();
}

float USWActiveGameplayAbility::GetEffectiveDuration(const float BaseDuration) const
{
	const float SafeBaseDuration = FMath::IsFinite(BaseDuration) ? FMath::Max(0.f, BaseDuration) : 0.f;
	const float BonusPercent = GetOwnerAttributeValue(USWAttributeSet::GetAbilityDurationBonusPercentAttribute());
	return FMath::Max(0.f, SafeBaseDuration * (1.f + BonusPercent));
}

float USWActiveGameplayAbility::GetEffectiveCooldown(const float BaseCooldown) const
{
	const float SafeBaseCooldown = FMath::IsFinite(BaseCooldown) ? FMath::Max(0.f, BaseCooldown) : 0.f;
	const float SafeMinimum = FMath::IsFinite(MinimumCooldownSeconds) ? FMath::Max(0.f, MinimumCooldownSeconds) : 0.f;
	const float ReductionPercent = GetOwnerAttributeValue(USWAttributeSet::GetCooldownReductionPercentAttribute());
	const float Result = SafeBaseCooldown * (1.f - ReductionPercent);
	return FMath::Max(SafeMinimum, FMath::IsFinite(Result) ? Result : SafeMinimum);
}

int32 USWActiveGameplayAbility::GetEffectiveCharges(const float BaseCharges) const
{
	const float SafeBaseCharges = FMath::IsFinite(BaseCharges) ? BaseCharges : 1.f;
	if (!bUseAbilityChargeBonus)
	{
		return FMath::Max(1, FMath::FloorToInt(SafeBaseCharges));
	}

	const float ChargeBonus = GetOwnerAttributeValue(USWAttributeSet::GetAbilityChargeBonusAttribute());
	const float SafeChargeBonus = FMath::IsFinite(ChargeBonus) ? ChargeBonus : 0.f;
	return FMath::Max(1, FMath::FloorToInt(SafeBaseCharges) + FMath::FloorToInt(SafeChargeBonus));
}

int32 USWActiveGameplayAbility::GetMaxCharges() const
{
	return GetMaxCharges(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo());
}

int32 USWActiveGameplayAbility::GetMaxChargesForLevel(const int32 AbilityLevel, const UAbilitySystemComponent* const AbilitySystemComponent) const
{
	const float BaseCharges = MaxChargesByLevel.GetValueAtLevel(FMath::Max(1, AbilityLevel));
	const float SafeBaseCharges = FMath::IsFinite(BaseCharges) ? BaseCharges : 1.f;
	if (!bUseAbilityChargeBonus || !AbilitySystemComponent)
	{
		return FMath::Max(1, FMath::FloorToInt(SafeBaseCharges));
	}

	const float ChargeBonus = AbilitySystemComponent->GetNumericAttribute(USWAttributeSet::GetAbilityChargeBonusAttribute());
	const float SafeChargeBonus = FMath::IsFinite(ChargeBonus) ? ChargeBonus : 0.f;
	return FMath::Max(1, FMath::FloorToInt(SafeBaseCharges) + FMath::FloorToInt(SafeChargeBonus));
}

int32 USWActiveGameplayAbility::GetAvailableCharges() const
{
	const FGameplayAbilityActorInfo* const ActorInfo = GetCurrentActorInfo();
	const int32 MaxCharges = GetMaxCharges();
	return UsesChargeCooldown(GetCurrentAbilitySpecHandle(), ActorInfo)
		? FMath::Max(0, MaxCharges - GetSpentChargeCount(ActorInfo))
		: MaxCharges;
}

float USWActiveGameplayAbility::GetEffectiveManaCost() const
{
	const float ManaCost = ManaCostByLevel.GetValueAtLevel(FMath::Max(1, GetAbilityLevel()));
	return FMath::IsFinite(ManaCost) ? FMath::Max(0.f, ManaCost) : 0.f;
}

bool USWActiveGameplayAbility::CheckCost(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	const int32 AbilityLevel = GetAbilityLevel(Handle, ActorInfo);
	const float ManaCost = ManaCostByLevel.GetValueAtLevel(FMath::Max(1, AbilityLevel));
	const float SafeManaCost = FMath::IsFinite(ManaCost) ? FMath::Max(0.f, ManaCost) : 0.f;
	if (SafeManaCost <= 0.f)
	{
		return true;
	}

	if (!GetCostGameplayEffect())
	{
		if (OptionalRelevantTags)
		{
			OptionalRelevantTags->AddTag(SWGameplayTags::Ability_Fail_InvalidCostData);
		}
		return false;
	}

	const float CurrentMana = GetOwnerAttributeValue(USWAttributeSet::GetManaAttribute());
	if (!FMath::IsFinite(CurrentMana) || CurrentMana < SafeManaCost)
	{
		if (OptionalRelevantTags)
		{
			OptionalRelevantTags->AddTag(SWGameplayTags::Ability_Fail_NoMana);
		}
		return false;
	}

	return true;
}

bool USWActiveGameplayAbility::CheckCooldown(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!UsesChargeCooldown(Handle, ActorInfo))
	{
		return true;
	}

	if (GetSpentChargeCount(ActorInfo) >= GetMaxCharges(Handle, ActorInfo))
	{
		if (OptionalRelevantTags)
		{
			OptionalRelevantTags->AddTag(SWGameplayTags::Ability_Fail_NoCharges);
		}
		return false;
	}

	return true;
}

void USWActiveGameplayAbility::ApplyCost(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	if (!ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid())
	{
		return;
	}

	const int32 AbilityLevel = GetAbilityLevel(Handle, ActorInfo);
	const float ManaCost = ManaCostByLevel.GetValueAtLevel(FMath::Max(1, AbilityLevel));
	const float SafeManaCost = FMath::IsFinite(ManaCost) ? FMath::Max(0.f, ManaCost) : 0.f;
	UGameplayEffect* CostGameplayEffect = GetCostGameplayEffect();
	if (SafeManaCost <= 0.f || !CostGameplayEffect)
	{
		return;
	}

	FGameplayEffectSpecHandle CostSpec = MakeOutgoingGameplayEffectSpec(
		Handle, ActorInfo, ActivationInfo, CostGameplayEffect->GetClass(), AbilityLevel);
	if (!CostSpec.IsValid())
	{
		return;
	}

	// Cost GE 使用 Add 修改 Mana；扣除量必须以负值写入 SetByCaller。
	CostSpec.Data->SetSetByCallerMagnitude(SWGameplayTags::SetByCaller_Ability_ManaCost, -SafeManaCost);
	ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, CostSpec);
}

UGameplayEffect* USWActiveGameplayAbility::GetCooldownGameplayEffect() const
{
	return CooldownEffectClass ? CooldownEffectClass->GetDefaultObject<UGameplayEffect>() : nullptr;
}

void USWActiveGameplayAbility::ApplyCooldown(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	if (!CooldownEffectClass || !ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid())
	{
		return;
	}

	const int32 AbilityLevel = GetAbilityLevel(Handle, ActorInfo);
	const float BaseCooldown = CooldownByLevel.GetValueAtLevel(FMath::Max(1, AbilityLevel));
	const float EffectiveCooldown = GetEffectiveCooldown(BaseCooldown);
	if (EffectiveCooldown <= 0.f)
	{
		return;
	}

	FGameplayEffectSpecHandle CooldownSpec = MakeOutgoingGameplayEffectSpec(
		Handle, ActorInfo, ActivationInfo, CooldownEffectClass, AbilityLevel);
	if (!CooldownSpec.IsValid())
	{
		return;
	}

	CooldownSpec.Data->SetSetByCallerMagnitude(SWGameplayTags::SetByCaller_Ability_Cooldown, EffectiveCooldown);
	ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, CooldownSpec);
}

int32 USWActiveGameplayAbility::GetMaxCharges(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo) const
{
	const int32 AbilityLevel = GetAbilityLevel(Handle, ActorInfo);
	return GetMaxChargesForLevel(AbilityLevel, ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr);
}

int32 USWActiveGameplayAbility::GetSpentChargeCount(const FGameplayAbilityActorInfo* ActorInfo) const
{
	if (!ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid() || !CooldownTag.IsValid())
	{
		return 0;
	}

	FGameplayTagContainer CooldownTags;
	CooldownTags.AddTag(CooldownTag);
	const FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(CooldownTags);
	const TArray<FActiveGameplayEffectHandle> ActiveEffects = ActorInfo->AbilitySystemComponent->GetActiveEffects(Query);

	int32 SpentCharges = 0;
	for (const FActiveGameplayEffectHandle& ActiveEffect : ActiveEffects)
	{
		SpentCharges += FMath::Max(0, ActorInfo->AbilitySystemComponent->GetCurrentStackCount(ActiveEffect));
	}

	return SpentCharges;
}

bool USWActiveGameplayAbility::UsesChargeCooldown(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo) const
{
	if (!CooldownEffectClass || !CooldownTag.IsValid())
	{
		return false;
	}

	const int32 AbilityLevel = GetAbilityLevel(Handle, ActorInfo);
	const float BaseCooldown = CooldownByLevel.GetValueAtLevel(FMath::Max(1, AbilityLevel));
	return GetEffectiveCooldown(BaseCooldown) > 0.f;
}
