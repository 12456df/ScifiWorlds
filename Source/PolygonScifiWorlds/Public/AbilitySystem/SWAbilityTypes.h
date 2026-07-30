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

	/** 自定义网络序列化；后续新增需复制的字段时在实现中扩展。 */
	virtual bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess) override;
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
