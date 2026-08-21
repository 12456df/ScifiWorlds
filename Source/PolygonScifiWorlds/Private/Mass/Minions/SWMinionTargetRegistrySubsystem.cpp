// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "Mass/Minions/SWMinionTargetRegistrySubsystem.h"

#include "Character/SWCharacter_Minion.h"
#include "Character/SWCharacter_Player.h"
#include "Interaction/SWCombatInterface.h"
#include "Interaction/SWTeamInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SWMinionTargetRegistrySubsystem)

FSWMinionTargetRegistrationHandle USWMinionTargetRegistrySubsystem::RegisterTarget(AActor& TargetActor)
{
	FSWMinionTargetRegistrationHandle Result;
	if (!TargetActor.HasAuthority()
		|| !TargetActor.Implements<USWCombatInterface>()
		|| !TargetActor.Implements<USWTeamInterface>())
	{
		return Result;
	}

	const TObjectKey<const AActor> ActorKey(&TargetActor);
	if (const uint32* const ExistingId = ActorToTargetId.Find(ActorKey))
	{
		Result.TargetId = *ExistingId;
		return Result;
	}

	// 0 是无效句柄；溢出后跳过 0，极端长局也保持该约定。
	uint32 NewTargetId = NextTargetId++;
	if (NewTargetId == 0)
	{
		NewTargetId = NextTargetId++;
	}

	FTargetEntry& Entry = EntriesByTargetId.Add(NewTargetId);
	Entry.Actor = &TargetActor;
	Entry.TargetId = NewTargetId;
	Entry.Category = ClassifyTarget(TargetActor);
	ActorToTargetId.Add(ActorKey, NewTargetId);
	Result.TargetId = NewTargetId;
	return Result;
}

void USWMinionTargetRegistrySubsystem::UnregisterTarget(const AActor& TargetActor)
{
	const TObjectKey<const AActor> ActorKey(&TargetActor);
	const uint32* const TargetId = ActorToTargetId.Find(ActorKey);
	if (!TargetId)
	{
		return;
	}

	EntriesByTargetId.Remove(*TargetId);
	ActorToTargetId.Remove(ActorKey);
}

FSWMinionTargetResult USWMinionTargetRegistrySubsystem::FindBestTarget(const FSWMinionTargetQuery& Query)
{
	FSWMinionTargetResult Result;
	if (!Query.SourceActor || Query.SourceTeam == ESWTeamId::None || Query.AcquisitionRange <= 0.f)
	{
		return Result;
	}

	PruneInvalidTargets();

	// 保留仍合法的当前目标，避免多个同距单位导致每次感知都换目标。
	if (Query.CurrentTargetId != 0)
	{
		if (const FTargetEntry* const CurrentEntry = EntriesByTargetId.Find(Query.CurrentTargetId))
		{
			float CurrentDistanceSquared = 0.f;
			if (CurrentEntry->Actor == Query.CurrentTarget && IsLegalTarget(*CurrentEntry, Query, CurrentDistanceSquared))
			{
				Result.TargetActor = CurrentEntry->Actor;
				Result.TargetId = CurrentEntry->TargetId;
				Result.Category = CurrentEntry->Category;
				return Result;
			}
		}
	}

	int32 BestPriority = MAX_int32;
	float BestDistanceSquared = TNumericLimits<float>::Max();
	uint32 BestTargetId = MAX_uint32;
	for (const TPair<uint32, FTargetEntry>& Pair : EntriesByTargetId)
	{
		const FTargetEntry& Entry = Pair.Value;
		float DistanceSquared = 0.f;
		if (!IsLegalTarget(Entry, Query, DistanceSquared))
		{
			continue;
		}

		const int32 Priority = Entry.Category == ESWMinionTargetCategory::Minion ? 0
			: Entry.Category == ESWMinionTargetCategory::Player ? 1
			: Entry.Category == ESWMinionTargetCategory::Structure ? 2 : MAX_int32;
		if (Priority == MAX_int32)
		{
			continue;
		}

		const bool bIsBetter = Priority < BestPriority
			|| (Priority == BestPriority && (DistanceSquared < BestDistanceSquared
				|| (FMath::IsNearlyEqual(DistanceSquared, BestDistanceSquared) && Entry.TargetId < BestTargetId)));
		if (bIsBetter)
		{
			BestPriority = Priority;
			BestDistanceSquared = DistanceSquared;
			BestTargetId = Entry.TargetId;
			Result.TargetActor = Entry.Actor;
			Result.TargetId = Entry.TargetId;
			Result.Category = Entry.Category;
		}
	}

	return Result;
}

int32 USWMinionTargetRegistrySubsystem::GetRegisteredTargetCount()
{
	PruneInvalidTargets();
	return EntriesByTargetId.Num();
}

void USWMinionTargetRegistrySubsystem::PruneInvalidTargets()
{
	for (auto It = EntriesByTargetId.CreateIterator(); It; ++It)
	{
		if (It.Value().Actor.IsValid())
		{
			continue;
		}

		const uint32 RemovedTargetId = It.Key();
		for (auto ActorIt = ActorToTargetId.CreateIterator(); ActorIt; ++ActorIt)
		{
			if (ActorIt.Value() == RemovedTargetId)
			{
				ActorIt.RemoveCurrent();
				break;
			}
		}
		It.RemoveCurrent();
	}
}

bool USWMinionTargetRegistrySubsystem::IsLegalTarget(const FTargetEntry& Entry, const FSWMinionTargetQuery& Query, float& OutDistanceSquared) const
{
	AActor* const Candidate = Entry.Actor.Get();
	if (!Candidate || Candidate == Query.SourceActor
		|| !Candidate->Implements<USWCombatInterface>()
		|| !Candidate->Implements<USWTeamInterface>()
		|| ISWCombatInterface::Execute_IsDead(Candidate))
	{
		return false;
	}

	const ISWTeamInterface* const CandidateTeamOwner = Cast<ISWTeamInterface>(Candidate);
	const ESWTeamId CandidateTeam = CandidateTeamOwner ? CandidateTeamOwner->GetTeamId() : ESWTeamId::None;
	if (CandidateTeam == ESWTeamId::None || CandidateTeam == Query.SourceTeam)
	{
		return false;
	}

	// M11 地面兵线移动、攻击停靠和 Leash 都在 XY 平面执行；索敌必须使用同一
	// 距离语义，不能因 Capsule 原点或地形投射的 Z 差让可接近目标永远无法被获取。
	OutDistanceSquared = FVector::DistSquared2D(Query.SourceLocation, Candidate->GetActorLocation());
	return OutDistanceSquared <= FMath::Square(Query.AcquisitionRange);
}

ESWMinionTargetCategory USWMinionTargetRegistrySubsystem::ClassifyTarget(const AActor& TargetActor)
{
	if (TargetActor.IsA<ASWCharacter_Minion>())
	{
		return ESWMinionTargetCategory::Minion;
	}

	return TargetActor.IsA<ASWCharacter_Player>() ? ESWMinionTargetCategory::Player : ESWMinionTargetCategory::Structure;
}
