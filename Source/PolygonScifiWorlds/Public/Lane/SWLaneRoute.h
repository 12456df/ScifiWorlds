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
