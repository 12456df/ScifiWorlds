#include "Lane/SWLaneRoute.h"

#include "Components/SplineComponent.h"
#include "Engine/World.h"
#include "Math/RotationMatrix.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SWLaneRoute)

DEFINE_LOG_CATEGORY_STATIC(LogSWLaneRoute, Log, All);

bool FSWLaneRouteSnapshot::TrySampleTransform(const float DistanceAlongLane, const ESWLaneDirection Direction, FTransform& OutTransform) const
{
	if (Length <= 0.f || Direction == ESWLaneDirection::None
		|| SampleDistances.Num() < 2
		|| SampleLocations.Num() != SampleDistances.Num()
		|| SampleDirections.Num() != SampleDistances.Num()
		|| SampleUps.Num() != SampleDistances.Num())
	{
		return false;
	}

	const float ClampedDistance = FMath::Clamp(DistanceAlongLane, 0.f, Length);
	const int32 LastIndex = SampleDistances.Num() - 1;
	int32 UpperIndex = 1;
	while (UpperIndex < LastIndex && SampleDistances[UpperIndex] < ClampedDistance)
	{
		++UpperIndex;
	}

	const int32 LowerIndex = UpperIndex - 1;
	const float LowerDistance = SampleDistances[LowerIndex];
	const float UpperDistance = SampleDistances[UpperIndex];
	const float Alpha = FMath::IsNearlyEqual(LowerDistance, UpperDistance)
		? 0.f
		: (ClampedDistance - LowerDistance) / (UpperDistance - LowerDistance);

	const FVector Location = FMath::Lerp(SampleLocations[LowerIndex], SampleLocations[UpperIndex], Alpha);
	FVector Forward = FMath::Lerp(SampleDirections[LowerIndex], SampleDirections[UpperIndex], Alpha).GetSafeNormal();
	const FVector Up = FMath::Lerp(SampleUps[LowerIndex], SampleUps[UpperIndex], Alpha).GetSafeNormal();
	if (Forward.IsNearlyZero() || Up.IsNearlyZero())
	{
		return false;
	}

	if (Direction == ESWLaneDirection::Reverse)
	{
		Forward *= -1.f;
	}

	OutTransform = FTransform(FRotationMatrix::MakeFromXZ(Forward, Up).ToQuat(), Location, FVector::OneVector);
	return true;
}

bool FSWLaneRouteSnapshot::TryProjectDistanceAlongLane(const FVector& WorldLocation, float& OutDistanceAlongLane) const
{
	if (Length <= 0.f || SampleDistances.Num() < 2 || SampleLocations.Num() != SampleDistances.Num())
	{
		return false;
	}

	float BestDistanceSquared = TNumericLimits<float>::Max();
	float BestLaneDistance = 0.f;
	for (int32 Index = 1; Index < SampleLocations.Num(); ++Index)
	{
		const FVector SegmentStart = SampleLocations[Index - 1];
		const FVector SegmentDelta = SampleLocations[Index] - SegmentStart;
		const float SegmentLengthSquared = SegmentDelta.SizeSquared2D();
		const float Alpha = SegmentLengthSquared > KINDA_SMALL_NUMBER
			? FMath::Clamp(FVector2D::DotProduct(
				FVector2D(WorldLocation.X - SegmentStart.X, WorldLocation.Y - SegmentStart.Y),
				FVector2D(SegmentDelta.X, SegmentDelta.Y)) / SegmentLengthSquared, 0.f, 1.f)
			: 0.f;
		const FVector ClosestPoint = SegmentStart + SegmentDelta * Alpha;
		const float DistanceSquared = FVector::DistSquared2D(WorldLocation, ClosestPoint);
		if (DistanceSquared < BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			BestLaneDistance = FMath::Lerp(SampleDistances[Index - 1], SampleDistances[Index], Alpha);
		}
	}

	OutDistanceAlongLane = BestLaneDistance;
	return true;
}

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
	RouteSnapshot.SampleDistances.Reset();
	RouteSnapshot.SampleLocations.Reset();
	RouteSnapshot.SampleDirections.Reset();
	RouteSnapshot.SampleUps.Reset();

	const float SampleSpacing = FMath::Max(RuntimeSampleSpacing, 10.f);
	TArray<float> SampleDistanceCandidates;
	SampleDistanceCandidates.Reserve(LaneSpline->GetNumberOfSplinePoints() + FMath::CeilToInt(SplineLength / SampleSpacing) + 2);
	SampleDistanceCandidates.Add(0.f);
	SampleDistanceCandidates.Add(SplineLength);

	// 固定采样用于平滑插值，同时强制保留每个控制点，防止路线在控制点附近因采样间距而失真。
	for (float Distance = SampleSpacing; Distance < SplineLength; Distance += SampleSpacing)
	{
		SampleDistanceCandidates.Add(Distance);
	}
	for (int32 PointIndex = 0; PointIndex <= LastPointIndex; ++PointIndex)
	{
		SampleDistanceCandidates.Add(LaneSpline->GetDistanceAlongSplineAtSplinePoint(PointIndex));
	}
	SampleDistanceCandidates.Sort();

	FCollisionQueryParams GroundTraceParams(SCENE_QUERY_STAT(SWMinionLaneGroundProjection), false, this);
	GroundTraceParams.bTraceComplex = false;
	int32 GroundProjectionMissCount = 0;
	for (const float CandidateDistance : SampleDistanceCandidates)
	{
		if (!RouteSnapshot.SampleDistances.IsEmpty() && FMath::IsNearlyEqual(RouteSnapshot.SampleDistances.Last(), CandidateDistance))
		{
			continue;
		}

		FVector SampleLocation = LaneSpline->GetLocationAtDistanceAlongSpline(CandidateDistance, ESplineCoordinateSpace::World);
		if (bProjectMinionRouteToGround)
		{
			FHitResult GroundHit;
			const FVector TraceStart = SampleLocation + FVector::UpVector * GroundTraceStartHeight;
			const FVector TraceEnd = SampleLocation - FVector::UpVector * GroundTraceDownDistance;
			if (GetWorld()->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, GroundTraceChannel, GroundTraceParams))
			{
				SampleLocation = GroundHit.ImpactPoint;
			}
			else
			{
				++GroundProjectionMissCount;
			}
		}

		RouteSnapshot.SampleDistances.Add(CandidateDistance);
		RouteSnapshot.SampleLocations.Add(SampleLocation);
		RouteSnapshot.SampleDirections.Add(LaneSpline->GetDirectionAtDistanceAlongSpline(CandidateDistance, ESplineCoordinateSpace::World));
		RouteSnapshot.SampleUps.Add(LaneSpline->GetUpVectorAtDistanceAlongSpline(CandidateDistance, ESplineCoordinateSpace::World));
	}

	if (GroundProjectionMissCount > 0)
	{
		UE_LOG(LogSWLaneRoute, Warning, TEXT("Lane route '%s' has %d ground-projection misses. Verify the ground collision response for channel %d."),
			*GetName(), GroundProjectionMissCount, static_cast<int32>(GroundTraceChannel.GetValue()));
	}
	UE_LOG(LogSWLaneRoute, Display, TEXT("Lane route '%s' snapshot frozen: ControlPoints=%d Samples=%d Length=%.1f."),
		*GetName(), LastPointIndex + 1, RouteSnapshot.SampleDistances.Num(), RouteSnapshot.Length);
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
