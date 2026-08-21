// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#include "Mass/Minions/Processors/SWMinionActorSyncProcessor.h"

#include "Character/SWCharacter_Minion.h"
#include "Mass/Minions/SWMinionMassTypes.h"
#include "MassActorSubsystem.h"
#include "MassCommonFragments.h"
#include "MassCommonTypes.h"
#include "MassEntityManager.h"
#include "MassExecutionContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SWMinionActorSyncProcessor)

USWMinionActorSyncProcessor::USWMinionActorSyncProcessor()
	: EntityQuery(*this)
{
	// 由 USWMinionLaneWaveSubsystem 按 World 生命周期显式动态注册。
	bAutoRegisterWithProcessingPhases = false;
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::Standalone | EProcessorExecutionFlags::Server);
	ProcessingPhase = EMassProcessingPhase::PrePhysics;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::UpdateWorldFromMass;
	ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::Movement);
	// Actor 写入、碰撞与网络复制均只能在 Game Thread 发生。
	bRequiresGameThreadExecution = true;
}

void USWMinionActorSyncProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	// 需要从 Fragment 取得可写 Actor 指针以同步其 Transform；不修改 Fragment 自身的关联关系。
	EntityQuery.AddRequirement<FMassActorFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddTagRequirement<FSWMinionReadyTag>(EMassFragmentPresence::All);
	EntityQuery.AddTagRequirement<FSWMinionDeadTag>(EMassFragmentPresence::None);
}

void USWMinionActorSyncProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	const float DeltaSeconds = FMath::Max(Context.GetDeltaTimeSeconds(), KINDA_SMALL_NUMBER);
	EntityQuery.ForEachEntityChunk(Context, [DeltaSeconds](FMassExecutionContext& ChunkContext)
	{
		const TConstArrayView<FTransformFragment> Transforms = ChunkContext.GetFragmentView<FTransformFragment>();
		TArrayView<FMassActorFragment> Actors = ChunkContext.GetMutableFragmentView<FMassActorFragment>();

		for (FMassExecutionContext::FEntityIterator EntityIt = ChunkContext.CreateEntityIterator(); EntityIt; ++EntityIt)
		{
			ASWCharacter_Minion* const MinionActor = Cast<ASWCharacter_Minion>(Actors[EntityIt].GetMutable(FMassActorFragment::EActorAccess::OnlyWhenAlive));
			if (!MinionActor || !MinionActor->HasAuthority())
			{
				continue;
			}

			const FTransform& MassTransform = Transforms[EntityIt].GetTransform();
			const FVector PreviousActorLocation = MinionActor->GetActorLocation();
			// UE 的 AActor Movement Replication 通过 GetVelocity() 写 FRepMovement。
			// Mass 直接写 Transform 时必须同步提供连续的权威速度；否则客户端只能收到
			// 位置跳变与零速度，导致 AnimBP 站立平移。
			MinionActor->SetMassVisualVelocityAuthority((MassTransform.GetLocation() - PreviousActorLocation) / DeltaSeconds);
			if (!MinionActor->GetActorTransform().Equals(MassTransform, 0.01f))
			{
				MinionActor->SetActorTransform(MassTransform, false, nullptr, ETeleportType::TeleportPhysics);
			}
		}
	});
}
