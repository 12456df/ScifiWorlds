// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/SWProjectile.h"
#include "SWStructureAttackProjectile.generated.h"

/**
 * 防御结构专用的服务器权威追踪投射物。
 *
 * 它复用项目统一的伤害 GE 与复制弹丸生命周期，但使用独立 StructureProjectile 对象类型，
 * 因而不会参与玩家屏障对 Projectile 的吸收。追踪与命中只在服务器模拟，客户端仅接收复制表现。
 */
UCLASS(Abstract, Blueprintable)
class POLYGONSCIFIWORLDS_API ASWStructureAttackProjectile : public ASWProjectile
{
	GENERATED_BODY()

public:
	ASWStructureAttackProjectile();

	/**
	 * 仅由结构攻击 Ability 在服务器 Deferred Spawn 后调用。
	 * TargetActor 必须是当前结构锁定的有效目标；初始化完成后蓝图不得改写追踪目标或伤害数据。
	 */
	bool InitializeStructureAttackProjectileAuthority(APawn* InInstigatorPawn, AActor* TargetActor,
		const FVector& LaunchDirection, TSubclassOf<USWDamageGameplayEffect> InDamageEffectClass,
		const FSWDamageApplicationParams& InDamageParams);

	virtual bool CanBeAbsorbedByShield_Implementation() const override;

protected:
	virtual ECollisionChannel GetProjectileCollisionChannel() const override;
	virtual bool ShouldHandleImpactAuthority(AActor* HitActor) const override;

	/** 追踪加速度由具体火球蓝图配置；它不是攻击间隔或伤害数值。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Structure|Projectile", meta = (ClampMin = "0.0"))
	float HomingAccelerationMagnitude = 12000.f;

private:
	/** 只在服务器存储的追踪目标；其失效后弹体按现有速度继续，最终由寿命清理。 */
	TWeakObjectPtr<AActor> HomingTarget;
};
