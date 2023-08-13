// Copyright to Kat Code Labs, SRL. All Rights Reserved.

#include "Slate/SSpline.h"

void SSpline::Construct(const FArguments& InArguments)
{
	Spline = InArguments._Spline;
}

FVector2D SSpline::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
	const FSlateSpline& SplineRef = Spline.Get();
	if (SplineRef.Points.Num() == 0)
	{
		return FVector2D::ZeroVector;
	}

	FBox2D BoundingBox(FVector2D::ZeroVector, FVector2D::UnitVector);
	for (const FSlateSplinePoint& SplinePoint : SplineRef.Points)
	{
		BoundingBox.Min.X = FMath::Min(BoundingBox.Min.X, SplinePoint.Location.X);
		BoundingBox.Min.Y = FMath::Min(BoundingBox.Min.Y, SplinePoint.Location.Y);
		BoundingBox.Max.X = FMath::Max(BoundingBox.Max.X, SplinePoint.Location.X);
		BoundingBox.Max.Y = FMath::Max(BoundingBox.Max.Y, SplinePoint.Location.Y);
	}

	return BoundingBox.GetSize();
}

