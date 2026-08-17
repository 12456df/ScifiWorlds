#include "Lane/SWLaneRoute.h"

#include "Components/SplineComponent.h"
#include "Math/RotationMatrix.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SWLaneRoute)

DEFINE_LOG_CATEGORY_STATIC(LogSWLaneRoute, Log, All);

ASWLaneRoute::ASWLaneRoute()
{
	PrimaryActorTick.bCanEverTick = false;
	SetActorEnableCollision(false);

	LaneSpline = CreateDefaultSubobject<USplineComponent>(TEXT("LaneSpline"));
	SetRootComponent(LaneSpline);
}

void ASWLaneRoute::BeginPlay()
{
	Super::BeginPlay();

	// 客户端不参与路线校验、波次或小兵生成决策；路线真值只在 Server/Standalone World 固化。
	if (GetNetMode() == NM_Client)
	{
		return;
	}

	if (!BuildRouteSnapshot())
	{
		UE_LOG(LogSWLaneRoute, Error, TEXT("Lane route '%s' is invalid: %s"), *GetName(), *ValidationError);
	}
}

void ASWLaneRoute::ValidateRouteInEditor()
{
	FString Failure;
	if (ValidateSpline(Failure))
	{
		UE_LOG(LogSWLaneRoute, Display, TEXT("Lane route '%s' passed editor validation."), *GetName());
	}
	else
	{
		UE_LOG(LogSWLaneRoute, Error, TEXT("Lane route '%s' failed editor validation: %s"), *GetName(), *Failure);
	}
}

bool ASWLaneRoute::TryGetRouteSnapshot(FSWLaneRouteSnapshot& OutSnapshot) const
{
	if (!bHasValidRouteSnapshot)
	{
		return false;
	}

	OutSnapshot = RouteSnapshot;
	return true;
}

ESWLaneDirection ASWLaneRoute::GetDirectionForTeam(const ESWTeamId TeamId)
{
	switch (TeamId)
	{
	case ESWTeamId::TeamA:
		return ESWLaneDirection::Forward;
	case ESWTeamId::TeamB:
		return ESWLaneDirection::Reverse;
	default:
		return ESWLaneDirection::None;
	}
}

bool ASWLaneRoute::BuildRouteSnapshot()
{
	bHasValidRouteSnapshot = false;
	ValidationError.Reset();

	if (!ValidateSpline(ValidationError))
	{
		return false;
	}

	const int32 LastPointIndex = LaneSpline->GetNumberOfSplinePoints() - 1;
	const float SplineLength = LaneSpline->GetSplineLength();
	const FTransform TeamATransform = LaneSpline->GetTransformAtSplinePoint(0, ESplineCoordinateSpace::World);
	FTransform TeamBTransform = LaneSpline->GetTransformAtSplinePoint(LastPointIndex, ESplineCoordinateSpace::World);

	// TeamB 从同一终点反向推进，故其出生朝向使用反向切线，而非维护第二份镜像 Spline。
	const FVector TeamBForward = -LaneSpline->GetDirectionAtSplinePoint(LastPointIndex, ESplineCoordinateSpace::World);
	const FVector TeamBUp = LaneSpline->GetUpVectorAtSplinePoint(LastPointIndex, ESplineCoordinateSpace::World);
	TeamBTransform.SetRotation(FRotationMatrix::MakeFromXZ(TeamBForward, TeamBUp).ToQuat());

	RouteSnapshot.LaneId = LaneId;
	RouteSnapshot.Length = SplineLength;
	RouteSnapshot.TeamASpawnTransform = TeamATransform;
	RouteSnapshot.TeamBSpawnTransform = TeamBTransform;
	bHasValidRouteSnapshot = true;
	return true;
}

bool ASWLaneRoute::ValidateSpline(FString& OutFailure) const
{
	OutFailure.Reset();

	if (LaneId == ESWLaneId::None)
	{
		OutFailure = TEXT("LaneId must be Top, Middle, or Bottom.");
		return false;
	}

	if (!LaneSpline)
	{
		OutFailure = TEXT("LaneSpline component is missing.");
		return false;
	}

	const int32 PointCount = LaneSpline->GetNumberOfSplinePoints();
	if (PointCount < 2)
	{
		OutFailure = TEXT("LaneSpline requires at least two control points.");
		return false;
	}

	const float SplineLength = LaneSpline->GetSplineLength();
	if (SplineLength < MinimumRouteLength)
	{
		OutFailure = FString::Printf(TEXT("Spline length %.1f is below the minimum %.1f."), SplineLength, MinimumRouteLength);
		return false;
	}

	const FVector FirstPoint = LaneSpline->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::World);
	const FVector LastPoint = LaneSpline->GetLocationAtSplinePoint(PointCount - 1, ESplineCoordinateSpace::World);
	if (FVector::DistSquared(FirstPoint, LastPoint) < FMath::Square(MinimumEndpointSeparation))
	{
		OutFailure = FString::Printf(TEXT("Spline endpoints are closer than %.1f units."), MinimumEndpointSeparation);
		return false;
	}

	if (LaneSpline->GetDirectionAtSplinePoint(0, ESplineCoordinateSpace::World).IsNearlyZero()
		|| LaneSpline->GetDirectionAtSplinePoint(PointCount - 1, ESplineCoordinateSpace::World).IsNearlyZero())
	{
		OutFailure = TEXT("Spline endpoint tangent cannot be zero.");
		return false;
	}

	return true;
}
