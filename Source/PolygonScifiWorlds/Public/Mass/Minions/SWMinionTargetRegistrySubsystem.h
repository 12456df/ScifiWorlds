// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Team/SWTeamTypes.h"
#include "UObject/ObjectKey.h"
#include "SWMinionTargetRegistrySubsystem.generated.h"

class AActor;

/** 用于稳定排序和后续 M12 扩展的目标类别；当前只注册小兵和玩家。 */
UENUM()
enum class ESWMinionTargetCategory : uint8
{
	None,
	Minion,
	Player,
	Structure,
};

/** Registry 返回的稳定注册句柄；0 表示注册失败或目标不存在。 */
USTRUCT()
struct FSWMinionTargetRegistrationHandle
{
	GENERATED_BODY()

	uint32 TargetId = 0;

	bool IsValid() const { return TargetId != 0; }
};

/** Targeting Processor 的只读查询输入。 */
struct FSWMinionTargetQuery
{
	const AActor* SourceActor = nullptr;
	ESWTeamId SourceTeam = ESWTeamId::None;
	FVector SourceLocation = FVector::ZeroVector;
	TWeakObjectPtr<AActor> CurrentTarget;
	uint32 CurrentTargetId = 0;
	float AcquisitionRange = 0.f;
};

/** 查询结果不拥有 Actor；调用方仅能在本帧写入自己的 Target Fragment。 */
struct FSWMinionTargetResult
{
	TWeakObjectPtr<AActor> TargetActor;
	uint32 TargetId = 0;
	ESWMinionTargetCategory Category = ESWMinionTargetCategory::None;

	bool HasTarget() const { return TargetActor.IsValid() && TargetId != 0; }
};

/**
 * 服务器 World 的可战斗 Actor 弱引用索引。
 * 它不是 Combat Manager：仅在查询时复核现有 Combat/Team Interface，绝不缓存或改写生命、队伍与死亡真值。
 */
UCLASS()
class POLYGONSCIFIWORLDS_API USWMinionTargetRegistrySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	FSWMinionTargetRegistrationHandle RegisterTarget(AActor& TargetActor);
	void UnregisterTarget(const AActor& TargetActor);
	FSWMinionTargetResult FindBestTarget(const FSWMinionTargetQuery& Query);

	/** 仅服务器诊断：先清理失效弱引用，再返回当前可查询 Target 数。 */
	int32 GetRegisteredTargetCount();

private:
	struct FTargetEntry
	{
		TWeakObjectPtr<AActor> Actor;
		uint32 TargetId = 0;
		ESWMinionTargetCategory Category = ESWMinionTargetCategory::None;
	};

	void PruneInvalidTargets();
	bool IsLegalTarget(const FTargetEntry& Entry, const FSWMinionTargetQuery& Query, float& OutDistanceSquared) const;
	static ESWMinionTargetCategory ClassifyTarget(const AActor& TargetActor);

	TMap<TObjectKey<const AActor>, uint32> ActorToTargetId;
	TMap<uint32, FTargetEntry> EntriesByTargetId;
	uint32 NextTargetId = 1;
};
