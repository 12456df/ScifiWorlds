// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "Player/SWPlayerState.h"

#include "AbilitySystem/SWAbilitySystemComponent.h"
#include "Algo/Count.h"
#include "AbilitySystem/SWAttributeSet.h"
#include "Character/SWCharacter_Base.h"
#include "Components/SceneComponent.h"
#include "AbilitySystem/Data/SWProgressionData.h"
#include "Economy/SWEconomyData.h"
#include "Equipment/SWEquipmentItemDefinition.h"
#include "Engine/AssetManager.h"
#include "Engine/World.h"
#include "GameState/SWGameState.h"
#include "GameplayEffect.h"
#include "GameplayTags/SWGameplayTags.h"
#include "Net/UnrealNetwork.h"
#include "Shop/SWShopZone.h"

ASWPlayerState::ASWPlayerState()
{
	// PlayerState replicates GAS state; raise the update rate above the default.
	SetNetUpdateFrequency(100.f);

	AbilitySystemComponent = CreateDefaultSubobject<USWAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	// Players use Mixed: GEs replicate to the owner, cues/tags to everyone.
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<USWAttributeSet>(TEXT("AttributeSet"));
	EquipmentSlots.SetNum(EquipmentSlotCount);
}

void ASWPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(ASWPlayerState, Level, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ASWPlayerState, Experience, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ASWPlayerState, AbilityPoints, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ASWPlayerState, Gold, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ASWPlayerState, EquipmentSlots, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ASWPlayerState, bCanTradeAtShop, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME(ASWPlayerState, TeamId);
}

UAbilitySystemComponent* ASWPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ASWPlayerState::SetTeamId(const ESWTeamId NewTeamId)
{
	check(HasAuthority());

	if (!ensureMsgf(IsValidTeamId(NewTeamId), TEXT("请求了无效的队伍归属。")))
	{
		return;
	}

	if (TeamId == NewTeamId)
	{
		// 即使 TeamId 未变化，也修复可能因重生或晚初始化缺失的 ASC 派生 Tag。
		AbilitySystemComponent->SetTeamIdTagAuthority(TeamId);
		return;
	}

	const ESWTeamId PreviousTeamId = TeamId;
	TeamId = NewTeamId;
	AbilitySystemComponent->SetTeamIdTagAuthority(TeamId);
	OnTeamIdChanged.Broadcast(PreviousTeamId, TeamId);
}

bool ASWPlayerState::IsValidTeamId(const ESWTeamId TeamIdToValidate) const
{
	return TeamIdToValidate == ESWTeamId::None
		|| TeamIdToValidate == ESWTeamId::TeamA
		|| TeamIdToValidate == ESWTeamId::TeamB;
}

void ASWPlayerState::AddExperienceAuthority(const int32 DeltaExperience)
{
	if (!HasAuthority() || DeltaExperience <= 0)
	{
		return;
	}

	const USWProgressionData* ProgressionData = GetProgressionData();
	if (!ProgressionData || !ProgressionData->HasValidLevelEntries())
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerState %s 的 ProgressionData 缺失或无效；拒绝本次经验结算。"), *GetName());
		return;
	}

	const int64 AccumulatedExperience = static_cast<int64>(Experience) + DeltaExperience;
	Experience = static_cast<int32>(FMath::Min<int64>(AccumulatedExperience, MAX_int32));

	const int32 PreviousLevel = Level;
	const int32 NewLevel = FMath::Max(PreviousLevel, FindLevelForExperience(Experience));
	int32 AbilityPointsToGrant = 0;
	for (int32 AwardedLevel = PreviousLevel + 1; AwardedLevel <= NewLevel; ++AwardedLevel)
	{
		const int64 AccumulatedAbilityPoints = static_cast<int64>(AbilityPointsToGrant) + ProgressionData->GetAbilityPointRewardForLevel(AwardedLevel);
		AbilityPointsToGrant = static_cast<int32>(FMath::Min<int64>(AccumulatedAbilityPoints, MAX_int32));
	}

	// 先写入完整最终状态，再广播委托，确保 HUD 与 Avatar 观察到的是同一份快照。
	Level = NewLevel;
	AbilityPoints = static_cast<int32>(FMath::Min<int64>(static_cast<int64>(AbilityPoints) + AbilityPointsToGrant, MAX_int32));
	OnExperienceChanged.Broadcast(Experience);

	if (Level != PreviousLevel)
	{
		OnLevelChanged.Broadcast(Level);
		ExecuteLevelUpGameplayCueAuthority();
	}

	if (AbilityPointsToGrant > 0)
	{
		OnAbilityPointsChanged.Broadcast(AbilityPoints);
	}
}

void ASWPlayerState::SetLevel(int32 NewLevel)
{
	if (!HasAuthority())
	{
		return;
	}

	const USWProgressionData* ProgressionData = GetProgressionData();
	const int32 MaximumLevel = ProgressionData ? ProgressionData->GetMaximumLevel() : MAX_int32;
	const int32 PreviousLevel = Level;
	Level = FMath::Clamp(NewLevel, 1, MaximumLevel);
	if (Level != PreviousLevel)
	{
		OnLevelChanged.Broadcast(Level);
		ExecuteLevelUpGameplayCueAuthority();
	}
}

void ASWPlayerState::ExecuteLevelUpGameplayCueAuthority()
{
	if (!HasAuthority() || !AbilitySystemComponent)
	{
		return;
	}

	AActor* const AvatarActor = AbilitySystemComponent->GetAvatarActor();
	if (!AvatarActor)
	{
		// Avatar 尚未绑定时不补播历史升级特效；等级复制仍是唯一真值。
		return;
	}

	FGameplayCueParameters CueParameters;
	CueParameters.Location = AvatarActor->GetActorLocation();
	CueParameters.Instigator = AvatarActor;
	CueParameters.EffectCauser = AvatarActor;
	CueParameters.SourceObject = this;
	CueParameters.TargetAttachComponent = AvatarActor->GetRootComponent();
	CueParameters.bReplicateLocationWhenUsingMinimalRepProxy = true;

	AbilitySystemComponent->ExecuteGameplayCue(SWGameplayTags::GameplayCue_Player_LevelUp, CueParameters);
}

const USWProgressionData* ASWPlayerState::GetProgressionData() const
{
	const UWorld* World = GetWorld();
	const ASWGameState* GameState = World ? World->GetGameState<ASWGameState>() : nullptr;
	return GameState ? GameState->GetProgressionData() : nullptr;
}

int32 ASWPlayerState::FindLevelForExperience(const int32 TotalExperience) const
{
	const USWProgressionData* ProgressionData = GetProgressionData();
	return ProgressionData && ProgressionData->HasValidLevelEntries()
		? ProgressionData->FindLevelForExperience(TotalExperience)
		: 1;
}

void ASWPlayerState::GrantAbilityPoints(int32 DeltaPoints)
{
	if (!HasAuthority() || DeltaPoints <= 0)
	{
		return;
	}

	AbilityPoints = static_cast<int32>(FMath::Min<int64>(static_cast<int64>(AbilityPoints) + DeltaPoints, MAX_int32));
	OnAbilityPointsChanged.Broadcast(AbilityPoints);
}

bool ASWPlayerState::SpendAbilityPoint()
{
	if (!HasAuthority() || AbilityPoints <= 0)
	{
		return false;
	}

	AbilityPoints -= 1;
	OnAbilityPointsChanged.Broadcast(AbilityPoints);
	return true;
}

void ASWPlayerState::GrantGoldAuthority(const int32 DeltaGold)
{
	if (!HasAuthority() || DeltaGold <= 0)
	{
		return;
	}

	ApplyGoldDeltaAuthority(DeltaGold);
}

bool ASWPlayerState::TrySpendGoldAuthority(const int32 GoldCost)
{
	if (!HasAuthority() || GoldCost < 0 || Gold < GoldCost)
	{
		return false;
	}

	if (GoldCost == 0)
	{
		return true;
	}

	return ApplyGoldDeltaAuthority(-GoldCost);
}

int32 ASWPlayerState::GetOwnedEquipmentCount(const FPrimaryAssetId& ItemDefinitionId) const
{
	if (!ItemDefinitionId.IsValid())
	{
		return 0;
	}

	return Algo::CountIf(EquipmentSlots, [&ItemDefinitionId](const FSWEquipmentSlot& Slot)
	{
		return Slot.ItemDefinitionId == ItemDefinitionId;
	});
}

bool ASWPlayerState::TryPurchaseEquipmentAuthority(const USWEquipmentItemDefinition* const ItemDefinition)
{
	if (!HasAuthority() || !bCanTradeAtShop || !ItemDefinition)
	{
		return false;
	}

	FString ValidationFailure;
	if (!ValidateEquipmentDefinition(ItemDefinition, ValidationFailure))
	{
		UE_LOG(LogTemp, Warning, TEXT("拒绝购买无效装备 %s：%s"), *GetNameSafe(ItemDefinition), *ValidationFailure);
		return false;
	}

	const FPrimaryAssetId ItemId = ItemDefinition->GetPrimaryAssetId();
	if (GetOwnedEquipmentCount(ItemId) >= ItemDefinition->MaxOwnedCount || Gold < ItemDefinition->PurchasePrice)
	{
		return false;
	}

	const int32 EmptySlotIndex = EquipmentSlots.IndexOfByPredicate([](const FSWEquipmentSlot& Slot)
	{
		return Slot.IsEmpty();
	});
	if (EmptySlotIndex == INDEX_NONE)
	{
		return false;
	}

	// 先让 M08 原子应用全部 GE；只有成功才提交金币和持久槽位状态。
	FSWEquipmentSlot& EmptySlot = EquipmentSlots[EmptySlotIndex];
	EmptySlot.ItemDefinitionId = ItemId;
	EmptySlot.PurchasePricePaid = ItemDefinition->PurchasePrice;
	if (!ApplyEquipmentEffectsForSlotAuthority(EmptySlotIndex, ItemDefinition))
	{
		EmptySlot.Clear();
		return false;
	}

	if (!TrySpendGoldAuthority(ItemDefinition->PurchasePrice))
	{
		RemoveEquipmentEffectsForSlotAuthority(EmptySlotIndex);
		EmptySlot.Clear();
		return false;
	}

	OnEquipmentSlotsChanged.Broadcast();
	ForceNetUpdate();
	return true;
}

bool ASWPlayerState::TrySellEquipmentSlotAuthority(const int32 SlotIndex)
{
	if (!HasAuthority() || !bCanTradeAtShop || !EquipmentSlots.IsValidIndex(SlotIndex))
	{
		return false;
	}

	FSWEquipmentSlot& Slot = EquipmentSlots[SlotIndex];
	if (Slot.IsEmpty())
	{
		return false;
	}

	const USWEconomyData* const EconomyData = GetEconomyData();
	const float RefundRate = EconomyData ? EconomyData->SellRefundRate : 0.f;
	const int32 Refund = FMath::Max(0, FMath::FloorToInt(static_cast<float>(Slot.PurchasePricePaid) * RefundRate));

	// 句柄意外缺失时仍允许出售，避免派生状态错误永久锁死物品栏。
	RemoveEquipmentEffectsForSlotAuthority(SlotIndex);
	Slot.Clear();
	if (Refund > 0)
	{
		GrantGoldAuthority(Refund);
	}

	OnEquipmentSlotsChanged.Broadcast();
	ForceNetUpdate();
	return true;
}

void ASWPlayerState::AddShopZoneAuthority(ASWShopZone* const ShopZone)
{
	if (!HasAuthority() || !ShopZone)
	{
		return;
	}

	ActiveShopZones.Add(ShopZone);
	RefreshShopTradeAccessAuthority();
}

void ASWPlayerState::RemoveShopZoneAuthority(ASWShopZone* const ShopZone)
{
	if (!HasAuthority() || !ShopZone)
	{
		return;
	}

	ActiveShopZones.Remove(ShopZone);
	RefreshShopTradeAccessAuthority();
}

void ASWPlayerState::ClearShopTradeAccessAuthority()
{
	if (!HasAuthority())
	{
		return;
	}

	ActiveShopZones.Reset();
	RefreshShopTradeAccessAuthority();
}

void ASWPlayerState::InitializeEconomyAuthority()
{
	if (!HasAuthority() || bEconomyInitialized)
	{
		return;
	}

	// 即使配置缺失也锁定本局的首次初始化，避免配置错误导致重复发放起始金币。
	bEconomyInitialized = true;
	const USWEconomyData* const EconomyData = GetEconomyData();
	if (!EconomyData)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerState %s 缺少 EconomyData；本局初始金币保持为 0。"), *GetName());
		return;
	}

	GrantGoldAuthority(EconomyData->StartingGold);
}

void ASWPlayerState::GrantPassiveGoldIncomeAuthority(const float GoldPerSecond)
{
	if (!HasAuthority() || GoldPerSecond <= 0.f || Gold >= GetMaximumGold())
	{
		return;
	}

	PassiveGoldFraction += static_cast<double>(GoldPerSecond);
	const int64 WholeGold = FMath::FloorToInt64(PassiveGoldFraction);
	if (WholeGold <= 0)
	{
		return;
	}

	PassiveGoldFraction -= static_cast<double>(WholeGold);
	GrantGoldAuthority(static_cast<int32>(FMath::Min<int64>(WholeGold, MAX_int32)));
}

int32 ASWPlayerState::GetMaximumGold() const
{
	const USWEconomyData* const EconomyData = GetEconomyData();
	return EconomyData ? FMath::Max(1, EconomyData->MaxGold) : MAX_int32;
}

const USWEconomyData* ASWPlayerState::GetEconomyData() const
{
	const UWorld* const World = GetWorld();
	const ASWGameState* const GameState = World ? World->GetGameState<ASWGameState>() : nullptr;
	return GameState ? GameState->GetEconomyData() : nullptr;
}

bool ASWPlayerState::ApplyGoldDeltaAuthority(const int32 DeltaGold)
{
	if (!HasAuthority() || DeltaGold == 0)
	{
		return false;
	}

	const int64 RequestedGold = static_cast<int64>(Gold) + static_cast<int64>(DeltaGold);
	if (RequestedGold < 0)
	{
		return false;
	}

	const int32 NewGold = static_cast<int32>(FMath::Clamp<int64>(RequestedGold, 0, GetMaximumGold()));
	if (NewGold == Gold)
	{
		return false;
	}

	Gold = NewGold;
	BroadcastGoldChanged();
	return true;
}

void ASWPlayerState::BroadcastGoldChanged()
{
	OnGoldChanged.Broadcast(Gold);
}

void ASWPlayerState::RefreshShopTradeAccessAuthority()
{
	check(HasAuthority());
	for (auto It = ActiveShopZones.CreateIterator(); It; ++It)
	{
		if (!It->IsValid())
		{
			It.RemoveCurrent();
		}
	}

	const ASWCharacter_Base* const AvatarCharacter = Cast<ASWCharacter_Base>(AbilitySystemComponent ? AbilitySystemComponent->GetAvatarActor() : nullptr);
	const bool bNewCanTrade = ActiveShopZones.Num() > 0 && AvatarCharacter && !AvatarCharacter->IsDeadCommitted();
	if (bCanTradeAtShop != bNewCanTrade)
	{
		bCanTradeAtShop = bNewCanTrade;
		OnShopTradeAccessChanged.Broadcast(bCanTradeAtShop);
		ForceNetUpdate();
	}
}

void ASWPlayerState::EnsureEquipmentEffectsAppliedAuthority()
{
	if (!HasAuthority() || !AbilitySystemComponent || !AbilitySystemComponent->GetAvatarActor())
	{
		return;
	}

	InitializeStartingEquipmentAuthority();

	for (int32 SlotIndex = 0; SlotIndex < EquipmentSlots.Num(); ++SlotIndex)
	{
		const FSWEquipmentSlot& Slot = EquipmentSlots[SlotIndex];
		if (Slot.IsEmpty())
		{
			continue;
		}

		if (AppliedEquipmentEffectHandles.Contains(SlotIndex))
		{
			continue;
		}

		const USWEquipmentItemDefinition* const ItemDefinition = ResolveEquipmentDefinitionAuthority(Slot.ItemDefinitionId);
		if (!ApplyEquipmentEffectsForSlotAuthority(SlotIndex, ItemDefinition))
		{
			UE_LOG(LogTemp, Error, TEXT("PlayerState %s 无法为装备槽位 %d 应用装备效果。"), *GetName(), SlotIndex);
		}
	}
}

bool ASWPlayerState::ApplyEquipmentEffectsForSlotAuthority(const int32 SlotIndex, const USWEquipmentItemDefinition* const ItemDefinition)
{
	if (!HasAuthority() || !AbilitySystemComponent || !AbilitySystemComponent->GetAvatarActor()
		|| !EquipmentSlots.IsValidIndex(SlotIndex) || !ItemDefinition)
	{
		return false;
	}

	const FSWEquipmentSlot& Slot = EquipmentSlots[SlotIndex];
	if (Slot.IsEmpty() || Slot.ItemDefinitionId != ItemDefinition->GetPrimaryAssetId())
	{
		return false;
	}

	if (AppliedEquipmentEffectHandles.Contains(SlotIndex))
	{
		return true;
	}

	FString ValidationFailure;
	if (!ValidateEquipmentDefinition(ItemDefinition, ValidationFailure))
	{
		UE_LOG(LogTemp, Error, TEXT("装备 %s 不可应用：%s"), *ItemDefinition->GetName(), *ValidationFailure);
		return false;
	}

	TArray<FActiveGameplayEffectHandle> NewHandles;
	for (const TSubclassOf<UGameplayEffect> EffectClass : ItemDefinition->EquippedEffectClasses)
	{
		FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
		EffectContext.AddSourceObject(const_cast<USWEquipmentItemDefinition*>(ItemDefinition));

		const FGameplayEffectSpecHandle EffectSpec = AbilitySystemComponent->MakeOutgoingSpec(EffectClass, 1.f, EffectContext);
		if (!EffectSpec.IsValid())
		{
			for (const FActiveGameplayEffectHandle Handle : NewHandles)
			{
				AbilitySystemComponent->RemoveActiveGameplayEffect(Handle);
			}
			return false;
		}

		const FActiveGameplayEffectHandle ActiveHandle = AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*EffectSpec.Data.Get());
		if (!ActiveHandle.IsValid())
		{
			for (const FActiveGameplayEffectHandle Handle : NewHandles)
			{
				AbilitySystemComponent->RemoveActiveGameplayEffect(Handle);
			}
			return false;
		}

		NewHandles.Add(ActiveHandle);
	}

	AppliedEquipmentEffectHandles.Add(SlotIndex, MoveTemp(NewHandles));
	return true;
}

bool ASWPlayerState::RemoveEquipmentEffectsForSlotAuthority(const int32 SlotIndex)
{
	if (!HasAuthority() || !AbilitySystemComponent)
	{
		return false;
	}

	TArray<FActiveGameplayEffectHandle> Handles;
	if (!AppliedEquipmentEffectHandles.RemoveAndCopyValue(SlotIndex, Handles))//？？？
	{
		return false;
	}

	for (const FActiveGameplayEffectHandle Handle : Handles)
	{
		if (Handle.IsValid())
		{
			AbilitySystemComponent->RemoveActiveGameplayEffect(Handle);
		}
	}

	return true;
}

bool ASWPlayerState::RebuildEquipmentEffectsAuthority()
{
	if (!HasAuthority() || !AbilitySystemComponent || !AbilitySystemComponent->GetAvatarActor())
	{
		return false;
	}

	for (int32 SlotIndex = 0; SlotIndex < EquipmentSlots.Num(); ++SlotIndex)
	{
		RemoveEquipmentEffectsForSlotAuthority(SlotIndex);
	}

	bool bAllEffectsApplied = true;
	for (int32 SlotIndex = 0; SlotIndex < EquipmentSlots.Num(); ++SlotIndex)
	{
		const FSWEquipmentSlot& Slot = EquipmentSlots[SlotIndex];
		if (Slot.IsEmpty())
		{
			continue;
		}

		const USWEquipmentItemDefinition* const ItemDefinition = ResolveEquipmentDefinitionAuthority(Slot.ItemDefinitionId);
		bAllEffectsApplied &= ApplyEquipmentEffectsForSlotAuthority(SlotIndex, ItemDefinition);
	}

	return bAllEffectsApplied;
}

bool ASWPlayerState::ValidateEquipmentDefinition(const USWEquipmentItemDefinition* const ItemDefinition, FString& OutFailureReason) const
{
	OutFailureReason.Reset();
	if (!ItemDefinition)
	{
		OutFailureReason = TEXT("装备定义为空。");
		return false;
	}

	if (!ItemDefinition->GetPrimaryAssetId().IsValid())
	{
		OutFailureReason = TEXT("装备定义缺少有效的 Primary Asset Id。");
		return false;
	}

	if (ItemDefinition->EquippedEffectClasses.IsEmpty())
	{
		OutFailureReason = TEXT("装备定义未配置任何常驻 Gameplay Effect。");
		return false;
	}

	for (const TSubclassOf<UGameplayEffect> EffectClass : ItemDefinition->EquippedEffectClasses)
	{
		const UGameplayEffect* const Effect = EffectClass ? EffectClass->GetDefaultObject<UGameplayEffect>() : nullptr;
		if (!Effect)
		{
			OutFailureReason = TEXT("装备定义包含空 Gameplay Effect 类。");
			return false;
		}

		if (Effect->DurationPolicy != EGameplayEffectDurationType::Infinite)
		{
			OutFailureReason = FString::Printf(TEXT("Gameplay Effect %s 必须使用 Infinite Duration Policy。"), *Effect->GetName());
			return false;
		}
	}

	return true;
}

void ASWPlayerState::InitializeStartingEquipmentAuthority()
{
	if (!HasAuthority() || bStartingEquipmentInitialized)
	{
		return;
	}

	EquipmentSlots.SetNum(EquipmentSlotCount);
	TMap<FPrimaryAssetId, int32> OwnedItemCounts;
	for (int32 SlotIndex = 0; SlotIndex < StartingEquipmentDefinitions.Num() && SlotIndex < EquipmentSlotCount; ++SlotIndex)
	{
		const USWEquipmentItemDefinition* const ItemDefinition = StartingEquipmentDefinitions[SlotIndex];
		FString ValidationFailure;
		if (!ValidateEquipmentDefinition(ItemDefinition, ValidationFailure))
		{
			UE_LOG(LogTemp, Error, TEXT("PlayerState %s 的初始装备槽位 %d 无效：%s"), *GetName(), SlotIndex, *ValidationFailure);
			continue;
		}

		const FPrimaryAssetId ItemId = ItemDefinition->GetPrimaryAssetId();
		const int32 NewOwnedCount = OwnedItemCounts.FindRef(ItemId) + 1;
		if (NewOwnedCount > ItemDefinition->MaxOwnedCount)
		{
			UE_LOG(LogTemp, Error, TEXT("PlayerState %s 的初始装备 %s 超过 MaxOwnedCount。"), *GetName(), *ItemDefinition->GetName());
			continue;
		}

		OwnedItemCounts.Add(ItemId, NewOwnedCount);
		EquipmentSlots[SlotIndex].ItemDefinitionId = ItemId;
	}

	bStartingEquipmentInitialized = true;
	OnEquipmentSlotsChanged.Broadcast();
	ForceNetUpdate();
}

const USWEquipmentItemDefinition* ASWPlayerState::ResolveEquipmentDefinitionAuthority(const FPrimaryAssetId& ItemDefinitionId) const
{
	if (!ItemDefinitionId.IsValid())
	{
		return nullptr;
	}

	UAssetManager& AssetManager = UAssetManager::Get();
	if (USWEquipmentItemDefinition* const LoadedDefinition = Cast<USWEquipmentItemDefinition>(AssetManager.GetPrimaryAssetObject(ItemDefinitionId)))
	{
		return LoadedDefinition;
	}

	const FSoftObjectPath AssetPath = AssetManager.GetPrimaryAssetPath(ItemDefinitionId);
	return Cast<USWEquipmentItemDefinition>(AssetPath.TryLoad());
}

void ASWPlayerState::OnRep_TeamId(const ESWTeamId PreviousTeamId)
{
	OnTeamIdChanged.Broadcast(PreviousTeamId, TeamId);
}

void ASWPlayerState::OnRep_Level(int32 OldLevel)
{
	OnLevelChanged.Broadcast(Level);
}

void ASWPlayerState::OnRep_Experience(int32 OldExperience)
{
	OnExperienceChanged.Broadcast(Experience);
}

void ASWPlayerState::OnRep_AbilityPoints(int32 OldAbilityPoints)
{
	OnAbilityPointsChanged.Broadcast(AbilityPoints);
}

void ASWPlayerState::OnRep_Gold(int32 OldGold)
{
	BroadcastGoldChanged();
}

void ASWPlayerState::OnRep_CanTradeAtShop()
{
	OnShopTradeAccessChanged.Broadcast(bCanTradeAtShop);
}

void ASWPlayerState::OnRep_EquipmentSlots()
{
	OnEquipmentSlotsChanged.Broadcast();
}
