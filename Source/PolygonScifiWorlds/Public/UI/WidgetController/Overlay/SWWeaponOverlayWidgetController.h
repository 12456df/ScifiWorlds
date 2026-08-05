// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/WidgetController/SWWidgetController.h"
#include "SWWeaponOverlayWidgetController.generated.h"

class APawn;
class ASWCharacter_Player;
class ASWWeapon;
class UTexture2D;

/** 当前固定武器的拥有者专属弹药快照。 */
USTRUCT(BlueprintType)
struct FSWOverlayWeaponSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Overlay")
	int32 MagazineAmmo = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Overlay")
	int32 MagazineCapacity = 0;

	/** M04 规则为无限备弹；保留字段使 Widget 不必依赖具体武器实现。 */
	UPROPERTY(BlueprintReadOnly, Category = "Overlay")
	bool bInfiniteReserveAmmo = true;

	/** 当前武器的 HUD 图标软引用；控制器在首次快照时确保本地资源已解析。 */
	UPROPERTY(BlueprintReadOnly, Category = "Overlay")
	TSoftObjectPtr<UTexture2D> WeaponIcon;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSWOnOverlayWeaponChanged, const FSWOverlayWeaponSnapshot&, Snapshot);

/**
 * 跟随本地 PlayerController 的 Pawn 与当前武器，订阅拥有者复制的弹匣弹药。
 * 重生更换 Pawn 或武器复制到达时自动重绑，不直接修改武器状态。
 */
UCLASS(BlueprintType, Blueprintable)
class POLYGONSCIFIWORLDS_API USWWeaponOverlayWidgetController : public USWWidgetController
{
	GENERATED_BODY()

public:
	virtual void BroadcastInitialValues() override;
	virtual void BindCallbacksToDependencies() override;
	virtual void BeginDestroy() override;

	UPROPERTY(BlueprintAssignable, Category = "Overlay|Weapon")
	FSWOnOverlayWeaponChanged OnWeaponChanged;

private:
	void BindToPawn(APawn* NewPawn);
	void BindToWeapon(ASWWeapon* NewWeapon);
	void UnbindCallbacks();
	void BroadcastWeapon();

	UFUNCTION()
	void HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

	UFUNCTION()
	void HandleCurrentWeaponChanged(ASWWeapon* NewWeapon);

	UFUNCTION()
	void HandleAmmoChanged(int32 NewMagazineAmmo);

	TWeakObjectPtr<ASWCharacter_Player> BoundCharacter;
	TWeakObjectPtr<ASWWeapon> BoundWeapon;
	bool bControllerCallbackBound = false;
};
