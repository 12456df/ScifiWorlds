// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "Mass/Minions/Processors/SWMinionSeparationProcessor.h"

#include "Mass/Minions/Processors/SWMinionLaneMovementProcessor.h"
#include "Mass/Minions/SWMinionMassTypes.h"
#include "MassCommonFragments.h"
#include "MassCommonTypes.h"
#include "MassEntityManager.h"
#include "MassExecutionContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SWMinionSeparationProcessor)

namespace SWMinionSeparation
{
	struct FEntry
	{
		FMassEntityHandle Entity;
		ESWTeamId TeamId = ESWTeamId::None;
		ESWLaneId LaneId = ESWLaneId::None;
		FVector Location = FVector::ZeroVector;
		float CollisionRadius = 0.f;
		float SameTeamSeparationMultiplier = 1.f;
	};
}

USWMinionSeparationProcessor::USWMinionSeparationProcessor()
	: EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = false;
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::Standalone | EProcessorExecutionFlags::Server);
	ProcessingPhase = EMassProcessingPhase::PrePhysics;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Movement;
	ExecutionOrder.ExecuteAfter.Add(USWMinionLaneMovementProcessor::StaticClass()->GetFName());
	// 需要跨 Chunk 收集并回写同一帧的 Mass Fragment，因此显式限制 Game Thread。
	bRequiresGameThreadExecution = true;
}

void USWMinionSeparationProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FSWMinionSpatialFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FSWMinionTeamFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FSWMinionLaneFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddConstSharedRequirement<FSWMinionArchetypeSharedFragment>();
	EntityQuery.AddTagRequirement<FSWMinionReadyTag>(EMassFragmentPresence::All);
	EntityQuery.AddTagRequirement<FSWMinionDeadTag>(EMassFragmentPresence::None);
}

void USWMinionSeparationProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	TArray<SWMinionSeparation::FEntry> Entries;
	float MaxCollisionRadius = 0.f;
	float MaxSameTeamSeparationMultiplier = 1.f;
	EntityQuery.ForEachEntityChunk(Context, [&Entries, &MaxCollisionRadius, &MaxSameTeamSeparationMultiplier](FMassExecutionContext& ChunkContext)
	{
		const FSWMinionArchetypeSharedFragment& Archetype = ChunkContext.GetConstSharedFragment<FSWMinionArchetypeSharedFragment>();
		const TConstArrayView<FTransformFragment> Transforms = ChunkContext.GetFragmentView<FTransformFragment>();
		const TConstArrayView<FSWMinionSpatialFragment> Spatials = ChunkContext.GetFragmentView<FSWMinionSpatialFragment>();
		const TConstArrayView<FSWMinionTeamFragment> Teams = ChunkContext.GetFragmentView<FSWMinionTeamFragment>();
		const TConstArrayView<FSWMinionLaneFragment> Lanes = ChunkContext.GetFragmentView<FSWMinionLaneFragment>();

		for (FMassExecutionContext::FEntityIterator EntityIt = ChunkContext.CreateEntityIterator(); EntityIt; ++EntityIt)
		{
			const float CollisionRadius = Spatials[EntityIt].CollisionRadius;
			if (CollisionRadius <= KINDA_SMALL_NUMBER)
			{
				continue;
			}

			SWMinionSeparation::FEntry& Entry = Entries.AddDefaulted_GetRef();
			Entry.Entity = ChunkContext.GetEntity(EntityIt);
			Entry.TeamId = Teams[EntityIt].TeamId;
			Entry.LaneId = Lanes[EntityIt].LaneId;
			Entry.Location = Transforms[EntityIt].GetTransform().GetLocation();
			Entry.CollisionRadius = CollisionRadius;
			Entry.SameTeamSeparationMultiplier = FMath::Max(1.f, Archetype.SameTeamSeparationMultiplier);
			MaxCollisionRadius = FMath::Max(MaxCollisionRadius, CollisionRadius);
			MaxSameTeamSeparationMultiplier = FMath::Max(MaxSameTeamSeparationMultiplier, Entry.SameTeamSeparationMultiplier);
		}
	});

	if (Entries.Num() < 2 || MaxCollisionRadius <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	// 与全局 O(N^2) 检查相比，二维格仅检查相邻 3x3 格。首版波次规模下仍会跨 Chunk 正确工作。
	// 格边长必须覆盖当前配置的最大同队期望间距；否则倍率高于默认值时，
	// 相邻但跨出固定 1.15 倍格范围的单位会被漏检，造成“配置已提高但未完全分散”。
	const float CellSize = MaxCollisionRadius * 2.f * MaxSameTeamSeparationMultiplier;
	TArray<FVector2D> ResolvedLocations;
	ResolvedLocations.Reserve(Entries.Num());
	for (const SWMinionSeparation::FEntry& Entry : Entries)
	{
		ResolvedLocations.Add(FVector2D(Entry.Location.X, Entry.Location.Y));
	}

	// 小规模位置约束迭代；每轮重建格，覆盖同一帧多个单位同时汇聚后产生的新相邻关系。
	for (int32 Iteration = 0; Iteration < 4; ++Iteration)
	{
		TMap<FIntPoint, TArray<int32>> SpatialGrid;
		SpatialGrid.Reserve(Entries.Num());
		for (int32 Index = 0; Index < Entries.Num(); ++Index)
		{
			SpatialGrid.FindOrAdd(FIntPoint(
				FMath::FloorToInt(ResolvedLocations[Index].X / CellSize),
				FMath::FloorToInt(ResolvedLocations[Index].Y / CellSize))).Add(Index);
		}

		TArray<FVector2D> Corrections;
		Corrections.Init(FVector2D::ZeroVector, Entries.Num());
		for (int32 Index = 0; Index < Entries.Num(); ++Index)
		{
			const FIntPoint Cell(FMath::FloorToInt(ResolvedLocations[Index].X / CellSize), FMath::FloorToInt(ResolvedLocations[Index].Y / CellSize));
			for (int32 OffsetX = -1; OffsetX <= 1; ++OffsetX)
			{
				for (int32 OffsetY = -1; OffsetY <= 1; ++OffsetY)
				{
					const TArray<int32>* const Candidates = SpatialGrid.Find(Cell + FIntPoint(OffsetX, OffsetY));
					if (!Candidates)
					{
						continue;
					}
					for (const int32 OtherIndex : *Candidates)
					{
						if (OtherIndex <= Index)
						{
							continue;
						}

						const bool bSameTeamAndLane = Entries[Index].TeamId == Entries[OtherIndex].TeamId
							&& Entries[Index].LaneId == Entries[OtherIndex].LaneId;
						const float RequiredDistance = (Entries[Index].CollisionRadius + Entries[OtherIndex].CollisionRadius)
							* (bSameTeamAndLane ? FMath::Max(Entries[Index].SameTeamSeparationMultiplier, Entries[OtherIndex].SameTeamSeparationMultiplier) : 1.f);
						const FVector2D Delta = ResolvedLocations[Index] - ResolvedLocations[OtherIndex];
						const float DistanceSquared = Delta.SquaredLength();
						if (DistanceSquared >= FMath::Square(RequiredDistance))
						{
							continue;
						}

						const float Distance = FMath::Sqrt(FMath::Max(DistanceSquared, KINDA_SMALL_NUMBER));
						const FVector2D Direction = DistanceSquared > KINDA_SMALL_NUMBER
							? Delta / Distance
							: (Index < OtherIndex ? FVector2D(1.f, 0.f) : FVector2D(-1.f, 0.f));
						const FVector2D Correction = Direction * ((RequiredDistance - Distance) * 0.5f);
						Corrections[Index] += Correction;
						Corrections[OtherIndex] -= Correction;
					}
				}
			}
		}

		for (int32 Index = 0; Index < Entries.Num(); ++Index)
		{
			ResolvedLocations[Index] += Corrections[Index];
		}
	}

	for (int32 Index = 0; Index < Entries.Num(); ++Index)
	{
		if (!EntityManager.IsEntityValid(Entries[Index].Entity))
		{
			continue;
		}

		FTransformFragment* const TransformFragment = EntityManager.GetFragmentDataPtr<FTransformFragment>(Entries[Index].Entity);
		FSWMinionSpatialFragment* const SpatialFragment = EntityManager.GetFragmentDataPtr<FSWMinionSpatialFragment>(Entries[Index].Entity);
		if (!TransformFragment || !SpatialFragment)
		{
			continue;
		}

		const FVector CurrentLocation = TransformFragment->GetTransform().GetLocation();
		const FVector NewLocation(ResolvedLocations[Index].X, ResolvedLocations[Index].Y, CurrentLocation.Z);
		const FVector AppliedCorrection = NewLocation - CurrentLocation;
		if (!AppliedCorrection.IsNearlyZero())
		{
			TransformFragment->GetMutableTransform().SetLocation(NewLocation);
			SpatialFragment->SeparationOffset += AppliedCorrection;
		}
	}
}
