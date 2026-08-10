// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapon/SWWeaponTypes.h"
#include "SWWeapon.generated.h"

class UMeshComponent;
class USkeletalMeshComponent;
class UStaticMeshComponent;
struct FOnAttributeChangeData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSWOnWeaponAmmoChanged, int32, MagazineAmmo);

/** 固定武器 Actor：服务器是弹药、射速和弹丸生成的唯一写入者。 */
UCLASS()
class POLYGONSCIFIWORLDS_API ASWWeapon : public AActor
{
	GENERATED_BODY()

public:
	ASWWeapon();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Weapon")
	int32 GetMagazineAmmo() const { return CurrentMagazineAmmo; }

	/** 返回属性修正后的有效弹匣容量，供拥有者 HUD 只读显示。 */
	UFUNCTION(BlueprintPure, Category = "Weapon")
	int32 GetEffectiveMagazineCapacityForUI() const { return GetEffectiveMagazineCapacity(); }

	/** 返回武器配置的 HUD 图标软引用；由拥有者 UI 负责按需加载与显示。 */
	TSoftObjectPtr<class UTexture2D> GetWeaponIcon() const { return WeaponConfig.WeaponIcon; }

	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool CanFire() const;

	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool CanReload() const;

	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool SupportsAim() const { return WeaponConfig.bSupportsAim; }

	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool IsAutomatic() const { return WeaponConfig.bAutomatic; }

	/**
	 * 由首个有效 FireCycle Section 的实际时长及当前射速属性推导的有效 RPM。
	 * 仅供 UI 和调试显示；真正的射击节奏由 Montage Section 的播放推进决定。
	 */
	UFUNCTION(BlueprintPure, Category = "Weapon|Animation")
	float GetEffectiveRoundsPerMinute() const;

	/**
	 * 从武器配置中选择一次开火表现，并以属性驱动的百分比同步计算播放倍率。
	 * 仅返回内容表现数据；弹药验证和命中结算仍由 TryFireAuthority 负责。
	 */
	bool ResolveFireMontageSelection(int32 RequestedVariantIndex, FSWFireMontageSelection& OutSelection) const;

	/** 返回换弹表现使用的 Montage；为空时换弹仍按服务器时钟完成。 */
	UAnimMontage* GetReloadMontage() const { return WeaponConfig.ReloadMontage; }

	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool GetMuzzleTransform(FTransform& OutMuzzleTransform) const;

	/** 返回当前武器的瞄准相机配置；仅供受信任的瞄准 Ability 读取。 */
	bool GetAimCameraSettings(float& OutAimFOV, FVector& OutAimCameraOffset, float& OutTransitionSeconds) const;

	/** 返回由拥有者 AttributeSet 修正后的单发射击间隔。 */

	/** 返回换弹时长；动画只负责表现，Ability 以此作为权威等待时间。 */
	float GetReloadDurationSeconds() const { return WeaponConfig.ReloadDurationSeconds; }

	/** 仅由受信任的 C++ Ability 在服务器调用；失败时没有弹药和弹丸副作用。 */
	FSWResolvedShot TryFireAuthority();

	/** 仅由受信任的 C++ Ability 在服务器调用；返回实际填入当前弹匣的数量。 */
	int32 TryCommitReloadAuthority();

	/** 仅供服务器上的换弹 Ability 通知表现状态；不会修改弹药。 */
	void NotifyReloadStateChangedAuthority(bool bReloading);

	UPROPERTY(BlueprintAssignable, Category = "Weapon")
	FSWOnWeaponAmmoChanged OnAmmoChanged;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FSWWeaponConfig WeaponConfig;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<USkeletalMeshComponent> SkeletalWeaponMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UStaticMeshComponent> StaticWeaponMesh;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentMagazineAmmo)
	int32 CurrentMagazineAmmo = 0;

	UFUNCTION()
	void OnRep_CurrentMagazineAmmo(int32 OldAmmo);

	UFUNCTION(BlueprintImplementableEvent, Category = "Weapon")
	void BP_OnFireCosmetic();

	UFUNCTION(BlueprintImplementableEvent, Category = "Weapon")
	void BP_OnReloadStateChanged(bool bReloading);

private:
	UMeshComponent* GetActiveWeaponMesh() const;
	int32 GetEffectiveMagazineCapacity() const;
	float GetBaseFireCycleDurationSeconds() const;
	float GetEffectiveFireMontagePlayRate(float BasePlayRate) const;
	float GetFireIntervalMultiplier() const;
	bool IsAiming() const;
	bool BuildDamageApplicationParamsAuthority(APawn* OwnerPawn, FSWDamageApplicationParams& OutDamageParams) const;
	bool BuildAuthoritativeShotQuery(const FTransform& MuzzleTransform, FVector& OutDirection, FVector& OutTraceEnd) const;
	bool ResolveProjectileAuthority(APawn* OwnerPawn, const FTransform& MuzzleTransform, const FVector& ShotDirection, const FSWDamageApplicationParams& DamageParams);
	void ResolveHitscanAuthority(APawn* OwnerPawn, const FVector& TraceStart, const FVector& TraceEnd, const FSWDamageApplicationParams& DamageParams, FSWResolvedShot& InOutResult);
	bool ApplyDamageEffectAuthority(AActor* HitActor, const FSWDamageApplicationParams& DamageParams);
	void BroadcastAmmoChanged();
	void ExecuteOwnerGameplayCue(FGameplayTag CueTag) const;
	void BindMagazineCapacityMultiplierAuthority();
	void HandleMagazineCapacityMultiplierChanged(const FOnAttributeChangeData& ChangeData);

	/** 只存在于服务器，作为射速验证的唯一时间来源。 */
};
