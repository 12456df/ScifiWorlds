#pragma once

#include "CoreMinimal.h"
#include "MassCommonFragments.h"
#include "MassEntityTypes.h"
#include "Team/SWTeamTypes.h"
#include "SWMinionMassTypes.generated.h"

/** 三路兵线的稳定身份；None 仅表示尚未由服务器初始化。 */
UENUM(BlueprintType)
enum class ESWLaneId : uint8
{
	None UMETA(DisplayName = "None"),
	Top UMETA(DisplayName = "Top"),
	Middle UMETA(DisplayName = "Middle"),
	Bottom UMETA(DisplayName = "Bottom"),
};

/** 同一条路线中两队相反的战略推进方向。 */
UENUM(BlueprintType)
enum class ESWLaneDirection : uint8
{
	None UMETA(DisplayName = "None"),
	Forward UMETA(DisplayName = "Forward"),
	Reverse UMETA(DisplayName = "Reverse"),
};

/**
 * Factory 在批量生成时仅写入一次的小兵身份数据。
 * UnitId 是定义资产的稳定标识，不使用短生命周期的 Mass Entity Handle 作为存档或玩法 ID。
 */
USTRUCT()
struct FSWMinionIdentityFragment : public FMassFragment
{
	GENERATED_BODY()

	FName UnitId = NAME_None;
	int32 WaveIndex = INDEX_NONE;
	int32 SpawnOrdinal = INDEX_NONE;
};

/**
 * 为 Mass Chunk 查询保留的队伍快照。
 * 真正可变的 TeamId 仍由未来的 Minion Actor/ASC 持有；小兵生命周期内不允许修改该 Fragment。
 */
USTRUCT()
struct FSWMinionTeamFragment : public FMassFragment
{
	GENERATED_BODY()

	ESWTeamId TeamId = ESWTeamId::None;
};

/** M10 Factory 写入初始路线位置；M11 的沿线移动仅更新 DistanceAlongLane。 */
USTRUCT()
struct FSWMinionLaneFragment : public FMassFragment
{
	GENERATED_BODY()

	ESWLaneId LaneId = ESWLaneId::None;
	ESWLaneDirection Direction = ESWLaneDirection::None;
	float DistanceAlongLane = 0.f;
	/** 相对路线中心线的横向/竖向编队偏移，防止同波 Entity 在首次推进时收拢到同一点。 */
	float LateralOffset = 0.f;
	float VerticalOffset = 0.f;
	/** 由桥接 Character 的 Capsule 半高初始化，用于让 Actor 原点稳定落在已投射的路线地面之上。 */
	float GroundOffset = 0.f;
};

/**
 * 服务器 Mass 空间真值。CollisionRadius 在 Actor Bridge 创建后从实际 Capsule 写入；
 * SeparationOffset 只由 Separation Processor 写入，并由 Movement Processor 消费，避免 Actor 同步
 * 使用 TeleportPhysics 时绕过 Capsule Sweep 而让小兵视觉/碰撞体重叠。
 */
USTRUCT()
struct FSWMinionSpatialFragment : public FMassFragment
{
	GENERATED_BODY()

	float CollisionRadius = 0.f;
	FVector SeparationOffset = FVector::ZeroVector;
};

/** StateTree 对小兵输出的唯一行为意图；具体移动、索敌与攻击 Processor 只消费该值，不自行维护第二套状态。 */
UENUM(BlueprintType)
enum class ESWMinionBehaviorIntent : uint8
{
	None UMETA(DisplayName = "None"),
	Advancing UMETA(DisplayName = "Advancing"),
	Engaging UMETA(DisplayName = "Engaging"),
	Attacking UMETA(DisplayName = "Attacking"),
	Returning UMETA(DisplayName = "Returning"),
};

/** M11 StateTree 写入的瞬时行为输出；本阶段只使用 Behavior，其他字段留给后续 Processor 消费。 */
USTRUCT()
struct FSWMinionIntentFragment : public FMassFragment
{
	GENERATED_BODY()

	ESWMinionBehaviorIntent Behavior = ESWMinionBehaviorIntent::None;
	FVector DesiredVelocity = FVector::ZeroVector;
	bool bAttackRequested = false;
	/** Returning 移动 Processor 重新并入前方路线后写入，StateTree 只读此值并切回 Advancing。 */
	bool bReachedReturnAnchor = false;
};

/**
 * 当前低频感知结果。只保存弱引用和诊断数据，不拥有 Target 的 Team、生命或生命周期真值。
 * 只能由服务器 Targeting Processor 写入，Actor 销毁后弱引用会自动失效。
 */
USTRUCT()
struct FSWMinionTargetFragment : public FObjectWrapperFragment
{
	GENERATED_BODY()

	TWeakObjectPtr<AActor> TargetActor;
	uint32 TargetId = 0;
	float LastValidServerTime = 0.f;
	bool bIsWithinAttackRange = false;
};

/** 出生时从 Minion Definition 写入的战斗运行时快照；Actor 与 Mass 共享同一攻击距离真值。 */
USTRUCT()
struct FSWMinionCombatFragment : public FMassFragment
{
	GENERATED_BODY()

	float AttackRange = 0.f;
};

/** 当前追击锚点；M11-3 仅在首次获得目标时写入，实际追击与回线移动由后续 Processor 消费。 */
USTRUCT()
struct FSWMinionLeashFragment : public FMassFragment
{
	GENERATED_BODY()

	float AnchorLaneDistance = 0.f;
	/** StateTree 进入 Returning 时置位；Movement 只在该边沿把当前世界位置投影到路线，保证脱战不会倒退。 */
	bool bNeedsForwardLaneRejoinProjection = false;
};

/** 小兵低频感知和后续尸体清理共用的服务器时间数据。 */
USTRUCT()
struct FSWMinionTimingFragment : public FMassFragment
{
	GENERATED_BODY()

	float NextSenseServerTime = 0.f;
	float CleanupServerTime = 0.f;
};

/**
 * 同一 Archetype 的不可变行为数值快照。
 * 具体数值由 EntityConfig/后续 MinionDefinition 配置；不在此保存生命、目标或其他运行时状态。
 */
USTRUCT(BlueprintType)
struct FSWMinionArchetypeSharedFragment : public FMassConstSharedFragment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Minion")
	float MoveSpeed = 400.f;

	UPROPERTY(EditAnywhere, Category = "Minion")
	float TargetingRange = 1500.f;

	/** 每个 Entity 的低频索敌间隔；Processor 会按稳定 Entity Id 错峰，不创建 Timer。 */
	UPROPERTY(EditAnywhere, Category = "Minion", meta = (ClampMin = "0.05"))
	float TargetScanIntervalSeconds = 0.25f;

	/** 获得目标时允许离开当前路线锚点的最大距离；追击 Processor 在 M11-4 消费。 */
	UPROPERTY(EditAnywhere, Category = "Minion", meta = (ClampMin = "0.0"))
	float LeashDistance = 600.f;

	/** 追击停靠时保留在 AttackRange 内侧的安全余量，抵消分离修正与浮点边界造成的“已停下但未进入攻击”情况。 */
	UPROPERTY(EditAnywhere, Category = "Minion|Combat", meta = (ClampMin = "0.0"))
	float AttackRangeArrivalBuffer = 25.f;

	/** 脱战归线时相对常规推进速度的倍率；必须大于等于 1，才能在持续前进的同时收敛回路线。 */
	UPROPERTY(EditAnywhere, Category = "Minion|Lane Rejoin", meta = (ClampMin = "1.0"))
	float LaneRejoinSpeedMultiplier = 1.25f;

	/** 到前方路线采样点的二维距离不超过该值时，Returning 完成并交还给 StateTree。 */
	UPROPERTY(EditAnywhere, Category = "Minion|Lane Rejoin", meta = (ClampMin = "0.0"))
	float LaneRejoinTolerance = 20.f;

	/** 同队、同 Lane 的期望中心间距相对两 Capsule 半径之和的倍率；敌对小兵仅保持 Capsule 不重叠。 */
	UPROPERTY(EditAnywhere, Category = "Minion|Separation", meta = (ClampMin = "1.0"))
	float SameTeamSeparationMultiplier = 1.15f;

	/** 无相邻单位时，沿线推进逐步回收临时分离偏移的速度（cm/s）。 */
	UPROPERTY(EditAnywhere, Category = "Minion|Separation", meta = (ClampMin = "0.0"))
	float SeparationRelaxationSpeed = 300.f;

	/** 已被附近单位挤出路线时，追击目标额外采用的侧向转向权重；0 表示只直线追击。 */
	UPROPERTY(EditAnywhere, Category = "Minion|Separation", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CombatDetourSteeringStrength = 0.65f;

};

/** 仅在 Entity、Actor 与 ASC 均完成权威初始化后，由 M10-5 Factory 添加。 */
USTRUCT()
struct FSWMinionReadyTag : public FMassTag
{
	GENERATED_BODY()
};

/** M11 死亡桥使用的结构标记；M10 不会主动添加。 */
USTRUCT()
struct FSWMinionDeadTag : public FMassTag
{
	GENERATED_BODY()
};

/**
 * 为后续专用 Signal Consumer 预留的稳定名称；默认 UMassStateTreeProcessor 不会自动订阅自定义名称。
 * M11-3 使用引擎的 UE::Mass::Signals::NewStateTreeTaskRequired 唤醒 StateTree。
 */
namespace SWMinionSignals
{
	inline const FName TargetUpdated(TEXT("SW.Minion.TargetUpdated"));
	inline const FName TargetInvalidated(TEXT("SW.Minion.TargetInvalidated"));
	inline const FName AttackFinished(TEXT("SW.Minion.AttackFinished"));
	inline const FName DeathCommitted(TEXT("SW.Minion.DeathCommitted"));
}
