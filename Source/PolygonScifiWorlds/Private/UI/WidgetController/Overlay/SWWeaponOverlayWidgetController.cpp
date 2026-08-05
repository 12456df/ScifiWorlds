// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "UI/WidgetController/Overlay/SWWeaponOverlayWidgetController.h"

#include "Character/SWCharacter_Player.h"
#include "GameFramework/PlayerController.h"
#include "Player/SWPlayerController.h"
#include "Weapon/SWWeapon.h"

void USWWeaponOverlayWidgetController::BroadcastInitialValues()
{
	BroadcastWeapon();
}

void USWWeaponOverlayWidgetController::BindCallbacksToDependencies()
{
	UnbindCallbacks();
	if (!PlayerController)
	{
		return;
	}

	PlayerController->OnPossessedPawnChanged.AddDynamic(this, &ThisClass::HandlePossessedPawnChanged);
	bControllerCallbackBound = true;
	BindToPawn(PlayerController->GetPawn());
}

void USWWeaponOverlayWidgetController::BeginDestroy()
{
	UnbindCallbacks();
	Super::BeginDestroy();
}

void USWWeaponOverlayWidgetController::BindToPawn(APawn* NewPawn)
{
	if (ASWCharacter_Player* PreviousCharacter = BoundCharacter.Get())
	{
		PreviousCharacter->OnCurrentWeaponChanged.RemoveDynamic(this, &ThisClass::HandleCurrentWeaponChanged);
	}

	BoundCharacter = Cast<ASWCharacter_Player>(NewPawn);
	if (ASWCharacter_Player* Character = BoundCharacter.Get())
	{
		Character->OnCurrentWeaponChanged.AddDynamic(this, &ThisClass::HandleCurrentWeaponChanged);
		BindToWeapon(Character->GetCurrentWeapon());
	}
	else
	{
		BindToWeapon(nullptr);
	}
}

void USWWeaponOverlayWidgetController::BindToWeapon(ASWWeapon* NewWeapon)
{
	if (ASWWeapon* PreviousWeapon = BoundWeapon.Get())
	{
		PreviousWeapon->OnAmmoChanged.RemoveDynamic(this, &ThisClass::HandleAmmoChanged);
	}

	BoundWeapon = NewWeapon;
	if (ASWWeapon* Weapon = BoundWeapon.Get())
	{
		Weapon->OnAmmoChanged.AddDynamic(this, &ThisClass::HandleAmmoChanged);
	}

	BroadcastWeapon();
}

void USWWeaponOverlayWidgetController::UnbindCallbacks()
{
	if (bControllerCallbackBound && PlayerController)
	{
		PlayerController->OnPossessedPawnChanged.RemoveDynamic(this, &ThisClass::HandlePossessedPawnChanged);
	}

	if (ASWCharacter_Player* Character = BoundCharacter.Get())
	{
		Character->OnCurrentWeaponChanged.RemoveDynamic(this, &ThisClass::HandleCurrentWeaponChanged);
	}

	if (ASWWeapon* Weapon = BoundWeapon.Get())
	{
		Weapon->OnAmmoChanged.RemoveDynamic(this, &ThisClass::HandleAmmoChanged);
	}

	BoundCharacter.Reset();
	BoundWeapon.Reset();
	bControllerCallbackBound = false;
}

void USWWeaponOverlayWidgetController::BroadcastWeapon()
{
	FSWOverlayWeaponSnapshot Snapshot;
	if (const ASWWeapon* Weapon = BoundWeapon.Get())
	{
		Snapshot.MagazineAmmo = Weapon->GetMagazineAmmo();
		Snapshot.MagazineCapacity = Weapon->GetEffectiveMagazineCapacityForUI();
		Snapshot.WeaponIcon = Weapon->GetWeaponIcon();

		// 图标仅在当前武器切换或首次 HUD 快照时加载一次，保证蓝图可直接解析软引用。
		if (!Snapshot.WeaponIcon.IsNull())
		{
			Snapshot.WeaponIcon.LoadSynchronous();
		}
	}

	OnWeaponChanged.Broadcast(Snapshot);
}

void USWWeaponOverlayWidgetController::HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	(void)OldPawn;
	BindToPawn(NewPawn);
}

void USWWeaponOverlayWidgetController::HandleCurrentWeaponChanged(ASWWeapon* NewWeapon)
{
	BindToWeapon(NewWeapon);
}

void USWWeaponOverlayWidgetController::HandleAmmoChanged(const int32 NewMagazineAmmo)
{
	(void)NewMagazineAmmo;
	BroadcastWeapon();
}
