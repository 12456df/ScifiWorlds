// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "Animation/AnimInstance.h"
#include "CoreMinimal.h"
#include "SWAnimInstance.generated.h"

class ACharacter;
class UCharacterMovementComponent;

/** 动画图读取的地面移动状态。 */
UENUM(BlueprintType)
enum class ESWLocomotionState : uint8
{
	Idle,
	Moving,
	Sprinting,
	Crouching,
	InAir
};

/**
 * 项目动画蓝图的只读 C++ 基类。
 *
 * 所有 Gameplay UObject 查询均在 NativeUpdateAnimation 的游戏线程阶段完成；
 * AnimGraph 与线程安全更新只能读取本类缓存的 Transient 字段。
 */
UCLASS(Blueprintable, BlueprintType)
class POLYGONSCIFIWORLDS_API USWAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	USWAnimInstance();

	//~ Begin UAnimInstance interface
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	//~ End UAnimInstance interface

protected:
	/** XY 平面速度长度，用于地面 Blend Space。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Locomotion")
	float GroundSpeed = 0.f;

	/** 相对角色朝向的移动方向，范围为 -180 到 180。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Locomotion")
	float Direction = 0.f;

	/** CMC 当前加速度的 XY 平面长度。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Locomotion")
	float Acceleration = 0.f;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Locomotion")
	bool bShouldMove = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Locomotion")
	bool bIsInAir = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Locomotion")
	bool bIsCrouching = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Locomotion")
	bool bIsSprinting = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Weapon")
	bool bIsAiming = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Weapon")
	bool bIsReloading = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "AimOffset")
	float AimYaw = 0.f;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "AimOffset")
	float AimPitch = 0.f;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Locomotion")
	ESWLocomotionState LocomotionState = ESWLocomotionState::Idle;

private:
	void CacheCharacterReferences();

	/** 仅游戏线程缓存，不暴露给 AnimGraph。 */
	UPROPERTY(Transient)
	TObjectPtr<ACharacter> OwningCharacter;

	UPROPERTY(Transient)
	TObjectPtr<UCharacterMovementComponent> MovementComponent;
};
