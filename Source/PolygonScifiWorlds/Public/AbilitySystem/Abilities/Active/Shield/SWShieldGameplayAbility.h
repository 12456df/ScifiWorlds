// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Active/SWActiveGameplayAbility.h"
#include "ScalableFloat.h"
#include "SWShieldGameplayAbility.generated.h"

class ASWShieldBarrier;

/** Shield 的技能数据与服务器生成入口；施法时序、Montage 与表现由蓝图子类编排。 */
UCLASS(Abstract, Blueprintable)
class POLYGONSCIFIWORLDS_API USWShieldGameplayAbility : public USWActiveGameplayAbility
{
	GENERATED_BODY()

public:
	USWShieldGameplayAbility();

	/** 屏障 Actor 类；其蓝图负责默认立方体尺寸、Niagara 与材质。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SW|Shield")
	TSubclassOf<ASWShieldBarrier> ShieldClass;

	/** 从 Avatar 朝向前方开始计算的固定生成距离。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SW|Shield", meta = (ClampMin = "0.0"))
	FScalableFloat ForwardSpawnDistance;

	/** 屏障存活时长；最终值受 AbilityDurationBonusPercent 影响。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SW|Shield", meta = (ClampMin = "0.0"))
	FScalableFloat BaseDuration;

	UFUNCTION(BlueprintPure, Category = "SW|Shield")
	float GetForwardSpawnDistance(int32 AbilityLevel) const;

	UFUNCTION(BlueprintPure, Category = "SW|Shield")
	float GetEffectiveShieldDuration(int32 AbilityLevel) const;

	/** 返回预览应使用的最终吸收盒半尺寸；范围加成只缩放 Y、Z，X 厚度保持默认值。 */
	UFUNCTION(BlueprintPure, Category = "SW|Shield")
	FVector GetEffectiveShieldPreviewBoxExtent() const;

	/**
	 * 仅服务器调用。生成位置只由服务器 Avatar 的位置和朝向推导，蓝图不能提交任意位置或旋转。
	 * 应在施法 Montage 的 Event.Ability.Shield.Spawn 帧调用。
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "SW|Shield")
	bool SpawnShieldAuthority();
};
