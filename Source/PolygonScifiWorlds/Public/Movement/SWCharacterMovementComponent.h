// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "SWCharacterMovementComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FSWOnSprintingChanged, bool);

/**
 * 处理角色标准移动与疾跑预测的 CMC。
 *
 * 疾跑意图由 GA_Sprint 设置，随后通过 FLAG_Custom_0 随 Character Move 一同发送给服务器；
 * 该组件不管理体力、消耗或技能激活资格。
 */
UCLASS()
class POLYGONSCIFIWORLDS_API USWCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	USWCharacterMovementComponent();

	/** 普通地面移动的最大速度。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Speed", meta = (ClampMin = "0.0"))
	float WalkSpeed = 600.f;

	/** 满足疾跑资格时的最大速度。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Speed", meta = (ClampMin = "0.0"))
	float SprintSpeed = 900.f;

	/** 下蹲状态的最大速度。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Speed", meta = (ClampMin = "0.0"))
	float CrouchSpeed = 300.f;

	/** 加速度与角色前向的最小点积；低于该值时即使按住疾跑也保持步行。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Sprint", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SprintForwardThreshold = 0.1f;

	/** 由 GA_Sprint 在激活和结束时调用，写入可预测的疾跑意图。 */
	void SetSprintRequested(bool bRequested);

	/** 返回当前帧是否实际采用疾跑速度。 */
	bool IsSprinting() const;

	/** 返回 GA 写入、尚未经过移动资格验证的疾跑意图。 */
	bool IsSprintRequested() const { return bWantsToSprint; }

	/** 验证疾跑意图与当前移动状态；不处理 M05 的体力资源。 */
	bool CanSprint() const;

	/** 仅在实际疾跑状态切换时广播，供本地镜头等表现系统订阅。 */
	FSWOnSprintingChanged OnSprintingChanged;

	//~ Begin UCharacterMovementComponent interface
	virtual float GetMaxSpeed() const override;
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;
	virtual void OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity) override;
	//~ End UCharacterMovementComponent interface

private:
	void UpdateSprintingState();

	/** 此字段会被 SavedMove 捕获并写入 FLAG_Custom_0。 */
	uint8 bWantsToSprint : 1;

	/** 缓存上一帧的实际疾跑状态，避免每帧向表现层重复广播。 */
	uint8 bWasSprinting : 1;
};
