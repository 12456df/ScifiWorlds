#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Mass/Minions/SWMinionMassTypes.h"
#include "Team/SWTeamTypes.h"
#include "SWLaneRoute.generated.h"

class USplineComponent;

/**
 * 服务器在 BeginPlay 从关卡 Spline 采集的不可变路线快照。
 * 它是波次与小兵系统的只读输入，不保存 Entity Handle、波次或其他运行时玩法状态。
 */
USTRUCT()
struct FSWLaneRouteSnapshot
{
	GENERATED_BODY()

	ESWLaneId LaneId = ESWLaneId::None;
	float Length = 0.f;
	FTransform TeamASpawnTransform = FTransform::Identity;
	FTransform TeamBSpawnTransform = FTransform::Identity;

	/** 运行时只读采样点；Mass Processor 不直接读取关卡 Spline UObject。 */
	TArray<float> SampleDistances;
	TArray<FVector> SampleLocations;
	TArray<FVector> SampleDirections;
	TArray<FVector> SampleUps;

	/** 按路线距离取得推进 Transform；Reverse 自动使用反向朝向。 */
	bool TrySampleTransform(float DistanceAlongLane, ESWLaneDirection Direction, FTransform& OutTransform) const;
	/** 将世界位置投影到冻结路线的最近二维线段；只供脱战归线的边沿使用，不访问 Spline UObject。 */
	bool TryProjectDistanceAlongLane(const FVector& WorldLocation, float& OutDistanceAlongLane) const;
};

/**
 * 关卡中一条可编辑的兵线战略路线。
 * TeamA 沿 Spline 0 -> Length 推进，TeamB 使用同一 Spline 反向推进；本 Actor 不负责出兵、移动或寻路。
 */
UCLASS(BlueprintType)
class POLYGONSCIFIWORLDS_API ASWLaneRoute : public AActor
{
	GENERATED_BODY()

public:
	ASWLaneRoute();

	/** 编辑器中通过此 Spline 放置路线控制点；运行时不会修改它。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lane")
	TObjectPtr<USplineComponent> LaneSpline;

	/** 每张对局地图的 Top/Middle/Bottom 各只能放置一个有效 Route。 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Lane")
	ESWLaneId LaneId = ESWLaneId::None;

	/** 有效路线的最小总长度，避免误放置或仅含一个短控制段。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lane|Validation", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float MinimumRouteLength = 1000.f;

	/** 两端出生锚点的最小直线间距，避免闭环或端点重叠路线。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lane|Validation", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float MinimumEndpointSeparation = 100.f;

	/** 将编辑器 Spline 冻结为运行时快照时的采样间距；越小越贴合曲线，数据量也越大。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lane|Runtime", meta = (ClampMin = "10.0", UIMin = "10.0"))
	float RuntimeSampleSpacing = 100.f;

	/** 冻结路线时向下投射采样点，使 Mass 小兵使用关卡真实地面高度；不在每帧移动时做 Trace。 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Lane|Grounding")
	bool bProjectMinionRouteToGround = true;

	/** 地面投射起点相对路线采样点的高度。 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Lane|Grounding", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bProjectMinionRouteToGround"))
	float GroundTraceStartHeight = 1000.f;

	/** 地面投射终点相对路线采样点的向下距离。 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Lane|Grounding", meta = (ClampMin = "1.0", UIMin = "1.0", EditCondition = "bProjectMinionRouteToGround"))
	float GroundTraceDownDistance = 5000.f;

	/** 关卡地面响应的 Trace Channel；默认 Visibility，必要时可在关卡实例上改为专用地面通道。 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Lane|Grounding", meta = (EditCondition = "bProjectMinionRouteToGround"))
	TEnumAsByte<ECollisionChannel> GroundTraceChannel = ECC_Visibility;

	/** 仅供编辑器手动检查 Spline；不会写入运行时快照。 */
	UFUNCTION(CallInEditor, Category = "Lane|Validation")
	void ValidateRouteInEditor();

	/** Wave Subsystem 读取已冻结的路线数据；失败时 OutSnapshot 保持不变。 */
	bool TryGetRouteSnapshot(FSWLaneRouteSnapshot& OutSnapshot) const;

	/** 返回指定队伍在此路线中的推进方向；None 不是有效结果。 */
	static ESWLaneDirection GetDirectionForTeam(ESWTeamId TeamId);

	/** 最近一次验证失败原因，仅用于服务器诊断和编辑器日志。 */
	const FString& GetValidationError() const { return ValidationError; }

	virtual void BeginPlay() override;

private:
	bool BuildRouteSnapshot();
	bool ValidateSpline(FString& OutFailure) const;

	/** BeginPlay 后只读；不允许蓝图在运行时移动控制点并改变兵线事实。 */
	FSWLaneRouteSnapshot RouteSnapshot;
	bool bHasValidRouteSnapshot = false;
	FString ValidationError;
};
