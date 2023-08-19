// Copyright to Kat Code Labs, SRL. All Rights Reserved.

#include "SplineWidgetFunctionLibrary.h"

#include "SplineWidget.h"
#include "Blueprint/SlateBlueprintLibrary.h"

float USplineWidgetFunctionLibrary::GetSplineLength(const USplineWidget* InSplineWidget)
{
	check(InSplineWidget);
	return InSplineWidget->SplineCurves.GetSplineLength();
}

float USplineWidgetFunctionLibrary::GetDistanceAlongSplineAtSplineInputKey(const USplineWidget* InSplineWidget, float InKey)
{
	check(InSplineWidget);
	const int32 NumPoints = InSplineWidget->SplineCurves.Position.Points.Num();
	const int32 NumSegments = InSplineWidget->SplineData.bIsClosedLoop ? NumPoints : NumPoints - 1;

	if (InKey >= 0 && InKey < NumSegments)
	{
		const int32 PointIndex = FMath::FloorToInt(InKey);
		const float Fraction = InKey - PointIndex;
		const int32 ReparamPointIndex = PointIndex * InSplineWidget->SplineCurves.ReparamStepsPerSegment;
		const float Distance = InSplineWidget->SplineCurves.ReparamTable.Points[ReparamPointIndex].InVal;
		return Distance + InSplineWidget->SplineCurves.GetSegmentLength(PointIndex, Fraction, InSplineWidget->SplineData.bIsClosedLoop, FVector2D(1.0f));;
	}

	if (InKey >= NumSegments)
	{
		return InSplineWidget->SplineCurves.GetSplineLength();
	}

	return 0.0f;
}

FVector2D USplineWidgetFunctionLibrary::GetLocationAtSplineInputKey(const USplineWidget* InSplineWidget, float InKey, ESlateSplineCoordinateSpace CoordinateSpace)
{
	const FVector2D Location = InSplineWidget->SplineCurves.Position.Eval(InKey, FVector2D::ZeroVector);

	if (CoordinateSpace == ESlateSplineCoordinateSpace::Screen)
	{
		return InSplineWidget->GetCachedGeometry().LocalToAbsolute(Location);
	}
	
	if(CoordinateSpace == ESlateSplineCoordinateSpace::Viewport)
	{
		FVector2D PixelPosition, ViewportPosition;
		USlateBlueprintLibrary::LocalToViewport(InSplineWidget->GetWorld(), InSplineWidget->GetCachedGeometry(), Location, PixelPosition, ViewportPosition);
		return ViewportPosition;
	}
	
    return Location;
}

FVector2D USplineWidgetFunctionLibrary::GetTangentAtSplineInputKey(const USplineWidget* InSplineWidget, float InKey, ESlateSplineCoordinateSpace CoordinateSpace)
{
	const FVector2D Tangent = InSplineWidget->SplineCurves.Position.EvalDerivative(InKey, FVector2D::ZeroVector);
	if (CoordinateSpace == ESlateSplineCoordinateSpace::Screen || CoordinateSpace == ESlateSplineCoordinateSpace::Viewport)
	{
		return InSplineWidget->GetCachedGeometry().GetAccumulatedRenderTransform().TransformVector(Tangent);
	}
    return Tangent;
}

FVector2D USplineWidgetFunctionLibrary::GetDirectionAtSplineInputKey(const USplineWidget* InSplineWidget, float InKey, ESlateSplineCoordinateSpace CoordinateSpace)
{
	const FVector2D Direction = InSplineWidget->SplineCurves.Position.EvalDerivative(InKey, FVector2D::ZeroVector).GetSafeNormal();
	if (CoordinateSpace == ESlateSplineCoordinateSpace::Screen || CoordinateSpace == ESlateSplineCoordinateSpace::Viewport)
	{
		return InSplineWidget->GetCachedGeometry().GetAccumulatedRenderTransform().TransformVector(Direction);
	}
	return Direction;
}

float USplineWidgetFunctionLibrary::GetRotationAngleAtSplineInputKey(const USplineWidget* InSplineWidget, float InKey, ESlateSplineCoordinateSpace CoordinateSpace)
{
	const FVector2D Direction = GetDirectionAtSplineInputKey(InSplineWidget, InKey, CoordinateSpace);
	const float AngleInRadians = Direction.IsNearlyZero() ? 0.0f : FMath::Atan2(Direction.Y, Direction.X);
	return FMath::RadiansToDegrees(AngleInRadians);
}

FVector2D USplineWidgetFunctionLibrary::GetLocationAtSplinePoint(const USplineWidget* InSplineWidget, int32 PointIndex, ESlateSplineCoordinateSpace CoordinateSpace)
{
	const FInterpCurvePointVector2D& Point = GetPositionPointSafe(InSplineWidget, PointIndex);
	const FVector2D& Location = Point.OutVal;

	if (CoordinateSpace == ESlateSplineCoordinateSpace::Screen)
	{
		return InSplineWidget->GetCachedGeometry().LocalToAbsolute(Location);
	}

	if (CoordinateSpace == ESlateSplineCoordinateSpace::Viewport)
	{
		FVector2D PixelPosition, ViewportPosition;
		USlateBlueprintLibrary::LocalToViewport(InSplineWidget->GetWorld(), InSplineWidget->GetCachedGeometry(), Location, PixelPosition, ViewportPosition);
		return ViewportPosition;
	}

    return Location;
}

FVector2D USplineWidgetFunctionLibrary::GetDirectionAtSplinePoint(const USplineWidget* InSplineWidget, int32 PointIndex, ESlateSplineCoordinateSpace CoordinateSpace)
{
	const FInterpCurvePointVector2D& Point = GetPositionPointSafe(InSplineWidget, PointIndex);
	const FVector2D Direction = Point.LeaveTangent.GetSafeNormal();

	if (CoordinateSpace == ESlateSplineCoordinateSpace::Screen || CoordinateSpace == ESlateSplineCoordinateSpace::Viewport)
	{
		return InSplineWidget->GetCachedGeometry().GetAccumulatedRenderTransform().TransformVector(Direction);
	}

	return Direction;
}

FVector2D USplineWidgetFunctionLibrary::GetTangentAtSplinePoint(const USplineWidget* InSplineWidget, int32 PointIndex, ESlateSplineCoordinateSpace CoordinateSpace)
{
	check(InSplineWidget);
	const FInterpCurvePointVector2D& Point = GetPositionPointSafe(InSplineWidget, PointIndex);
	const FVector2D& Direction = Point.LeaveTangent;

	if (CoordinateSpace == ESlateSplineCoordinateSpace::Screen || CoordinateSpace == ESlateSplineCoordinateSpace::Viewport)
	{
		return InSplineWidget->GetCachedGeometry().GetAccumulatedRenderTransform().TransformVector(Direction);
	}

	return Direction;
}

float USplineWidgetFunctionLibrary::GetInputKeyAtDistanceAlongSpline(const USplineWidget* InSplineWidget, float Distance, ESlateSplineCoordinateSpace CoordinateSpace)
{
	check(InSplineWidget);
	const int32 NumPoints = InSplineWidget->SplineCurves.Position.Points.Num();

	if (NumPoints < 2)
	{
		return 0.0f;
	}

	return InSplineWidget->SplineCurves.ReparamTable.Eval(Distance, 0.0f);
}

FVector2D USplineWidgetFunctionLibrary::GetLocationAtDistanceAlongSpline(const USplineWidget* InSplineWidget, float Distance, ESlateSplineCoordinateSpace CoordinateSpace)
{
	check(InSplineWidget);
	const float Param = InSplineWidget->SplineCurves.ReparamTable.Eval(Distance, 0.0f);
	return GetLocationAtSplineInputKey(InSplineWidget, Param, CoordinateSpace);
}

FVector2D USplineWidgetFunctionLibrary::GetDirectionAtDistanceAlongSpline(const USplineWidget* InSplineWidget, float Distance, ESlateSplineCoordinateSpace CoordinateSpace)
{
	check(InSplineWidget);
	const float Param = InSplineWidget->SplineCurves.ReparamTable.Eval(Distance, 0.0f);
	return GetDirectionAtSplineInputKey(InSplineWidget, Param, CoordinateSpace);
}

FVector2D USplineWidgetFunctionLibrary::GetTangentAtDistanceAlongSpline(const USplineWidget* InSplineWidget, float Distance, ESlateSplineCoordinateSpace CoordinateSpace)
{
	check(InSplineWidget);
	const float Param = InSplineWidget->SplineCurves.ReparamTable.Eval(Distance, 0.0f);
	return GetTangentAtSplineInputKey(InSplineWidget, Param, CoordinateSpace);
}

float USplineWidgetFunctionLibrary::GetRotationAngleAtDistanceAlongSpline(const USplineWidget* InSplineWidget, float Distance, ESlateSplineCoordinateSpace CoordinateSpace)
{
	check(InSplineWidget);
	const float Param = InSplineWidget->SplineCurves.ReparamTable.Eval(Distance, 0.0f);
    return GetRotationAngleAtSplineInputKey(InSplineWidget, Param, CoordinateSpace);
}

const FInterpCurvePointVector2D& USplineWidgetFunctionLibrary::GetPositionPointSafe(const USplineWidget* InSplineWidget, int32 PointIndex)
{
	check(InSplineWidget);
	const TArray<FInterpCurvePointVector2D>& Points = InSplineWidget->SplineCurves.Position.Points;
	const int32 NumPoints = Points.Num();
	if (NumPoints > 0)
	{
		const int32 ClampedIndex = (InSplineWidget->SplineData.bIsClosedLoop && PointIndex >= NumPoints) ? 0 : FMath::Clamp(PointIndex, 0, NumPoints - 1);
		return Points[ClampedIndex];
	}

	static FInterpCurvePointVector2D DefaultPoint(0.0f, FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D::ZeroVector, CIM_Constant);
	return DefaultPoint;
}
