// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "AbilitySystem/Abilities/SWGameplayAbility.h"
#include "GameplayTagContainer.h"
#include "ScalableFloat.h"
#include "TimerManager.h"
#include "SWStructureAttackGameplayAbility.generated.h"

/**
 * 防御塔与水晶攻击 Ability 的类型契约。
 *
 * C++ 固定服务器目标复核、冷却提交、攻击前摇和火球生成；蓝图只配置数值、GE、火球类和表现。
 */
UCLASS(Abstract)
class POLYGONSCIFIWORLDS_API USWStructureAttackGameplayAbility : public USWGameplayAbility
{
	GENERATED_BODY()

public:
	USWStructureAttackGameplayAbility();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void CancelAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateCancelAbility) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	/** 让 GAS 的默认冷却检查与 Commit 使用本类公开的 CooldownEffectClass。 */
	virtual UGameplayEffect* GetCooldownGameplayEffect() const override;

	/** 结构等级对应的基础物理伤害；不读取角色攻击力或法强。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Structure|Attack|Damage", meta = (ClampMin = "0.0"))
	FScalableFloat RawDamageByLevel;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Structure|Attack|Damage", meta = (Categories = "Damage.Type"))
	FGameplayTag DamageType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Structure|Attack|Damage")
	bool bCanCritical = false;

	/** 结构攻击必须使用项目统一的 Instant Damage GE。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Structure|Attack|Damage")
	TSubclassOf<class USWDamageGameplayEffect> DamageEffectClass;

	/** 结构火球的蓝图子类；负责 Mesh、Niagara 与 FSWProjectileConfig。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Structure|Attack|Projectile")
	TSubclassOf<class ASWStructureAttackProjectile> ProjectileClass;

	/** 服务器从收到攻击事件到生成火球的前摇时间；不使用 Tick。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Structure|Attack|Timing", meta = (ClampMin = "0.0"))
	float AttackWindupSeconds = 0.f;

	/** 冷却 GE 的 Granted Tag 是攻击频率唯一真值。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Structure|Attack|Cooldown")
	TSubclassOf<class UGameplayEffect> CooldownEffectClass;

	/** 纯表现 GameplayCue；留空时不执行。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Structure|Attack|Cue", meta = (Categories = "GameplayCue"))
	FGameplayTag AttackCueTag;

private:
	/** 服务器前摇结束时再次复核目标并生成火球。 */
	void ExecutePendingAttackAuthority();

	bool SpawnProjectileAuthority(AActor* TargetActor);
	void ClearPendingAttack();

	TWeakObjectPtr<AActor> PendingTarget;
	FGameplayAbilitySpecHandle PendingHandle;
	FGameplayAbilityActivationInfo PendingActivationInfo;
	FTimerHandle AttackWindupTimer;
};
