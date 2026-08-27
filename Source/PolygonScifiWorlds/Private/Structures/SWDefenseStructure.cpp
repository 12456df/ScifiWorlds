// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "Structures/SWDefenseStructure.h"

#include "AbilitySystem/SWAbilitySystemComponent.h"
#include "AbilitySystem/SWAttributeSet.h"
#include "AbilitySystem/SWGameplayEffect.h"
#include "AI/Structures/SWStructureAIController.h"
#include "AbilitySystem/Data/SWCombatantDefinition.h"
#include "Collision/SWCollisionChannels.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "GameplayAbilitySpec.h"
#include "GameplayTagContainer.h"
#include "GameplayTags/SWGameplayTags.h"
#include "Net/UnrealNetwork.h"
#include "Player/SWPlayerState.h"
#include "Structures/SWStructureDefinition.h"
#include "Structures/SWStructureObjectiveSubsystem.h"
#include "Structures/SWStructureTargetingComponent.h"
#include "Combat/Targeting/SWCombatTargetRegistrySubsystem.h"
#include "UI/World/SWTargetHealthBarComponent.h"

ASWDefenseStructure::ASWDefenseStructure()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicateMovement(false);
	SetNetUpdateFrequency(10.f);
	SetMinNetUpdateFrequency(2.f);
	AIControllerClass = ASWStructureAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	USceneComponent* const StructureRoot = CreateDefaultSubobject<USceneComponent>(TEXT("StructureRoot"));
	SetRootComponent(StructureRoot);

	StructureMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StructureMesh"));
	StructureMesh->SetupAttachment(StructureRoot);
	// 没有自定义 Structure Profile 前，先以标准静态阻挡体作为角色移动、武器命中和塔火球的共同受击 Primitive。
	StructureMesh->SetCollisionProfileName(TEXT("BlockAll"));
	// 仅对本地玩家血条范围球产生 Query Overlap；不改变结构的物理阻挡和武器受击行为。
	StructureMesh->SetCollisionResponseToChannel(SWCollisionChannels::HealthBarRangeProbe, ECR_Overlap);
	StructureMesh->SetGenerateOverlapEvents(true);

	// 与角色复用同一套本地受击血条逻辑。塔模型高度不统一，具体 Z 轴位置由蓝图子类设置。
	TargetHealthBarComponent = CreateDefaultSubobject<USWTargetHealthBarComponent>(TEXT("TargetHealthBarComponent"));
	TargetHealthBarComponent->SetupAttachment(StructureMesh);

	AttackOrigin = CreateDefaultSubobject<USceneComponent>(TEXT("AttackOrigin"));
	AttackOrigin->SetupAttachment(StructureMesh);

	CombatRange = CreateDefaultSubobject<USphereComponent>(TEXT("CombatRange"));
	CombatRange->SetupAttachment(StructureRoot);
	CombatRange->SetSphereRadius(1000.f);
	CombatRange->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CombatRange->SetGenerateOverlapEvents(false);
	CombatRange->SetCollisionResponseToAllChannels(ECR_Ignore);

	TargetingComponent = CreateDefaultSubobject<USWStructureTargetingComponent>(TEXT("TargetingComponent"));

	AbilitySystemComponent = CreateDefaultSubobject<USWAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	AttributeSet = CreateDefaultSubobject<USWAttributeSet>(TEXT("AttributeSet"));
}

void ASWDefenseStructure::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshCombatRangeFromDefinition();
}

void ASWDefenseStructure::BeginPlay()
{
	Super::BeginPlay();

	RefreshCombatRangeFromDefinition();
	InitAbilityActorInfo();

	if (HasAuthority())
	{
		if (TeamId != ESWTeamId::TeamA && TeamId != ESWTeamId::TeamB)
		{
			UE_LOG(LogTemp, Error, TEXT("结构 %s 未配置有效 TeamId。"), *GetName());
		}

		AbilitySystemComponent->SetTeamIdTagAuthority(TeamId);
		SetVulnerableAuthority(false);
		ApplyInitializationEffectsAuthority();
		GrantAttackAbilityAuthority();

		// Step 4 的临时明确查询配置只接受 Pawn；Step 8 会由项目 Profile 固化到蓝图/碰撞设置。
		CombatRange->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		CombatRange->SetCollisionObjectType(ECC_WorldDynamic);
		CombatRange->SetCollisionResponseToAllChannels(ECR_Ignore);
		CombatRange->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		CombatRange->SetGenerateOverlapEvents(true);

		if (USWCombatTargetRegistrySubsystem* const TargetRegistry = GetWorld()->GetSubsystem<USWCombatTargetRegistrySubsystem>())
		{
			TargetRegistry->RegisterTarget(*this);
		}

		TargetingComponent->InitializeAuthority();

		if (USWStructureObjectiveSubsystem* const ObjectiveSubsystem = GetWorld()->GetSubsystem<USWStructureObjectiveSubsystem>())
		{
			ObjectiveSubsystem->RegisterStructure(*this);
		}
	}
}

void ASWDefenseStructure::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HasAuthority())
	{
		if (USWCombatTargetRegistrySubsystem* const TargetRegistry = GetWorld() ? GetWorld()->GetSubsystem<USWCombatTargetRegistrySubsystem>() : nullptr)
		{
			TargetRegistry->UnregisterTarget(*this);
		}

		if (USWStructureObjectiveSubsystem* const ObjectiveSubsystem = GetWorld() ? GetWorld()->GetSubsystem<USWStructureObjectiveSubsystem>() : nullptr)
		{
			ObjectiveSubsystem->UnregisterStructure(*this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void ASWDefenseStructure::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASWDefenseStructure, TeamId);
	DOREPLIFETIME(ASWDefenseStructure, bDead);
	DOREPLIFETIME(ASWDefenseStructure, bVulnerable);
}

UAbilitySystemComponent* ASWDefenseStructure::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

int32 ASWDefenseStructure::GetCombatLevel_Implementation() const
{
	return StructureDefinition ? FMath::Max(1, StructureDefinition->CombatLevel) : 1;
}

bool ASWDefenseStructure::IsDead_Implementation() const
{
	return bDead;
}

bool ASWDefenseStructure::IsTargetableBy(const AActor* Requestor) const
{
	static_cast<void>(Requestor);

	// Requestor 的敌我、距离和比赛阶段校验由调用方及后续 Damage Policy 负责；此接口只表达结构自身的推进门槛。
	return bVulnerable && !bDead;
}

FSWDamageReceptionResult ASWDefenseStructure::EvaluateDamageReceptionAuthority(const FSWDamageReceptionQuery& Query) const
{
	// ExecCalc 仅在服务器目标 ASC 上调用；结构仍在接口边界复核，避免其他入口绕过推进锁定。
	if (!HasAuthority() || bDead || !bVulnerable || !StructureDefinition)
	{
		return FSWDamageReceptionResult::Reject(ESWDamageReceptionRejectionReason::TargetUnavailable);
	}

	const AActor* const SourceAvatar = Query.SourceAvatar;
	if (!IsValid(SourceAvatar) || Query.ServerSourceLocation.ContainsNaN())
	{
		return FSWDamageReceptionResult::Reject(ESWDamageReceptionRejectionReason::InvalidSource);
	}

	const ISWTeamInterface* const SourceTeamProvider = Cast<ISWTeamInterface>(SourceAvatar);
	const ESWTeamId SourceTeamId = SourceTeamProvider ? SourceTeamProvider->GetTeamId() : ESWTeamId::None;
	if (TeamId == ESWTeamId::None || SourceTeamId == ESWTeamId::None || SourceTeamId == TeamId)
	{
		return FSWDamageReceptionResult::Reject(ESWDamageReceptionRejectionReason::InvalidTeam);
	}

	if (Query.DamageType != SWGameplayTags::Damage_Type_Physical)
	{
		return FSWDamageReceptionResult::Reject(ESWDamageReceptionRejectionReason::UnsupportedDamageType);
	}

	const float CombatRadius = FMath::Max(0.f, StructureDefinition->CombatRadius);
	if (CombatRadius <= 0.f
		|| FVector::DistSquared2D(Query.ServerSourceLocation, GetActorLocation()) > FMath::Square(CombatRadius))
	{
		return FSWDamageReceptionResult::Reject(ESWDamageReceptionRejectionReason::SourceOutOfRange);
	}

	const float DamageReduction = FMath::Clamp(StructureDefinition->DamageReductionPercent, 0.f, 1.f);
	return FSWDamageReceptionResult::Accept(1.f - DamageReduction);
}

bool ASWDefenseStructure::TryCommitDeathAuthority(const FSWDeathContext& DeathContext)
{
	if (!HasAuthority() || bDead || !AbilitySystemComponent)
	{
		return false;
	}

	// 先写入复制真值，再取消 Ability，避免伤害回调或 Ability 结束回调重入时重复提交死亡。
	bDead = true;
	AbilitySystemComponent->SetDeadStateTagAuthority(true);
	CancelStructureAttackAbilityAuthority();

	FGameplayTagContainer SurviveDeathTags;
	SurviveDeathTags.AddTag(SWGameplayTags::Ability_Behavior_SurviveDeath);
	AbilitySystemComponent->CancelAbilities(nullptr, &SurviveDeathTags);

	GrantDeathRewardsAuthority(DeathContext);
	OnDeath.Broadcast(DeathContext);
	ApplyDeathStatePresentation();
	ForceNetUpdate();
	return true;
}

void ASWDefenseStructure::GrantDeathRewardsAuthority(const FSWDeathContext& DeathContext)
{
	const USWCombatantDefinition* const CombatantDefinition = StructureDefinition
		? StructureDefinition->CombatantDefinition
		: nullptr;
	if (!HasAuthority() || StructureKind != ESWStructureKind::Tower
		|| !AbilitySystemComponent || !CombatantDefinition || !DeathContext.InstigatorActor)
	{
		return;
	}

	const IAbilitySystemInterface* const KillerAbilitySystemInterface = Cast<IAbilitySystemInterface>(DeathContext.InstigatorActor);
	USWAbilitySystemComponent* const KillerAbilitySystemComponent = KillerAbilitySystemInterface
		? Cast<USWAbilitySystemComponent>(KillerAbilitySystemInterface->GetAbilitySystemComponent())
		: nullptr;
	if (!KillerAbilitySystemComponent || KillerAbilitySystemComponent == AbilitySystemComponent
		|| USWGameplayEffect::AreAbilitySystemComponentsOnSameTeam(KillerAbilitySystemComponent, AbilitySystemComponent))
	{
		return;
	}

	// 固定结构首版奖励只归属最后一击玩家；小兵、环境或其他非玩家 ASC 不参与结算。
	ASWPlayerState* const KillerPlayerState = Cast<ASWPlayerState>(KillerAbilitySystemComponent->GetOwnerActor());
	if (!KillerPlayerState)
	{
		return;
	}

	const int32 CombatLevel = FMath::Max(1, GetCombatLevel_Implementation());
	const float ExperienceRewardFloat = CombatantDefinition->XPRewardByLevel.GetValueAtLevel(CombatLevel);
	if (FMath::IsFinite(ExperienceRewardFloat) && ExperienceRewardFloat > 0.f)
	{
		const int32 ExperienceReward = ExperienceRewardFloat >= static_cast<float>(MAX_int32)
			? MAX_int32
			: FMath::FloorToInt(ExperienceRewardFloat);
		KillerAbilitySystemComponent->ApplyExperienceRewardToSelfAuthority(ExperienceReward, this);
	}

	const float GoldRewardFloat = CombatantDefinition->GoldRewardByLevel.GetValueAtLevel(CombatLevel);
	if (!FMath::IsFinite(GoldRewardFloat) || GoldRewardFloat <= 0.f)
	{
		return;
	}

	const int32 GoldReward = GoldRewardFloat >= static_cast<float>(MAX_int32)
		? MAX_int32
		: FMath::FloorToInt(GoldRewardFloat);
	KillerPlayerState->GrantGoldAuthority(GoldReward);
}

ESWTeamId ASWDefenseStructure::GetTeamId() const
{
	return TeamId;
}

void ASWDefenseStructure::InitAbilityActorInfo()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
}

void ASWDefenseStructure::ApplyInitializationEffectsAuthority()
{
	if (!HasAuthority() || bInitializationEffectsApplied || !AbilitySystemComponent || !StructureDefinition)
	{
		return;
	}

	const USWCombatantDefinition* const CombatantDefinition = StructureDefinition->CombatantDefinition;
	if (!CombatantDefinition)
	{
		UE_LOG(LogTemp, Error, TEXT("结构 %s 的 StructureDefinition 未配置 CombatantDefinition。"), *GetName());
		return;
	}

	const int32 EffectLevel = GetCombatLevel_Implementation();
	const auto ApplyEffect = [this, EffectLevel](const TSubclassOf<UGameplayEffect> EffectClass, const TCHAR* EffectLabel)
	{
		if (!EffectClass)
		{
			UE_LOG(LogTemp, Error, TEXT("结构 %s 的 CombatantDefinition 未配置%s。"), *GetName(), EffectLabel);
			return false;
		}

		FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
		EffectContext.AddSourceObject(this);
		const FGameplayEffectSpecHandle EffectSpec = AbilitySystemComponent->MakeOutgoingSpec(EffectClass, EffectLevel, EffectContext);
		if (!EffectSpec.IsValid())
		{
			UE_LOG(LogTemp, Error, TEXT("结构 %s 无法创建%s的 Gameplay Effect Spec。"), *GetName(), EffectLabel);
			return false;
		}

		AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*EffectSpec.Data.Get());
		return true;
	};

	const bool bLevelApplied = ApplyEffect(CombatantDefinition->LevelAttributesEffect, TEXT("等级属性 GE"));
	const bool bVitalApplied = ApplyEffect(CombatantDefinition->VitalAttributesEffect, TEXT("满资源 GE"));

	bool bRegenerationApplied = true;
	if (CombatantDefinition->ResourceRegenerationEffect)
	{
		bRegenerationApplied = ApplyEffect(CombatantDefinition->ResourceRegenerationEffect, TEXT("资源自然恢复 GE"));
	}

	// 任一必需初始化失败时保留 false，避免隐藏配置错误；修复资产后重新进入 PIE 即可重试。
	bInitializationEffectsApplied = bLevelApplied && bVitalApplied && bRegenerationApplied;
}

void ASWDefenseStructure::GrantAttackAbilityAuthority()
{
	if (!HasAuthority() || !AbilitySystemComponent || !StructureDefinition || StructureAttackAbilityHandle.IsValid())
	{
		return;
	}

	if (!StructureDefinition->AttackAbilityClass)
	{
		UE_LOG(LogTemp, Error, TEXT("结构 %s 的 StructureDefinition 未配置 AttackAbilityClass。"), *GetName());
		return;
	}

	StructureAttackAbilityHandle = AbilitySystemComponent->GiveAbility(
		FGameplayAbilitySpec(StructureDefinition->AttackAbilityClass, GetCombatLevel_Implementation()));
}

bool ASWDefenseStructure::TryActivateStructureAttackAbilityAuthority(AActor* TargetActor)
{
	if (!HasAuthority() || bDead || !AbilitySystemComponent || !StructureAttackAbilityHandle.IsValid()
		|| !AbilitySystemComponent->FindAbilitySpecFromHandle(StructureAttackAbilityHandle))
	{
		return false;
	}

	USWStructureTargetingComponent* const Targeting = TargetingComponent;
	if (!IsValid(TargetActor) || !Targeting || Targeting->GetCurrentTargetAuthority() != TargetActor)
	{
		return false;
	}

	FGameplayEventData EventData;
	EventData.EventTag = SWGameplayTags::Event_Combat_StructureAttack;
	EventData.Instigator = this;
	EventData.Target = TargetActor;
	return AbilitySystemComponent->HandleGameplayEvent(EventData.EventTag, &EventData) > 0;
}

void ASWDefenseStructure::CancelStructureAttackAbilityAuthority()
{
	if (HasAuthority() && AbilitySystemComponent && StructureAttackAbilityHandle.IsValid())
	{
		AbilitySystemComponent->CancelAbilityHandle(StructureAttackAbilityHandle);
	}
}

void ASWDefenseStructure::RefreshCombatRangeFromDefinition()
{
	if (CombatRange && StructureDefinition)
	{
		CombatRange->SetSphereRadius(FMath::Max(1.f, StructureDefinition->CombatRadius));
	}
}

void ASWDefenseStructure::SetVulnerableAuthority(const bool bNewVulnerable)
{
	check(HasAuthority());

	const bool bEffectiveVulnerable = bNewVulnerable && !bDead;
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->SetInvulnerableStateTagAuthority(!bEffectiveVulnerable);
	}

	if (bVulnerable == bEffectiveVulnerable)
	{
		return;
	}

	bVulnerable = bEffectiveVulnerable;
	ForceNetUpdate();

	if (GetNetMode() != NM_DedicatedServer)
	{
		BP_OnVulnerableStateChanged(bVulnerable);
	}
}

void ASWDefenseStructure::OnRep_Dead()
{
	ApplyDeathStatePresentation();
}

void ASWDefenseStructure::OnRep_Vulnerable()
{
	if (GetNetMode() != NM_DedicatedServer)
	{
		BP_OnVulnerableStateChanged(bVulnerable);
	}
}

void ASWDefenseStructure::ApplyDeathStatePresentation()
{
	// Dedicated Server 不加载死亡表现；客户端与 Standalone 只消费复制的死亡真值。
	if (GetNetMode() != NM_DedicatedServer)
	{
		BP_OnDeathStateChanged(bDead);
	}
}
