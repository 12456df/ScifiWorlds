// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Active/SWActiveGameplayAbility.h"
#include "ScalableFloat.h"
#include "SWAoeBuffGameplayAbility.generated.h"

class USWSpeedBuffGameplayEffect;
class USWHealBuffGameplayEffect;
class USWStunDebuffGameplayEffect;
class USWPoisonDebuffGameplayEffect;

/** 服务器权威的、以施法者为圆心的半球 AOE：友军获得加速/Heal，敌军获得眩晕/中毒。 */
UCLASS(Abstract, Blueprintable)
class POLYGONSCIFIWORLDS_API USWAoeBuffGameplayAbility : public USWActiveGameplayAbility
{
	GENERATED_BODY()

public:
	USWAoeBuffGameplayAbility();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SW|AOE Buff|Effects")
	TSubclassOf<USWSpeedBuffGameplayEffect> SpeedBuffEffectClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SW|AOE Buff|Effects")
	TSubclassOf<USWHealBuffGameplayEffect> HealBuffEffectClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SW|AOE Buff|Effects")
	TSubclassOf<USWStunDebuffGameplayEffect> StunDebuffEffectClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SW|AOE Buff|Effects")
	TSubclassOf<USWPoisonDebuffGameplayEffect> PoisonDebuffEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SW|AOE Buff|Area", meta = (ClampMin = "0.0"))
	FScalableFloat BaseHemisphereRadius;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SW|AOE Buff|Duration", meta = (ClampMin = "0.0"))
	FScalableFloat BaseBuffDuration;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SW|AOE Buff|Allies", meta = (ClampMin = "0.0"))
	FScalableFloat SpeedMultiplierDelta;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SW|AOE Buff|Allies", meta = (ClampMin = "0.0"))
	FScalableFloat BaseHealPerSecond;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SW|AOE Buff|Allies", meta = (ClampMin = "0.0"))
	FScalableFloat CasterCurrentHealthHealCoefficientPerSecond;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SW|AOE Buff|Enemies", meta = (ClampMin = "0.0"))
	FScalableFloat BasePoisonDamagePerTick;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SW|AOE Buff|Enemies", meta = (ClampMin = "0.0"))
	FScalableFloat SpellPowerCoefficientPerTick;
	/**
	 * 首版 Stun 的实际移动速度加法修正。没有眩晕动作时保持角色可行动，
	 * 以负值降低 MovementSpeedMultiplier；默认 -0.5 即减速 50%。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SW|AOE Buff|Enemies", meta = (ClampMin = "-0.99", ClampMax = "0.0"))
	FScalableFloat StunMovementSpeedDelta;

	/** 仅服务器调用；蓝图在 Commit 成功后的施法命中帧调用一次。 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "SW|AOE Buff")
	int32 ApplyCasterCenteredHemisphereBuffsAuthority();
};
