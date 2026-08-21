// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "Mass/Minions/SWMinionSpawnInitializerProcessor.h"

#include "MassCommonFragments.h"
#include "MassEntityManager.h"
#include "MassExecutionContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SWMinionSpawnInitializerProcessor)

USWMinionSpawnInitializerProcessor::USWMinionSpawnInitializerProcessor()
	: EntityQuery(*this)
{
	// 该 Processor 只能由 UMassSpawnerSubsystem 在创建批次时显式调用，绝不进入逐帧调度。
	bAutoRegisterWithProcessingPhases = false;
}

void USWMinionSpawnInitializerProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FSWMinionIdentityFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FSWMinionTeamFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FSWMinionLaneFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FSWMinionCombatFragment>(EMassFragmentAccess::ReadWrite);
}

void USWMinionSpawnInitializerProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	if (!ensure(Context.ValidateAuxDataType<FSWMinionSpawnData>()))
	{
		return;
	}

	const FSWMinionSpawnData& SpawnData = Context.GetAuxData().Get<FSWMinionSpawnData>();
	int32 NextEntryIndex = 0;
	EntityQuery.ForEachEntityChunk(Context, [&SpawnData, &NextEntryIndex](FMassExecutionContext& ChunkContext)
	{
		const int32 NumEntities = ChunkContext.GetNumEntities();
		if (!ensure(NextEntryIndex + NumEntities <= SpawnData.Entries.Num()))
		{
			return;
		}

		TArrayView<FTransformFragment> Transforms = ChunkContext.GetMutableFragmentView<FTransformFragment>();
		TArrayView<FSWMinionIdentityFragment> Identities = ChunkContext.GetMutableFragmentView<FSWMinionIdentityFragment>();
		TArrayView<FSWMinionTeamFragment> Teams = ChunkContext.GetMutableFragmentView<FSWMinionTeamFragment>();
		TArrayView<FSWMinionLaneFragment> Lanes = ChunkContext.GetMutableFragmentView<FSWMinionLaneFragment>();
		TArrayView<FSWMinionCombatFragment> Combats = ChunkContext.GetMutableFragmentView<FSWMinionCombatFragment>();

		for (int32 EntityIndex = 0; EntityIndex < NumEntities; ++EntityIndex)
		{
			const FSWMinionSpawnEntry& Entry = SpawnData.Entries[NextEntryIndex++];
			Transforms[EntityIndex].GetMutableTransform() = Entry.SpawnTransform;
			Identities[EntityIndex].UnitId = Entry.UnitId;
			Identities[EntityIndex].WaveIndex = Entry.WaveIndex;
			Identities[EntityIndex].SpawnOrdinal = Entry.SpawnOrdinal;
			Teams[EntityIndex].TeamId = Entry.TeamId;
			Lanes[EntityIndex].LaneId = Entry.LaneId;
			Lanes[EntityIndex].Direction = Entry.LaneDirection;
			Lanes[EntityIndex].DistanceAlongLane = Entry.DistanceAlongLane;
			Lanes[EntityIndex].LateralOffset = Entry.LateralOffset;
			Lanes[EntityIndex].VerticalOffset = Entry.VerticalOffset;
			Combats[EntityIndex].AttackRange = Entry.AttackRange;
		}
	});

	ensureMsgf(NextEntryIndex == SpawnData.Entries.Num(), TEXT("小兵 SpawnData 数量与 MassSpawner 创建的 Entity 数量不一致。"));
}
