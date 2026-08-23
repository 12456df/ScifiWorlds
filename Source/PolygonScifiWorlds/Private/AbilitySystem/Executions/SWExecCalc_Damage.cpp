// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "AbilitySystem/Executions/SWExecCalc_Damage.h"

#include "AbilitySystem/Data/SWDamageCalculationConfig.h"
#include "AbilitySystem/Effects/SWDamageGameplayEffect.h"
#include "AbilitySystem/SWAttributeSet.h"
#include "AbilitySystem/SWAbilityTypes.h"
#include "AbilitySystem/SWGameplayEffect.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "GameMode/SWGameMode.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectExtension.h"
#include "GameplayTags/SWGameplayTags.h"
#include "Interaction/SWDamageReceiverPolicyInterface.h"

namespace
{
	struct FSWDamageCaptureStatics
	{
		DECLARE_ATTRIBUTE_CAPTUREDEF(PhysicalPenetrationPercent)
		DECLARE_ATTRIBUTE_CAPTUREDEF(MagicalPenetrationPercent)
		DECLARE_ATTRIBUTE_CAPTUREDEF(PhysicalPenetrationFlat)
		DECLARE_ATTRIBUTE_CAPTUREDEF(MagicalPenetrationFlat)
		DECLARE_ATTRIBUTE_CAPTUREDEF(CriticalChance)
		DECLARE_ATTRIBUTE_CAPTUREDEF(CriticalDamage)
		DECLARE_ATTRIBUTE_CAPTUREDEF(PhysicalLifesteal)
		DECLARE_ATTRIBUTE_CAPTUREDEF(PhysicalArmor)
		DECLARE_ATTRIBUTE_CAPTUREDEF(MagicalArmor)

		FSWDamageCaptureStatics()
		{
			DEFINE_ATTRIBUTE_CAPTUREDEF(USWAttributeSet, PhysicalPenetrationPercent, Source, true)
			DEFINE_ATTRIBUTE_CAPTUREDEF(USWAttributeSet, MagicalPenetrationPercent, Source, true)
			DEFINE_ATTRIBUTE_CAPTUREDEF(USWAttributeSet, PhysicalPenetrationFlat, Source, true)
			DEFINE_ATTRIBUTE_CAPTUREDEF(USWAttributeSet, MagicalPenetrationFlat, Source, true)
			DEFINE_ATTRIBUTE_CAPTUREDEF(USWAttributeSet, CriticalChance, Source, true)
			DEFINE_ATTRIBUTE_CAPTUREDEF(USWAttributeSet, CriticalDamage, Source, true)
			DEFINE_ATTRIBUTE_CAPTUREDEF(USWAttributeSet, PhysicalLifesteal, Source, true)
			DEFINE_ATTRIBUTE_CAPTUREDEF(USWAttributeSet, PhysicalArmor, Target, false)
			DEFINE_ATTRIBUTE_CAPTUREDEF(USWAttributeSet, MagicalArmor, Target, false)
		}
	};

	const FSWDamageCaptureStatics& DamageCaptureStatics()
	{
		static const FSWDamageCaptureStatics Statics;
		return Statics;
	}

	float CaptureMagnitude(
		const FGameplayEffectCustomExecutionParameters& ExecutionParams,
		const FGameplayEffectAttributeCaptureDefinition& CaptureDefinition,
		const FAggregatorEvaluateParameters& EvaluationParameters)
	{
		float Magnitude = 0.f;
		ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(CaptureDefinition, EvaluationParameters, Magnitude);
		return Magnitude;
	}

	float CalculateMitigatedDamage(float RawDamage, float Armor, float PercentPenetration, float FlatPenetration, float HalfPoint)
	{
		const float ArmorAfterPercentPenetration = FMath::Max(0.f, Armor * (1.f - FMath::Clamp(PercentPenetration, 0.f, 1.f)));
		const float EffectiveArmor = FMath::Max(0.f, ArmorAfterPercentPenetration - FlatPenetration);
		const float DamageReduction = EffectiveArmor / (EffectiveArmor + HalfPoint);
		return FMath::Max(0.f, RawDamage) * (1.f - DamageReduction);
	}
}

USWExecCalc_Damage::USWExecCalc_Damage()
{
	const FSWDamageCaptureStatics& Statics = DamageCaptureStatics();
	RelevantAttributesToCapture.Add(Statics.PhysicalPenetrationPercentDef);
	RelevantAttributesToCapture.Add(Statics.MagicalPenetrationPercentDef);
	RelevantAttributesToCapture.Add(Statics.PhysicalPenetrationFlatDef);
	RelevantAttributesToCapture.Add(Statics.MagicalPenetrationFlatDef);
	RelevantAttributesToCapture.Add(Statics.CriticalChanceDef);
	RelevantAttributesToCapture.Add(Statics.CriticalDamageDef);
	RelevantAttributesToCapture.Add(Statics.PhysicalLifestealDef);
	RelevantAttributesToCapture.Add(Statics.PhysicalArmorDef);
	RelevantAttributesToCapture.Add(Statics.MagicalArmorDef);
}

void USWExecCalc_Damage::Execute_Implementation(
	const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	UAbilitySystemComponent* const TargetAbilitySystemComponent = ExecutionParams.GetTargetAbilitySystemComponent();
	UAbilitySystemComponent* const SourceAbilitySystemComponent = ExecutionParams.GetSourceAbilitySystemComponent();
	if (!TargetAbilitySystemComponent || !TargetAbilitySystemComponent->IsOwnerActorAuthoritative())
	{
		return;
	}

	// ExecCalc 是所有伤害通道最终汇合处；即使迟到 Projectile、周期 GE 或 AI 未及时停止，也不能在非正式对局阶段写入 IncomingDamage。
	const UWorld* const World = TargetAbilitySystemComponent->GetWorld();
	const ASWGameMode* const GameMode = World ? World->GetAuthGameMode<ASWGameMode>() : nullptr;
	if (!GameMode || GameMode->GetMatchState() != MatchState::InProgress)
	{
		return;
	}

	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
	const USWDamageGameplayEffect* const DamageEffect = Cast<USWDamageGameplayEffect>(Spec.Def);
	if (!DamageEffect || !DamageEffect->GetDamageCalculationConfig())
	{
		UE_LOG(LogTemp, Warning, TEXT("伤害 GE 缺少 USWDamageGameplayEffect 类型或伤害计算配置：%s"), *GetNameSafe(Spec.Def));
		return;
	}

	if (SourceAbilitySystemComponent == TargetAbilitySystemComponent
		|| USWGameplayEffect::AreSourceAndTargetOnSameTeam(Spec, TargetAbilitySystemComponent)
		|| TargetAbilitySystemComponent->HasMatchingGameplayTag(SWGameplayTags::State_Dead))
	{
		return;
	}

	FGameplayEffectContext* const RawContext = Spec.GetContext().Get();
	FSWGameplayEffectContext* const SWContext = RawContext && RawContext->GetScriptStruct() == FSWGameplayEffectContext::StaticStruct()
		? static_cast<FSWGameplayEffectContext*>(RawContext)
		: nullptr;
	if (!SWContext)
	{
		UE_LOG(LogTemp, Warning, TEXT("伤害 GE 未使用 FSWGameplayEffectContext：%s"), *GetNameSafe(Spec.Def));
		return;
	}

	const FGameplayTag DamageType = SWContext->GetDamageType();
	if (DamageType != SWGameplayTags::Damage_Type_Physical
		&& DamageType != SWGameplayTags::Damage_Type_Magical
		&& DamageType != SWGameplayTags::Damage_Type_True)
	{
		UE_LOG(LogTemp, Warning, TEXT("伤害 GE 缺少有效伤害类型：%s"), *GetNameSafe(Spec.Def));
		return;
	}

	// 特殊接收者只给出纯服务器决策；常规目标没有该接口，继续走原有统一伤害公式。
	float PostMitigationMultiplier = 1.f;
	AActor* const TargetAvatar = TargetAbilitySystemComponent->GetAvatarActor();
	if (const ISWDamageReceiverPolicyInterface* const ReceiverPolicy = Cast<ISWDamageReceiverPolicyInterface>(TargetAvatar))
	{
		AActor* const SourceAvatar = Spec.GetContext().GetOriginalInstigator();
		FSWDamageReceptionQuery ReceptionQuery;
		ReceptionQuery.SourceAvatar = SourceAvatar;
		ReceptionQuery.TargetActor = TargetAvatar;
		ReceptionQuery.DamageType = DamageType;
		ReceptionQuery.ServerSourceLocation = SourceAvatar ? SourceAvatar->GetActorLocation() : FVector::ZeroVector;

		const FSWDamageReceptionResult ReceptionResult = ReceiverPolicy->EvaluateDamageReceptionAuthority(ReceptionQuery);
		if (!ReceptionResult.bAccepted || !FMath::IsFinite(ReceptionResult.PostMitigationMultiplier))
		{
			return;
		}

		PostMitigationMultiplier = FMath::Max(0.f, ReceptionResult.PostMitigationMultiplier);
	}

	// 通用无敌 Tag 是所有受击者共享的最终门槛；结构策略已在上方给出更具体的拒绝原因。
	if (TargetAbilitySystemComponent->HasMatchingGameplayTag(SWGameplayTags::State_Invulnerable))
	{
		return;
	}

	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	EvaluationParameters.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	const FSWDamageCaptureStatics& Statics = DamageCaptureStatics();
	const float RawDamage = FMath::Max(0.f, Spec.GetSetByCallerMagnitude(SWGameplayTags::SetByCaller_Damage_Raw, false, 0.f));

	float FinalDamage = RawDamage;
	const USWDamageCalculationConfig* const DamageConfig = DamageEffect->GetDamageCalculationConfig();
	if (DamageType == SWGameplayTags::Damage_Type_Physical)
	{
		FinalDamage = CalculateMitigatedDamage(
			RawDamage,
			CaptureMagnitude(ExecutionParams, Statics.PhysicalArmorDef, EvaluationParameters),
			CaptureMagnitude(ExecutionParams, Statics.PhysicalPenetrationPercentDef, EvaluationParameters),
			CaptureMagnitude(ExecutionParams, Statics.PhysicalPenetrationFlatDef, EvaluationParameters),
			FMath::Max(0.001f, DamageConfig->PhysicalArmorMitigationHalfPoint));
	}
	else if (DamageType == SWGameplayTags::Damage_Type_Magical)
	{
		FinalDamage = CalculateMitigatedDamage(
			RawDamage,
			CaptureMagnitude(ExecutionParams, Statics.MagicalArmorDef, EvaluationParameters),
			CaptureMagnitude(ExecutionParams, Statics.MagicalPenetrationPercentDef, EvaluationParameters),
			CaptureMagnitude(ExecutionParams, Statics.MagicalPenetrationFlatDef, EvaluationParameters),
			FMath::Max(0.001f, DamageConfig->MagicalArmorMitigationHalfPoint));
	}

	bool bCriticalHit = false;
	if (SWContext->CanCritical())
	{
		const float CriticalChance = FMath::Clamp(
			CaptureMagnitude(ExecutionParams, Statics.CriticalChanceDef, EvaluationParameters),
			0.f,
			DamageConfig->MaxCriticalChance);
		if (FMath::FRand() < CriticalChance)
		{
			bCriticalHit = true;
			FinalDamage *= FMath::Max(1.f, CaptureMagnitude(ExecutionParams, Statics.CriticalDamageDef, EvaluationParameters));
		}
	}

	// 结构等特殊目标在护甲、穿透和暴击之后应用自己的静态减伤；吸血随后按实际扣血量结算。
	FinalDamage *= PostMitigationMultiplier;

	const float CapturedPhysicalLifesteal = DamageType == SWGameplayTags::Damage_Type_Physical
		? CaptureMagnitude(ExecutionParams, Statics.PhysicalLifestealDef, EvaluationParameters)
		: 0.f;
	const float PhysicalLifesteal = FMath::IsFinite(CapturedPhysicalLifesteal)
		? FMath::Max(0.f, CapturedPhysicalLifesteal)
		: 0.f;

	// EffectContext 跨越执行计算与 AttributeSet 消费阶段，因此保存本次伤害结果及吸血快照。
	SWContext->SetCriticalHit(bCriticalHit);
	SWContext->SetPhysicalLifesteal(PhysicalLifesteal);

	if (FinalDamage > 0.f)
	{
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
			USWAttributeSet::GetIncomingDamageAttribute(),
			EGameplayModOp::Additive,
			FinalDamage));
	}
}
