// Copyright (c) 2026 ZhangJian Limited. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Team/SWTeamTypes.h"
#include "UObject/ObjectKey.h"
#include "SWCombatTargetRegistrySubsystem.generated.h"

class AActor;

/** 用于稳定排序和 M12 结构扩展的目标类别。 */
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

/**
 * 已注册目标的只读身份信息。
 * 该结构不授予任何目标选择所有权；结构索敌组件仅用它取得稳定破平局 ID 与类别。
 */
struct FSWMinionRegisteredTargetInfo
{
	uint32 TargetId = 0;
	ESWMinionTargetCategory Category = ESWMinionTargetCategory::None;

	bool IsValid() const { return TargetId != 0 && Category != ESWMinionTargetCategory::None; }
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
 * 服务器 World 的全局可战斗 Actor 弱引用索引。
 * 它不拥有任何阵营、AI 或 Combatant 状态：仅提供稳定身份与只读查询时的 Combat/Team/Targetable 复核。
 */
UCLASS()
class POLYGONSCIFIWORLDS_API USWCombatTargetRegistrySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	FSWMinionTargetRegistrationHandle RegisterTarget(AActor& TargetActor);
	void UnregisterTarget(const AActor& TargetActor);
	/** 只读返回既有注册身份；不会注册、清理或改写小兵目标选择状态。 */
	bool TryGetRegisteredTargetInfo(const AActor& TargetActor, FSWMinionRegisteredTargetInfo& OutInfo) const;
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
