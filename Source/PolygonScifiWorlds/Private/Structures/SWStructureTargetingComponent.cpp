// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "Structures/SWStructureTargetingComponent.h"

#include "Components/SphereComponent.h"
#include "Interaction/SWCombatInterface.h"
#include "Interaction/SWTargetableInterface.h"
#include "Interaction/SWTeamInterface.h"
#include "Combat/Targeting/SWCombatTargetRegistrySubsystem.h"
#include "Structures/SWDefenseStructure.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SWStructureTargetingComponent)

USWStructureTargetingComponent::USWStructureTargetingComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicated(false);
}

void USWStructureTargetingComponent::InitializeAuthority()
{
	if (bInitializedAuthority || !GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	ASWDefenseStructure* const Structure = Cast<ASWDefenseStructure>(GetOwner());
	USphereComponent* const CombatRange = Structure ? Structure->GetCombatRange() : nullptr;
	if (!Structure || !CombatRange)
	{
		UE_LOG(LogTemp, Error, TEXT("StructureTargetingComponent 的 Owner 或 CombatRange 无效。"));
		return;
	}

	StructureOwner = Structure;
	CombatRange->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::HandleCombatRangeBeginOverlap);
	CombatRange->OnComponentEndOverlap.AddDynamic(this, &ThisClass::HandleCombatRangeEndOverlap);
	bInitializedAuthority = true;

	TArray<AActor*> InitialOverlaps;
	CombatRange->GetOverlappingActors(InitialOverlaps);
	for (AActor* const CandidateActor : InitialOverlaps)
	{
		if (CandidateActor)
		{
			AddCandidateAuthority(*CandidateActor);
		}
	}

	RefreshTargetAuthority(ETargetInvalidReason::General);
}

AActor* USWStructureTargetingComponent::GetCurrentTargetAuthority()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return nullptr;
	}

	ETargetInvalidReason InvalidReason = ETargetInvalidReason::General;
	const FCandidateRecord* CurrentRecord = nullptr;
	if (AActor* const ExistingTarget = CurrentTarget.Get())
	{
		CurrentRecord = Candidates.Find(TObjectKey<const AActor>(ExistingTarget));
	}
	if (!CurrentRecord || !IsCandidateLegalAuthority(*CurrentRecord, InvalidReason))
	{
		RefreshTargetAuthority(InvalidReason);
	}

	return CurrentTarget.Get();
}

void USWStructureTargetingComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bInitializedAuthority)
	{
		if (ASWDefenseStructure* const Structure = StructureOwner.Get())
		{
			if (USphereComponent* const CombatRange = Structure->GetCombatRange())
			{
				CombatRange->OnComponentBeginOverlap.RemoveDynamic(this, &ThisClass::HandleCombatRangeBeginOverlap);
				CombatRange->OnComponentEndOverlap.RemoveDynamic(this, &ThisClass::HandleCombatRangeEndOverlap);
			}
		}
		ClearCandidatesAuthority();
		bInitializedAuthority = false;
	}

	Super::EndPlay(EndPlayReason);
}

void USWStructureTargetingComponent::HandleCombatRangeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, const int32 OtherBodyIndex, const bool bFromSweep, const FHitResult& SweepResult)
{
	static_cast<void>(OverlappedComponent);
	static_cast<void>(OtherComponent);
	static_cast<void>(OtherBodyIndex);
	static_cast<void>(bFromSweep);
	static_cast<void>(SweepResult);

	if (OtherActor)
	{
		AddCandidateAuthority(*OtherActor);
	}
}

void USWStructureTargetingComponent::HandleCombatRangeEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, const int32 OtherBodyIndex)
{
	static_cast<void>(OverlappedComponent);
	static_cast<void>(OtherComponent);
	static_cast<void>(OtherBodyIndex);

	if (OtherActor)
	{
		// 同一 Actor 可能有多个 PrimitiveComponent；仅在最后一个重叠结束后才移除候选。
		const ASWDefenseStructure* const Structure = StructureOwner.Get();
		const USphereComponent* const CombatRange = Structure ? Structure->GetCombatRange() : nullptr;
		if (!CombatRange || !CombatRange->IsOverlappingActor(OtherActor))
		{
			RemoveCandidateAuthority(*OtherActor, ETargetInvalidReason::General);
		}
	}
}

void USWStructureTargetingComponent::HandleCandidateDeath(const FSWDeathContext& DeathContext, const TWeakObjectPtr<AActor> DeadActor)
{
	static_cast<void>(DeathContext);
	if (AActor* const Actor = DeadActor.Get())
	{
		RemoveCandidateAuthority(*Actor, ETargetInvalidReason::Dead);
	}
}

void USWStructureTargetingComponent::AddCandidateAuthority(AActor& CandidateActor)
{
	ASWDefenseStructure* const Structure = StructureOwner.Get();
	if (!bInitializedAuthority || !Structure || &CandidateActor == Structure || Candidates.Contains(TObjectKey<const AActor>(&CandidateActor)))
	{
		return;
	}

	if (!CandidateActor.Implements<USWCombatInterface>() || !CandidateActor.Implements<USWTeamInterface>())
	{
		return;
	}

	USWCombatTargetRegistrySubsystem* const TargetRegistry = GetWorld() ? GetWorld()->GetSubsystem<USWCombatTargetRegistrySubsystem>() : nullptr;
	FSWMinionRegisteredTargetInfo TargetInfo;
	if (!TargetRegistry || !TargetRegistry->TryGetRegisteredTargetInfo(CandidateActor, TargetInfo)
		|| (TargetInfo.Category != ESWMinionTargetCategory::Minion && TargetInfo.Category != ESWMinionTargetCategory::Player))
	{
		return;
	}

	FCandidateRecord Candidate;
	Candidate.Actor = &CandidateActor;
	Candidate.TargetId = TargetInfo.TargetId;
	Candidate.Category = TargetInfo.Category;
	Candidate.EntrySequence = NextEntrySequence;

	ETargetInvalidReason InvalidReason = ETargetInvalidReason::General;
	if (!IsCandidateLegalAuthority(Candidate, InvalidReason))
	{
		return;
	}

	AdvanceEntrySequence();

	ISWCombatInterface* const Combatant = Cast<ISWCombatInterface>(&CandidateActor);
	check(Combatant);
	Candidate.DeathDelegateHandle = Combatant->GetOnDeathDelegate().AddUObject(this, &ThisClass::HandleCandidateDeath, TWeakObjectPtr<AActor>(&CandidateActor));
	Candidates.Add(TObjectKey<const AActor>(&CandidateActor), MoveTemp(Candidate));
	RefreshTargetAuthority(ETargetInvalidReason::General);
}

void USWStructureTargetingComponent::RemoveCandidateAuthority(const AActor& CandidateActor, const ETargetInvalidReason InvalidReason)
{
	FCandidateRecord* const Candidate = Candidates.Find(TObjectKey<const AActor>(&CandidateActor));
	if (!Candidate)
	{
		return;
	}

	if (ISWCombatInterface* const Combatant = Cast<ISWCombatInterface>(Candidate->Actor.Get()))
	{
		Combatant->GetOnDeathDelegate().Remove(Candidate->DeathDelegateHandle);
	}

	Candidates.Remove(TObjectKey<const AActor>(&CandidateActor));
	if (CurrentTarget.Get() == &CandidateActor)
	{
		RefreshTargetAuthority(InvalidReason);
	}
}

void USWStructureTargetingComponent::RefreshTargetAuthority(const ETargetInvalidReason InvalidReason)
{
	if (!bInitializedAuthority)
	{
		return;
	}

	ETargetInvalidReason CurrentInvalidReason = InvalidReason;
	const FCandidateRecord* const CurrentRecord = CurrentTarget.IsValid() ? Candidates.Find(TObjectKey<const AActor>(CurrentTarget.Get())) : nullptr;
	if (CurrentRecord && IsCandidateLegalAuthority(*CurrentRecord, CurrentInvalidReason))
	{
		return;
	}

	const bool bPriorTargetDied = CurrentInvalidReason == ETargetInvalidReason::Dead;
	const FCandidateRecord* BestCandidate = nullptr;
	for (const TPair<TObjectKey<const AActor>, FCandidateRecord>& Pair : Candidates)
	{
		ETargetInvalidReason CandidateInvalidReason = ETargetInvalidReason::General;
		const FCandidateRecord& Candidate = Pair.Value;
		if (!IsCandidateLegalAuthority(Candidate, CandidateInvalidReason))
		{
			continue;
		}

		const int32 CandidatePriority = bPriorTargetDied && Candidate.Category == ESWMinionTargetCategory::Minion ? 0 : 1;
		const int32 BestPriority = BestCandidate && bPriorTargetDied && BestCandidate->Category == ESWMinionTargetCategory::Minion ? 0 : 1;
		if (!BestCandidate || CandidatePriority < BestPriority
			|| (CandidatePriority == BestPriority && (Candidate.EntrySequence < BestCandidate->EntrySequence
				|| (Candidate.EntrySequence == BestCandidate->EntrySequence && Candidate.TargetId < BestCandidate->TargetId))))
		{
			BestCandidate = &Candidate;
		}
	}

	BroadcastTargetIfChanged(BestCandidate ? BestCandidate->Actor : nullptr);
}

bool USWStructureTargetingComponent::IsCandidateLegalAuthority(const FCandidateRecord& Candidate, ETargetInvalidReason& OutInvalidReason) const
{
	OutInvalidReason = ETargetInvalidReason::General;
	const ASWDefenseStructure* const Structure = StructureOwner.Get();
	AActor* const CandidateActor = Candidate.Actor.Get();
	if (!Structure || !CandidateActor || CandidateActor == Structure)
	{
		return false;
	}

	const USphereComponent* const CombatRange = Structure->GetCombatRange();
	if (!CombatRange || !CombatRange->IsOverlappingActor(CandidateActor))
	{
		return false;
	}

	if (!CandidateActor->Implements<USWCombatInterface>() || !CandidateActor->Implements<USWTeamInterface>())
	{
		return false;
	}

	if (ISWCombatInterface::Execute_IsDead(CandidateActor))
	{
		OutInvalidReason = ETargetInvalidReason::Dead;
		return false;
	}

	const ISWTeamInterface* const CandidateTeamOwner = Cast<ISWTeamInterface>(CandidateActor);
	const ESWTeamId CandidateTeam = CandidateTeamOwner ? CandidateTeamOwner->GetTeamId() : ESWTeamId::None;
	if (Structure->GetTeamId() == ESWTeamId::None || CandidateTeam == ESWTeamId::None || CandidateTeam == Structure->GetTeamId())
	{
		return false;
	}

	if (const ISWTargetableInterface* const Targetable = Cast<ISWTargetableInterface>(CandidateActor);
		Targetable && !Targetable->IsTargetableBy(Structure))
	{
		return false;
	}

	const USWCombatTargetRegistrySubsystem* const TargetRegistry = GetWorld() ? GetWorld()->GetSubsystem<USWCombatTargetRegistrySubsystem>() : nullptr;
	FSWMinionRegisteredTargetInfo TargetInfo;
	return TargetRegistry && TargetRegistry->TryGetRegisteredTargetInfo(*CandidateActor, TargetInfo)
		&& TargetInfo.TargetId == Candidate.TargetId && TargetInfo.Category == Candidate.Category
		&& (Candidate.Category == ESWMinionTargetCategory::Minion || Candidate.Category == ESWMinionTargetCategory::Player);
}

void USWStructureTargetingComponent::ClearCandidatesAuthority()
{
	for (const TPair<TObjectKey<const AActor>, FCandidateRecord>& Pair : Candidates)
	{
		if (ISWCombatInterface* const Combatant = Cast<ISWCombatInterface>(Pair.Value.Actor.Get()))
		{
			Combatant->GetOnDeathDelegate().Remove(Pair.Value.DeathDelegateHandle);
		}
	}

	Candidates.Empty();
	BroadcastTargetIfChanged(nullptr);
}

void USWStructureTargetingComponent::BroadcastTargetIfChanged(const TWeakObjectPtr<AActor> NewTarget)
{
	if (CurrentTarget == NewTarget)
	{
		return;
	}

	CurrentTarget = NewTarget;
	OnTargetChanged.Broadcast(CurrentTarget);
}

void USWStructureTargetingComponent::AdvanceEntrySequence()
{
	++NextEntrySequence;
	if (NextEntrySequence == 0)
	{
		NextEntrySequence = 1;
	}
}
