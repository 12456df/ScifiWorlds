// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "Character/SWCharacter_Minion.h"

#include "AbilitySystem/Abilities/Minions/SWMinionAttackGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Data/SWCombatantDefinition.h"
#include "GameplayTags/SWGameplayTags.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Interaction/SWCombatInterface.h"
#include "Interaction/SWTeamInterface.h"
#include "Mass/Minions/SWMinionMassTypes.h"
#include "MassEntityManager.h"
#include "MassEntitySubsystem.h"
#include "Math/RotationMatrix.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SWCharacter_Minion)

ASWCharacter_Minion::ASWCharacter_Minion()
{
	PrimaryActorTick.bCanEverTick = false;
	SetReplicates(true);
	SetReplicateMovement(true);
}

void ASWCharacter_Minion::SetMassVisualVelocityAuthority(const FVector& InVelocity)
{
	if (!HasAuthority())
	{
		return;
	}

	if (UCharacterMovementComponent* const MovementComponent = GetCharacterMovement())
	{
		// 位置只由 Mass Transform 写入；这里仅提供 AActor::GatherCurrentMovement()
		// 会复制的线速度，绝不重新启用或 Tick CMC。
		MovementComponent->Velocity = InVelocity;
	}
}

void ASWCharacter_Minion::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		// 位置仅由 Mass Actor Sync 写入，禁用 CharacterMovement，避免重力/自主移动与 Mass Transform 争夺真值。
		UCharacterMovementComponent* const MovementComponent = GetCharacterMovement();
		MovementComponent->DisableMovement();
		MovementComponent->SetComponentTickEnabled(false);

		if (MinionAttackAbilityClass && AbilitySystemComponent && !MinionAttackAbilityHandle.IsValid())
		{
			MinionAttackAbilityHandle = AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(MinionAttackAbilityClass, 1));
		}
	}
}

bool ASWCharacter_Minion::InitializeMinionAuthority(const FSWMinionActorInitializationData& InitializationData)
{
	if (!HasAuthority() || HasActorBegunPlay() || bHasDeferredInitialization)
	{
		ensureMsgf(false, TEXT("小兵 %s 必须在服务器 Deferred Spawn 与 FinishSpawning 之间仅初始化一次。"), *GetName());
		return false;
	}

	if (InitializationData.UnitId.IsNone()
		|| (InitializationData.TeamId != ESWTeamId::TeamA && InitializationData.TeamId != ESWTeamId::TeamB)
		|| !InitializationData.CombatantDefinition
		|| !InitializationData.AttackAbilityClass
		|| InitializationData.AttackRange <= 0.f)
	{
		ensureMsgf(false, TEXT("小兵 %s 收到了无效的 Deferred Spawn 初始化数据。"), *GetName());
		return false;
	}

	MinionUnitId = InitializationData.UnitId;
	MinionWaveIndex = InitializationData.WaveIndex;
	TeamId = InitializationData.TeamId;
	CombatLevel = FMath::Max(1, InitializationData.CombatLevel);
	CombatantDefinition = InitializationData.CombatantDefinition;
	MinionAttackAbilityClass = InitializationData.AttackAbilityClass;
	AttackRange = InitializationData.AttackRange;
	bHasDeferredInitialization = true;
	return true;
}

ESWMinionAttackAttemptResult ASWCharacter_Minion::ValidateMinionAttackTargetAuthority(const AActor* TargetActor) const
{
	if (!HasAuthority())
	{
		return ESWMinionAttackAttemptResult::NotAuthority;
	}
	if (bDead || ISWCombatInterface::Execute_IsDead(this))
	{
		return ESWMinionAttackAttemptResult::SourceDead;
	}
	if (!IsValid(TargetActor) || TargetActor == this
		|| !TargetActor->GetClass()->ImplementsInterface(USWCombatInterface::StaticClass())
		|| !TargetActor->GetClass()->ImplementsInterface(USWTeamInterface::StaticClass()))
	{
		return ESWMinionAttackAttemptResult::InvalidTarget;
	}
	if (ISWCombatInterface::Execute_IsDead(TargetActor))
	{
		return ESWMinionAttackAttemptResult::TargetDead;
	}

	const ISWTeamInterface* const TargetTeamInterface = Cast<ISWTeamInterface>(TargetActor);
	if (!TargetTeamInterface)
	{
		return ESWMinionAttackAttemptResult::InvalidTarget;
	}
	const ESWTeamId TargetTeam = TargetTeamInterface->GetTeamId();
	if (TeamId == ESWTeamId::None || TargetTeam == ESWTeamId::None || TeamId == TargetTeam)
	{
		return ESWMinionAttackAttemptResult::SameTeam;
	}
	// 小兵只在地面兵线平面内移动；与 Targeting/Movement 保持相同的二维攻击距离，
	// 避免 Z 偏移导致 StateTree 认为已到位而权威攻击桥又拒绝命中。
	if (AttackRange <= 0.f || FVector::DistSquared2D(GetActorLocation(), TargetActor->GetActorLocation()) > FMath::Square(AttackRange))
	{
		return ESWMinionAttackAttemptResult::OutOfRange;
	}
	return ESWMinionAttackAttemptResult::Accepted;
}

ESWMinionAttackAttemptResult ASWCharacter_Minion::TryActivateMinionAttackAuthority(AActor* TargetActor)
{
	const ESWMinionAttackAttemptResult ValidationResult = ValidateMinionAttackTargetAuthority(TargetActor);
	if (ValidationResult != ESWMinionAttackAttemptResult::Accepted)
	{
		return ValidationResult;
	}

	// 攻击开始时立即面向已通过权威校验的目标。Mass Attack Processor 同时写入
	// Transform Fragment，确保下一帧 Actor Sync 不会把该朝向覆盖回旧值。
	const FVector DirectionToTarget = (TargetActor->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	if (!DirectionToTarget.IsNearlyZero())
	{
		SetActorRotation(FRotationMatrix::MakeFromXZ(DirectionToTarget, FVector::UpVector).Rotator());
	}
	if (!AbilitySystemComponent || !MinionAttackAbilityHandle.IsValid()
		|| !AbilitySystemComponent->FindAbilitySpecFromHandle(MinionAttackAbilityHandle))
	{
		return ESWMinionAttackAttemptResult::AbilityUnavailable;
	}

	FGameplayEventData EventData;
	EventData.EventTag = SWGameplayTags::Event_Combat_MinionAttack;
	EventData.Instigator = this;
	EventData.Target = TargetActor;
	return AbilitySystemComponent->HandleGameplayEvent(EventData.EventTag, &EventData) > 0
		? ESWMinionAttackAttemptResult::Accepted
		: ESWMinionAttackAttemptResult::AbilityUnavailable;
}

void ASWCharacter_Minion::CancelMinionAttackAbilityAuthority()
{
	if (HasAuthority() && AbilitySystemComponent && MinionAttackAbilityHandle.IsValid())
	{
		AbilitySystemComponent->CancelAbilityHandle(MinionAttackAbilityHandle);
	}
}

bool ASWCharacter_Minion::SetMassEntityHandleAuthority(const FMassEntityHandle& InMassEntityHandle)
{
	if (!HasAuthority() || !bHasDeferredInitialization || MassEntityHandle.IsValid() || !InMassEntityHandle.IsValid())
	{
		ensureMsgf(false, TEXT("小兵 %s 的 Mass Entity Handle 只能在服务器初始化完成后写入一次。"), *GetName());
		return false;
	}

	MassEntityHandle = InMassEntityHandle;
	return true;
}

bool ASWCharacter_Minion::TryCommitDeathAuthority(const FSWDeathContext& DeathContext)
{
	if (!Super::TryCommitDeathAuthority(DeathContext))
	{
		return false;
	}

	// 基类已经完成唯一死亡提交、奖励与 Ability 取消；这里仅把已确定的结果同步给 Mass。
	MarkMassEntityDeadAuthority();
	return true;
}

void ASWCharacter_Minion::MarkMassEntityDeadAuthority()
{
	if (!HasAuthority() || !MassEntityHandle.IsValid())
	{
		return;
	}

	UMassEntitySubsystem* const EntitySubsystem = GetWorld() ? GetWorld()->GetSubsystem<UMassEntitySubsystem>() : nullptr;
	if (!EntitySubsystem)
	{
		ensureMsgf(false, TEXT("死亡小兵 %s 所在 World 缺少 UMassEntitySubsystem。"), *GetName());
		return;
	}

	FMassEntityManager& EntityManager = EntitySubsystem->GetMutableEntityManager();
	if (!EntityManager.IsEntityValid(MassEntityHandle))
	{
		ensureMsgf(false, TEXT("死亡小兵 %s 的 Mass Entity 已失效，跳过重复回收桥接。"), *GetName());
		return;
	}

	// TryCommitDeathAuthority 只有首次死亡提交会到达此处，因此无需调用未导出的查询 API 再次检查 Tag。
	EntityManager.AddTagToEntity(MassEntityHandle, FSWMinionDeadTag::StaticStruct());
}
