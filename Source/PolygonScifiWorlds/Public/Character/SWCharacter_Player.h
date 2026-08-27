// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/SWAbilityTypes.h"
#include "Character/SWCharacter_Base.h"
#include "GameplayTagContainer.h"
#include "SWCharacter_Player.generated.h"

class ASWPlayerState;
class ASWWeapon;
class UPrimitiveComponent;
class USphereComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSWOnCurrentWeaponChanged, ASWWeapon*, NewWeapon);

/**
 * 玩家可操控角色。
 *
 * ASC 与 AttributeSet 归属 ASWPlayerState（跨重生存活），本类只作为 ASC 的 Avatar。
 * 绑定在两端触发：服务器在 PossessedBy 后、拥有者客户端在 OnRep_PlayerState 后各绑定一次。
 */
UCLASS()
class POLYGONSCIFIWORLDS_API ASWCharacter_Player : public ASWCharacter_Base
{
	GENERATED_BODY()

public:
	ASWCharacter_Player(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual int32 GetCombatLevel_Implementation() const override;

	//~ Begin AActor/APawn interface
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PostInitializeComponents() override;
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~ End AActor/APawn interface

	/** 供 C++ Ability 或武器系统设置仅拥有者本地的瞄准相机目标。 */
	void SetLocalAimCameraSettings(bool bAiming, float AimFOV, FVector AimCameraOffset, float TransitionSeconds);

	/** 清除瞄准相机目标，平滑恢复角色蓝图配置的肩射相机。 */
	void ClearLocalAimCameraSettings();

	/** 返回当前 Pawn 的唯一固定武器；能力只读取得后仍须由服务器执行写入。 */
	ASWWeapon* GetCurrentWeapon() const { return CurrentWeapon; }

	/** 当前武器在服务器创建或客户端复制到达时广播，供只读 HUD 重新订阅弹药。 */
	UPROPERTY(BlueprintAssignable, Category = "Weapon")
	FSWOnCurrentWeaponChanged OnCurrentWeaponChanged;

protected:
	virtual void BeginPlay() override;
	virtual void InitAbilityActorInfo() override;

	/** 第三人称肩后相机臂；蓝图可配置长度、碰撞和基础偏移。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class USpringArmComponent> CameraBoom;

	/** 最终本地视角；不复制 Transform。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UCameraComponent> FollowCamera;

	/**
	 * 仅所属客户端启用的三档头顶血条范围球。它们只触发已声明为 HealthBarRangeProbe
	 * 响应对象的角色胶囊和结构网格，不参与服务器战斗或物理碰撞。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI|Target Health Bar", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> HealthBarNearRangeProbe;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI|Target Health Bar", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> HealthBarMiddleRangeProbe;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI|Target Health Bar", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> HealthBarFarRangeProbe;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Target Health Bar", meta = (AllowPrivateAccess = "true", ClampMin = "1.0", Units = "cm"))
	float HealthBarNearRange = 800.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Target Health Bar", meta = (AllowPrivateAccess = "true", ClampMin = "1.0", Units = "cm"))
	float HealthBarMiddleRange = 1600.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Target Health Bar", meta = (AllowPrivateAccess = "true", ClampMin = "1.0", Units = "cm"))
	float HealthBarFarRange = 2500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Target Health Bar", meta = (AllowPrivateAccess = "true", ClampMin = "0.1", ClampMax = "1.0"))
	float HealthBarMiddleScale = 0.75f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Target Health Bar", meta = (AllowPrivateAccess = "true", ClampMin = "0.1", ClampMax = "1.0"))
	float HealthBarFarScale = 0.55f;

	/** 默认第三人称镜头臂长度；角色蓝图可按具体角色覆盖。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Hip", meta = (ClampMin = "0.0"))
	float DefaultCameraArmLength = 260.f;

	/** 默认右肩镜头偏移；角色蓝图可按具体角色覆盖。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Hip")
	FVector DefaultHipCameraOffset = FVector(0.f, 55.f, 20.f);

	/** 仅拥有者本地播放的疾跑镜头震动类；不复制镜头 Transform。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Sprint")
	TSubclassOf<class UCameraShakeBase> SprintCameraShakeClass;

	/** 由角色蓝图默认值定义的启动技能及其输入路由。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
	TArray<FSWStartupAbility> StartupAbilities;

	/** 由角色蓝图指定的唯一固定武器类型；仅服务器生成。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<class ASWWeapon> DefaultWeaponClass;

	/** 武器附着到角色 Skeletal Mesh 的 Socket 名称。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName WeaponAttachSocket = NAME_None;

	/** 当前 Pawn 的唯一固定武器；重生时由新 Pawn 创建。 */
	UPROPERTY(ReplicatedUsing = OnRep_CurrentWeapon, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<class ASWWeapon> CurrentWeapon;

	/** 使用 Controller Yaw 将 Axis2D 输入转换为前后和左右移动。 */
	void Move(const struct FInputActionValue& InputActionValue);

	/** 将鼠标二维输入转换为 Controller Yaw/Pitch。 */
	void Look(const struct FInputActionValue& InputActionValue);

	void StartJump();
	void StopJump();
	void ToggleCrouch();
	void AbilityInputTagPressed(FGameplayTag InputTag);
	void AbilityInputTagReleased(FGameplayTag InputTag);

	/** 仅服务器调用；创建并附着当前 Pawn 的唯一固定武器。 */
	void SpawnDefaultWeaponAuthority();

	UFUNCTION()
	void OnRep_CurrentWeapon();

	UFUNCTION(BlueprintImplementableEvent, Category = "Weapon")
	void BP_OnWeaponReady(class ASWWeapon* Weapon);

private:
	/** 仅服务器调用：升级后按资源百分比重新应用等级属性，避免免费回满资源。 */
	void HandlePlayerLevelChanged(int32 NewLevel);

	/** 将当前 Pawn 订阅到其 PlayerState 的等级事件；重生时会自动解除旧订阅。 */
	void BindPlayerStateProgression(ASWPlayerState* InPlayerState);
	void UnbindPlayerStateProgression();

	void UpdateLocalCamera(float DeltaTime);
	void HandleSprintingChanged(bool bIsSprinting);
	void SetLocalSprintCameraShakeActive(bool bActive);
	bool IsAbilityUpgradeModifierDown() const;
	void ConfigureLocalHealthBarRangeProbes();
	void RefreshHealthBarRangeForActor(AActor& CandidateActor) const;

	UFUNCTION()
	void HandleHealthBarRangeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleHealthBarRangeEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex);

	float HipCameraFOV = 0.f;
	FVector HipCameraOffset = FVector::ZeroVector;
	float TargetCameraFOV = 0.f;
	FVector TargetCameraOffset = FVector::ZeroVector;
	float CameraTransitionSpeed = 0.f;
	TObjectPtr<class UCameraShakeBase> ActiveSprintCameraShake;
	FDelegateHandle SprintStateChangedHandle;
	TWeakObjectPtr<ASWPlayerState> BoundProgressionPlayerState;
	FDelegateHandle LevelChangedHandle;
	/** Alt+技能键已作为升级请求消费，释放时不可再转发给对应 Ability。 */
	FGameplayTagContainer UpgradeRequestInputTags;
};
