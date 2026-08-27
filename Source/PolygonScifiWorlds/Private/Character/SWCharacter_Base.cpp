// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "Character/SWCharacter_Base.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/SWAbilitySystemComponent.h"
#include "AbilitySystem/Data/SWCombatantDefinition.h"
#include "AbilitySystem/SWGameplayEffect.h"
#include "AbilitySystem/SWAttributeSet.h"
#include "Collision/SWCollisionChannels.h"
#include "Components/CapsuleComponent.h"
#include "GameplayEffect.h"
#include "GameplayTagContainer.h"
#include "GameplayTags/SWGameplayTags.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Combat/Targeting/SWCombatTargetRegistrySubsystem.h"
#include "Net/UnrealNetwork.h"
#include "Player/SWPlayerState.h"
#include "UI/World/SWTargetHealthBarComponent.h"

ASWCharacter_Base::ASWCharacter_Base(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;

	// 只响应本地玩家的表现范围球；默认通道为 Ignore，不会影响角色的真实移动或战斗碰撞。
	GetCapsuleComponent()->SetCollisionResponseToChannel(SWCollisionChannels::HealthBarRangeProbe, ECR_Overlap);
	GetCapsuleComponent()->SetGenerateOverlapEvents(true);

	// 默认放在胶囊体顶端上方；不同体型由派生 Character 蓝图只调整 Relative Location。
	TargetHealthBarComponent = CreateDefaultSubobject<USWTargetHealthBarComponent>(TEXT("TargetHealthBarComponent"));
	TargetHealthBarComponent->SetupAttachment(GetRootComponent());
	TargetHealthBarComponent->SetRelativeLocation(FVector(0.f, 0.f, 140.f));
}

void ASWCharacter_Base::BeginPlay()
{
	Super::BeginPlay();

	// 目标索引只存在于服务器 World；注册并不转移 Team/Health/Death 的所有权。
	if (HasAuthority())
	{
		if (USWCombatTargetRegistrySubsystem* const TargetRegistry = GetWorld()->GetSubsystem<USWCombatTargetRegistrySubsystem>())
		{
			TargetRegistry->RegisterTarget(*this);
		}
	}
}

void ASWCharacter_Base::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HasAuthority())
	{
		if (USWCombatTargetRegistrySubsystem* const TargetRegistry = GetWorld() ? GetWorld()->GetSubsystem<USWCombatTargetRegistrySubsystem>() : nullptr)
		{
			TargetRegistry->UnregisterTarget(*this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

UAbilitySystemComponent* ASWCharacter_Base::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ASWCharacter_Base::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASWCharacter_Base, bDead);
}

int32 ASWCharacter_Base::GetCombatLevel_Implementation() const
{
	return 1;
}

bool ASWCharacter_Base::IsDead_Implementation() const
{
	return bDead;
}

ESWTeamId ASWCharacter_Base::GetTeamId() const
{
	const ASWPlayerState* const SWPlayerState = GetPlayerState<ASWPlayerState>();
	return SWPlayerState ? SWPlayerState->GetTeamId() : ESWTeamId::None;
}

bool ASWCharacter_Base::TryCommitDeathAuthority(const FSWDeathContext& DeathContext)
{
	if (!HasAuthority() || bDead || !AbilitySystemComponent)
	{
		return false;
	}

	// 先写入持久真值，再收敛 GAS 与移动状态，避免重入伤害或 Ability 结束回调重复提交死亡。
	bDead = true;
	if (USWAbilitySystemComponent* const SWAbilitySystemComponent = Cast<USWAbilitySystemComponent>(AbilitySystemComponent))
	{
		SWAbilitySystemComponent->SetDeadStateTagAuthority(true);
	}

	FGameplayTagContainer SurviveDeathTags;
	SurviveDeathTags.AddTag(SWGameplayTags::Ability_Behavior_SurviveDeath);
	AbilitySystemComponent->CancelAbilities(nullptr, &SurviveDeathTags);

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->DisableMovement();
	}

	GrantDeathRewardsAuthority(DeathContext);
	OnDeath.Broadcast(DeathContext);
	ApplyDeathStatePresentation();
	ForceNetUpdate();
	return true;
}

void ASWCharacter_Base::GrantDeathRewardsAuthority(const FSWDeathContext& DeathContext)
{
	if (!HasAuthority() || !CombatantDefinition || !DeathContext.InstigatorActor)
	{
		return;
	}

	USWAbilitySystemComponent* KillerAbilitySystemComponent = nullptr;
	if (const IAbilitySystemInterface* const AbilitySystemInterface = Cast<IAbilitySystemInterface>(DeathContext.InstigatorActor))
	{
		KillerAbilitySystemComponent = Cast<USWAbilitySystemComponent>(AbilitySystemInterface->GetAbilitySystemComponent());
	}

	if (!KillerAbilitySystemComponent || KillerAbilitySystemComponent == AbilitySystemComponent
		|| USWGameplayEffect::AreAbilitySystemComponentsOnSameTeam(KillerAbilitySystemComponent, AbilitySystemComponent))
	{
		return;
	}

	const int32 CombatLevel = ISWCombatInterface::Execute_GetCombatLevel(this);
	const float ExperienceRewardFloat = CombatantDefinition->XPRewardByLevel.GetValueAtLevel(FMath::Max(1, CombatLevel));
	if (FMath::IsFinite(ExperienceRewardFloat) && ExperienceRewardFloat > 0.f)
	{
		const int32 ExperienceReward = ExperienceRewardFloat >= static_cast<float>(MAX_int32)
			? MAX_int32
			: FMath::FloorToInt(ExperienceRewardFloat);
		KillerAbilitySystemComponent->ApplyExperienceRewardToSelfAuthority(ExperienceReward, this);
	}

	// 金币不属于 GAS Attribute；它由击杀者 PlayerState 持有并通过唯一金币写入入口复制给所属客户端。
	const float GoldRewardFloat = CombatantDefinition->GoldRewardByLevel.GetValueAtLevel(FMath::Max(1, CombatLevel));
	if (!FMath::IsFinite(GoldRewardFloat) || GoldRewardFloat <= 0.f)
	{
		return;
	}

	ASWPlayerState* const KillerPlayerState = Cast<ASWPlayerState>(KillerAbilitySystemComponent->GetOwnerActor());
	if (!KillerPlayerState)
	{
		return;
	}

	const int32 GoldReward = GoldRewardFloat >= static_cast<float>(MAX_int32)
		? MAX_int32
		: FMath::FloorToInt(GoldRewardFloat);
	KillerPlayerState->GrantGoldAuthority(GoldReward);
}

void ASWCharacter_Base::InitAbilityActorInfo()
{
	// 基类不做任何绑定；玩家与 AI 子类分别覆写。
}

void ASWCharacter_Base::OnRep_Dead()
{
	ApplyDeathStatePresentation();
}

void ASWCharacter_Base::ApplyDeathStatePresentation()
{
	// Dedicated Server 不加载或模拟死亡视觉；客户端（含晚加入者）通过 OnRep 收到相同事件。
	if (GetNetMode() != NM_DedicatedServer)
	{
		BP_OnDeathStateChanged(bDead);
	}
}

void ASWCharacter_Base::ApplyRespawnInvulnerabilityEffectAuthority()
{
	if (!HasAuthority() || !AbilitySystemComponent || !CombatantDefinition || !CombatantDefinition->RespawnInvulnerabilityEffect)
	{
		return;
	}

	FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(this);
	const FGameplayEffectSpecHandle EffectSpec = AbilitySystemComponent->MakeOutgoingSpec(
		CombatantDefinition->RespawnInvulnerabilityEffect, 1.f, EffectContext);
	if (EffectSpec.IsValid())
	{
		AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*EffectSpec.Data.Get());
	}
}

void ASWCharacter_Base::ApplyCombatantInitializationEffectsAuthority(const int32 EffectLevel, const bool bRestoreVitalResources)
{
	if (!HasAuthority() || !AbilitySystemComponent)
	{
		return;
	}

	if (!CombatantDefinition)
	{
		UE_LOG(LogTemp, Warning, TEXT("角色 %s 未配置 CombatantDefinition，跳过初始化 Gameplay Effect。"), *GetName());
		return;
	}

	const int32 SafeEffectLevel = FMath::Max(1, EffectLevel);
	const auto ApplyEffect = [this, SafeEffectLevel](const TSubclassOf<UGameplayEffect> EffectClass, const TCHAR* EffectLabel)
	{
		if (!EffectClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("角色 %s 的 CombatantDefinition 未配置%s。"), *GetName(), EffectLabel);
			return;
		}

		FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
		EffectContext.AddSourceObject(this);
		const FGameplayEffectSpecHandle EffectSpec = AbilitySystemComponent->MakeOutgoingSpec(EffectClass, SafeEffectLevel, EffectContext);
		if (!EffectSpec.IsValid())
		{
			UE_LOG(LogTemp, Error, TEXT("角色 %s 无法创建%s的 Gameplay Effect Spec。"), *GetName(), EffectLabel);
			return;
		}

		AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*EffectSpec.Data.Get());
	};

	ApplyEffect(CombatantDefinition->LevelAttributesEffect, TEXT("等级属性 GE"));

	// ASC 可能在 PlayerState 上跨重生存活；先移除同源常驻效果，确保不会累计出多份恢复周期。
	if (CombatantDefinition->ResourceRegenerationEffect)
	{
		AbilitySystemComponent->RemoveActiveGameplayEffectBySourceEffect(
			CombatantDefinition->ResourceRegenerationEffect, AbilitySystemComponent, -1);
		ApplyEffect(CombatantDefinition->ResourceRegenerationEffect, TEXT("资源自然恢复 GE"));
	}

	if (bRestoreVitalResources)
	{
		ApplyEffect(CombatantDefinition->VitalAttributesEffect, TEXT("满资源 GE"));
	}
}

void ASWCharacter_Base::RestoreVitalResourcesToMaximumAuthority()
{
	if (!HasAuthority() || !AttributeSet)
	{
		return;
	}

	// 当前值不由装备 GE 直接修改；重生应以装备、等级等聚合后的最终最大值为准恢复资源。
	AttributeSet->SetHealth(AttributeSet->GetMaxHealth());
	AttributeSet->SetMana(AttributeSet->GetMaxMana());
	AttributeSet->SetStamina(AttributeSet->GetMaxStamina());
}
