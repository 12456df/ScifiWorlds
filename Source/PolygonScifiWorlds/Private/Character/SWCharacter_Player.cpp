// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "Character/SWCharacter_Player.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/SWAbilitySystemComponent.h"
#include "AbilitySystem/SWAttributeSet.h"
#include "Camera/CameraShakeBase.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/Controller.h"
#include "Camera/PlayerCameraManager.h"
#include "GameplayTags/SWGameplayTags.h"
#include "Input/SWInputConfig.h"
#include "InputActionValue.h"
#include "InputCoreTypes.h"
#include "Math/RotationMatrix.h"
#include "Movement/SWCharacterMovementComponent.h"
#include "Player/SWPlayerController.h"
#include "Player/SWPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Weapon/SWWeapon.h"

ASWCharacter_Player::ASWCharacter_Player(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<USWCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	USWCharacterMovementComponent* MovementComponent = GetCharacterMovement<USWCharacterMovementComponent>();
	MovementComponent->bOrientRotationToMovement = false;
	// Dedicated Server 不渲染角色，但仍须推进 Montage，才能在服务器触发权威 Gameplay Event Notify。
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickMontagesWhenNotRendered;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetRootComponent());
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->TargetArmLength = DefaultCameraArmLength;
	CameraBoom->SocketOffset = DefaultHipCameraOffset;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.TickGroup = TG_PostPhysics;
}

void ASWCharacter_Player::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	CameraBoom->TargetArmLength = DefaultCameraArmLength;
	CameraBoom->SocketOffset = DefaultHipCameraOffset;

	if (USWCharacterMovementComponent* MovementComponent = GetCharacterMovement<USWCharacterMovementComponent>())
	{
		SprintStateChangedHandle = MovementComponent->OnSprintingChanged.AddUObject(this, &ThisClass::HandleSprintingChanged);
	}
}

void ASWCharacter_Player::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocallyControlled() && CameraBoom && FollowCamera)
	{
		HipCameraFOV = FollowCamera->FieldOfView;
		HipCameraOffset = CameraBoom->SocketOffset;
		TargetCameraFOV = HipCameraFOV;
		TargetCameraOffset = HipCameraOffset;

		if (const USWCharacterMovementComponent* MovementComponent = GetCharacterMovement<USWCharacterMovementComponent>())
		{
			SetLocalSprintCameraShakeActive(MovementComponent->IsSprinting());
		}
	}

	SetActorTickEnabled(false);
}

void ASWCharacter_Player::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateLocalCamera(DeltaTime);
}

void ASWCharacter_Player::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SetLocalSprintCameraShakeActive(false);
	UnbindPlayerStateProgression();

	if (USWCharacterMovementComponent* MovementComponent = GetCharacterMovement<USWCharacterMovementComponent>(); MovementComponent && SprintStateChangedHandle.IsValid())
	{
		MovementComponent->OnSprintingChanged.Remove(SprintStateChangedHandle);
		SprintStateChangedHandle.Reset();
	}

	if (HasAuthority() && CurrentWeapon)
	{
		CurrentWeapon->Destroy();
		CurrentWeapon = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void ASWCharacter_Player::HandleSprintingChanged(const bool bIsSprinting)
{
	SetLocalSprintCameraShakeActive(bIsSprinting);
}

void ASWCharacter_Player::SetLocalSprintCameraShakeActive(const bool bActive)
{
	if (!IsLocallyControlled())
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	APlayerCameraManager* CameraManager = PlayerController ? PlayerController->PlayerCameraManager : nullptr;
	if (!CameraManager)
	{
		return;
	}

	if (bActive)
	{
		if (!ActiveSprintCameraShake && SprintCameraShakeClass)
		{
			ActiveSprintCameraShake = CameraManager->StartCameraShake(SprintCameraShakeClass);
		}
		return;
	}

	if (ActiveSprintCameraShake)
	{
		CameraManager->StopCameraShake(ActiveSprintCameraShake, true);
		ActiveSprintCameraShake = nullptr;
	}
}

void ASWCharacter_Player::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASWCharacter_Player, CurrentWeapon);
}

void ASWCharacter_Player::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// 服务器端：PlayerState 此时已就绪，完成 Owner/Avatar 绑定。
	InitAbilityActorInfo();
}

void ASWCharacter_Player::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// 拥有者客户端：PlayerState 复制到位后，执行与服务器一致的绑定。
	InitAbilityActorInfo();
}

void ASWCharacter_Player::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	const ASWPlayerController* SWPlayerController = Cast<ASWPlayerController>(GetController());
	const USWInputConfig* InputConfig = SWPlayerController ? SWPlayerController->GetInputConfig() : nullptr;
	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!InputConfig || !EnhancedInputComponent)
	{
		return;
	}

	if (InputConfig->MoveAction)
	{
		EnhancedInputComponent->BindAction(InputConfig->MoveAction, ETriggerEvent::Triggered, this, &ThisClass::Move);
	}
	if (InputConfig->LookAction)
	{
		EnhancedInputComponent->BindAction(InputConfig->LookAction, ETriggerEvent::Triggered, this, &ThisClass::Look);
	}
	if (InputConfig->JumpAction)
	{
		EnhancedInputComponent->BindAction(InputConfig->JumpAction, ETriggerEvent::Started, this, &ThisClass::StartJump);
		EnhancedInputComponent->BindAction(InputConfig->JumpAction, ETriggerEvent::Completed, this, &ThisClass::StopJump);
	}
	if (InputConfig->CrouchAction)
	{
		EnhancedInputComponent->BindAction(InputConfig->CrouchAction, ETriggerEvent::Started, this, &ThisClass::ToggleCrouch);
	}

	for (const FSWAbilityInputAction& AbilityInputAction : InputConfig->GetAbilityInputActions())
	{
		if (!AbilityInputAction.InputAction || !AbilityInputAction.InputTag.IsValid())
		{
			continue;
		}

		EnhancedInputComponent->BindAction(AbilityInputAction.InputAction, ETriggerEvent::Started, this, &ThisClass::AbilityInputTagPressed, AbilityInputAction.InputTag);
		EnhancedInputComponent->BindAction(AbilityInputAction.InputAction, ETriggerEvent::Completed, this, &ThisClass::AbilityInputTagReleased, AbilityInputAction.InputTag);
	}
}

void ASWCharacter_Player::SetLocalAimCameraSettings(const bool bAiming, const float AimFOV, const FVector AimCameraOffset, const float TransitionSeconds)
{
	if (!IsLocallyControlled() || !CameraBoom || !FollowCamera)
	{
		return;
	}

	if (!bAiming)
	{
		ClearLocalAimCameraSettings();
		return;
	}

	if (AimFOV <= 0.f || TransitionSeconds < 0.f)
	{
		return;
	}

	TargetCameraFOV = AimFOV;
	TargetCameraOffset = AimCameraOffset;
	CameraTransitionSpeed = TransitionSeconds > 0.f ? 1.f / TransitionSeconds : 0.f;

	if (CameraTransitionSpeed <= 0.f)
	{
		FollowCamera->SetFieldOfView(TargetCameraFOV);
		CameraBoom->SocketOffset = TargetCameraOffset;
		SetActorTickEnabled(false);
		return;
	}

	SetActorTickEnabled(true);
}

void ASWCharacter_Player::ClearLocalAimCameraSettings()
{
	if (!IsLocallyControlled() || !CameraBoom || !FollowCamera)
	{
		return;
	}

	TargetCameraFOV = HipCameraFOV;
	TargetCameraOffset = HipCameraOffset;

	if (CameraTransitionSpeed <= 0.f)
	{
		FollowCamera->SetFieldOfView(TargetCameraFOV);
		CameraBoom->SocketOffset = TargetCameraOffset;
		SetActorTickEnabled(false);
		return;
	}

	SetActorTickEnabled(true);
}

void ASWCharacter_Player::UpdateLocalCamera(const float DeltaTime)
{
	if (!IsLocallyControlled() || !CameraBoom || !FollowCamera || HipCameraFOV <= 0.f)
	{
		return;
	}

	if (CameraTransitionSpeed <= 0.f)
	{
		FollowCamera->SetFieldOfView(TargetCameraFOV);
		CameraBoom->SocketOffset = TargetCameraOffset;
		return;
	}

	FollowCamera->SetFieldOfView(FMath::FInterpTo(FollowCamera->FieldOfView, TargetCameraFOV, DeltaTime, CameraTransitionSpeed));
	CameraBoom->SocketOffset = FMath::VInterpTo(CameraBoom->SocketOffset, TargetCameraOffset, DeltaTime, CameraTransitionSpeed);

	if (FMath::IsNearlyEqual(FollowCamera->FieldOfView, TargetCameraFOV, 0.01f)
		&& CameraBoom->SocketOffset.Equals(TargetCameraOffset, 0.01f))
	{
		FollowCamera->SetFieldOfView(TargetCameraFOV);
		CameraBoom->SocketOffset = TargetCameraOffset;
		SetActorTickEnabled(false);
	}
}

void ASWCharacter_Player::Move(const FInputActionValue& InputActionValue)
{
	if (IsDead_Implementation())
	{
		return;
	}

	const FVector2D MovementInput = InputActionValue.Get<FVector2D>();
	if (!Controller)
	{
		return;
	}

	const FRotator ControlRotation = Controller->GetControlRotation();
	const FRotator YawRotation(0.f, ControlRotation.Yaw, 0.f);
	AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), MovementInput.Y);
	AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), MovementInput.X);
}

void ASWCharacter_Player::Look(const FInputActionValue& InputActionValue)
{
	if (IsDead_Implementation())
	{
		return;
	}

	const FVector2D LookInput = InputActionValue.Get<FVector2D>();
	AddControllerYawInput(LookInput.X);
	AddControllerPitchInput(LookInput.Y);
}

void ASWCharacter_Player::StartJump()
{
	if (IsDead_Implementation())
	{
		return;
	}

	Jump();
}

void ASWCharacter_Player::StopJump()
{
	StopJumping();
}

void ASWCharacter_Player::ToggleCrouch()
{
	if (IsDead_Implementation())
	{
		return;
	}

	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}

void ASWCharacter_Player::AbilityInputTagPressed(const FGameplayTag InputTag)
{
	if (IsDead_Implementation())
	{
		return;
	}

	if (USWAbilitySystemComponent* SWAbilitySystemComponent = Cast<USWAbilitySystemComponent>(GetAbilitySystemComponent()))
	{
		const bool bIsSkillInput = InputTag == SWGameplayTags::Ability_Input_Skill1
			|| InputTag == SWGameplayTags::Ability_Input_Skill2
			|| InputTag == SWGameplayTags::Ability_Input_Skill3;
		if (bIsSkillInput && IsAbilityUpgradeModifierDown())
		{
			// Alt+技能键是升级意图，不得同时作为该技能的施放按下输入。
			UpgradeRequestInputTags.AddTag(InputTag);
			if (ASWPlayerController* const SWPlayerController = Cast<ASWPlayerController>(GetController()))
			{
				SWPlayerController->RequestActiveAbilityUpgrade(InputTag);
			}
			return;
		}

		// 确认式施法期间，左键与第二技能键均可确认；不存在确认监听者时，第二技能键仍按普通技能输入处理。
		if (((InputTag == SWGameplayTags::Ability_Input_Fire || InputTag == SWGameplayTags::Ability_Input_Skill2)
			&& SWAbilitySystemComponent->TryConsumeGenericConfirmInput())
			|| (InputTag == SWGameplayTags::Ability_Input_Aim && SWAbilitySystemComponent->TryConsumeGenericCancelInput()))
		{
			return;
		}

		SWAbilitySystemComponent->AbilityInputTagPressed(InputTag);
	}
}

void ASWCharacter_Player::AbilityInputTagReleased(const FGameplayTag InputTag)
{
	if (IsDead_Implementation())
	{
		return;
	}

	if (UpgradeRequestInputTags.HasTagExact(InputTag))
	{
		UpgradeRequestInputTags.RemoveTag(InputTag);
		return;
	}

	if (USWAbilitySystemComponent* SWAbilitySystemComponent = Cast<USWAbilitySystemComponent>(GetAbilitySystemComponent()))
	{
		SWAbilitySystemComponent->AbilityInputTagReleased(InputTag);
	}
}

bool ASWCharacter_Player::IsAbilityUpgradeModifierDown() const
{
	const APlayerController* const PlayerController = Cast<APlayerController>(GetController());
	return PlayerController && (PlayerController->IsInputKeyDown(EKeys::LeftAlt) || PlayerController->IsInputKeyDown(EKeys::RightAlt));
}

void ASWCharacter_Player::InitAbilityActorInfo()
{
	ASWPlayerState* SWPlayerState = GetPlayerState<ASWPlayerState>();
	if (!SWPlayerState)
	{
		return;
	}

	UAbilitySystemComponent* PlayerASC = SWPlayerState->GetAbilitySystemComponent();
	if (!PlayerASC)
	{
		return;
	}

	// Owner = PlayerState（持久所有者），Avatar = 当前 Pawn。Avatar 改变时重复调用只更新绑定，
	// InitAbilityActorInfo 本身是幂等的。
	PlayerASC->InitAbilityActorInfo(SWPlayerState, this);

	// 缓存到基类指针，供 GetAbilitySystemComponent / GetAttributeSet 转发。
	AbilitySystemComponent = PlayerASC;
	AttributeSet = SWPlayerState->GetAttributeSet();

	if (HasAuthority())
	{
		BindPlayerStateProgression(SWPlayerState);
		ApplyCombatantInitializationEffectsAuthority(SWPlayerState->GetPlayerLevel(), true);

		if (USWAbilitySystemComponent* SWAbilitySystemComponent = Cast<USWAbilitySystemComponent>(PlayerASC))
		{
			SWAbilitySystemComponent->GrantStartupAbilities(StartupAbilities);
		}

		SpawnDefaultWeaponAuthority();
	}

	// 注意：基础能力授予与初始化 Gameplay Effect 只应由服务器在“首次有效绑定”后执行一次，
	// 且需显式的已初始化保护。相关逻辑依赖数据驱动的进度/初始属性配置（M03 实现顺序第 4 项 /
	// M04 的启动技能由服务器授予，并由 ASC 按 Ability Class 去重；重生后只更新 Avatar，不重复创建 Spec。
}

void ASWCharacter_Player::BindPlayerStateProgression(ASWPlayerState* const InPlayerState)
{
	if (BoundProgressionPlayerState.Get() == InPlayerState && LevelChangedHandle.IsValid())
	{
		return;
	}

	UnbindPlayerStateProgression();
	if (!InPlayerState)
	{
		return;
	}

	BoundProgressionPlayerState = InPlayerState;
	LevelChangedHandle = InPlayerState->OnLevelChanged.AddUObject(this, &ThisClass::HandlePlayerLevelChanged);
}

int32 ASWCharacter_Player::GetCombatLevel_Implementation() const
{
	const ASWPlayerState* const SWPlayerState = GetPlayerState<ASWPlayerState>();
	return SWPlayerState ? SWPlayerState->GetPlayerLevel() : 1;
}

void ASWCharacter_Player::UnbindPlayerStateProgression()
{
	if (ASWPlayerState* const BoundPlayerState = BoundProgressionPlayerState.Get(); BoundPlayerState && LevelChangedHandle.IsValid())
	{
		BoundPlayerState->OnLevelChanged.Remove(LevelChangedHandle);
	}

	LevelChangedHandle.Reset();
	BoundProgressionPlayerState.Reset();
}

void ASWCharacter_Player::HandlePlayerLevelChanged(const int32 NewLevel)
{
	if (!HasAuthority() || !AttributeSet)
	{
		return;
	}

	// 升级不免费回满资源：先记录比例，应用新的最大值后恢复相同比例。
	const float HealthPercent = AttributeSet->GetMaxHealth() > 0.f ? AttributeSet->GetHealth() / AttributeSet->GetMaxHealth() : 0.f;
	const float ManaPercent = AttributeSet->GetMaxMana() > 0.f ? AttributeSet->GetMana() / AttributeSet->GetMaxMana() : 0.f;
	const float StaminaPercent = AttributeSet->GetMaxStamina() > 0.f ? AttributeSet->GetStamina() / AttributeSet->GetMaxStamina() : 0.f;

	ApplyCombatantInitializationEffectsAuthority(NewLevel, false);

	AttributeSet->SetHealth(FMath::Clamp(AttributeSet->GetMaxHealth() * HealthPercent, 0.f, AttributeSet->GetMaxHealth()));
	AttributeSet->SetMana(FMath::Clamp(AttributeSet->GetMaxMana() * ManaPercent, 0.f, AttributeSet->GetMaxMana()));
	AttributeSet->SetStamina(FMath::Clamp(AttributeSet->GetMaxStamina() * StaminaPercent, 0.f, AttributeSet->GetMaxStamina()));
}

void ASWCharacter_Player::SpawnDefaultWeaponAuthority()
{
	if (!HasAuthority() || CurrentWeapon || !DefaultWeaponClass || !GetWorld() || !GetMesh())
	{
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.Instigator = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ASWWeapon* SpawnedWeapon = GetWorld()->SpawnActor<ASWWeapon>(DefaultWeaponClass, GetMesh()->GetComponentTransform(), SpawnParameters);
	if (!SpawnedWeapon)
	{
		UE_LOG(LogTemp, Error, TEXT("角色 %s 未能生成默认武器。"), *GetName());
		return;
	}

	if (!WeaponAttachSocket.IsNone() && GetMesh()->DoesSocketExist(WeaponAttachSocket))
	{
		SpawnedWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponAttachSocket);
	}
	else
	{
		SpawnedWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepWorldTransform);
		UE_LOG(LogTemp, Warning, TEXT("角色 %s 的 WeaponAttachSocket 无效，默认武器已附着到角色根部。"), *GetName());
	}

	CurrentWeapon = SpawnedWeapon;
	ForceNetUpdate();
	OnCurrentWeaponChanged.Broadcast(CurrentWeapon);
	BP_OnWeaponReady(CurrentWeapon);
}

void ASWCharacter_Player::OnRep_CurrentWeapon()
{
	OnCurrentWeaponChanged.Broadcast(CurrentWeapon);
	BP_OnWeaponReady(CurrentWeapon);
}
