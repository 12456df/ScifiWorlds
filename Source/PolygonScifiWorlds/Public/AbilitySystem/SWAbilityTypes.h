// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "SWAbilityTypes.generated.h"

class USWGameplayAbility;

/** 蓝图默认值中的启动技能与其输入路由 Tag。 */
USTRUCT(BlueprintType)
struct FSWStartupAbility
{
	GENERATED_BODY()

	/** 要由服务器授予的技能蓝图或 C++ 类。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
	TSubclassOf<USWGameplayAbility> AbilityClass;

	/** 与 Input Config 中 IA 对应的输入 Tag。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability", meta = (Categories = "Ability.Input"))
	FGameplayTag InputTag;
};

/**
 * ScifiWorlds 项目自定义的 GameplayEffectContext。
 *
 * 作为可扩展的效果上下文基础类型，供技能/效果在结算时携带项目特有的额外数据。
 * M03 只建立该扩展点与网络序列化契约，暂不定义任何战斗结果字段
 *（暴击、格挡、穿透、韧性、吸血等结算标记随 M05/M06 战斗结算加入）。
 */
USTRUCT(BlueprintType)
struct FSWGameplayEffectContext : public FGameplayEffectContext
{
	GENERATED_BODY()

public:
	/** 返回用于序列化的真实结构体类型；子类必须覆写。 */
	virtual UScriptStruct* GetScriptStruct() const override
	{
		return FSWGameplayEffectContext::StaticStruct();
	}

	/** 复制上下文，保留命中结果；子类必须覆写以返回正确的派生类型。 */
	virtual FGameplayEffectContext* Duplicate() const override
	{
		FSWGameplayEffectContext* NewContext = new FSWGameplayEffectContext();
		*NewContext = *this;
		if (GetHitResult())
		{
			NewContext->AddHitResult(*GetHitResult(), true);
		}
		return NewContext;
	}

	/** 返回本次伤害结算的类型；仅由服务器伤害执行计算写入。 */
	FGameplayTag GetDamageType() const { return DamageType; }

	/** 返回本次伤害是否由服务器判定为暴击。 */
	bool IsCriticalHit() const { return bCriticalHit; }

	/** 返回服务器在本次物理伤害结算中使用的物理吸血比例。 */
	float GetPhysicalLifesteal() const { return PhysicalLifesteal; }

	/** 仅服务器伤害执行计算调用：记录伤害类型，供后续结算与表现读取。 */
	void SetDamageType(const FGameplayTag InDamageType) { DamageType = InDamageType; }

	/** 仅服务器伤害执行计算调用：记录暴击判定结果，客户端不得自行重掷。 */
	void SetCriticalHit(const bool bInCriticalHit) { bCriticalHit = bInCriticalHit; }

	/** 仅服务器伤害执行计算调用：记录本次物理伤害可用于结算的吸血比例。 */
	void SetPhysicalLifesteal(const float InPhysicalLifesteal) { PhysicalLifesteal = InPhysicalLifesteal; }

	virtual bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess) override;

private:
	/** 本次伤害的物理、魔法或真实伤害 Tag。 */
	UPROPERTY()
	FGameplayTag DamageType;

	/** 由服务器唯一写入的暴击结果。 */
	UPROPERTY()
	bool bCriticalHit = false;

	/** 本次物理伤害在服务器快照得到的吸血比例，不参与客户端伤害重算。 */
	UPROPERTY()
	float PhysicalLifesteal = 0.f;
};

// 告知引擎该结构体支持自定义网络序列化与拷贝。
template<>
struct TStructOpsTypeTraits<FSWGameplayEffectContext> : public TStructOpsTypeTraitsBase2<FSWGameplayEffectContext>
{
	enum
	{
		WithNetSerializer = true,
		WithCopy = true
	};
};
