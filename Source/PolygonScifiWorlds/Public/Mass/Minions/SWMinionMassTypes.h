#pragma once

#include "CoreMinimal.h"
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

	UPROPERTY(EditAnywhere, Category = "Minion")
	float AttackRange = 250.f;
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
