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

int32 SSpline::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId,
	const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const int32 NextLayerId = LayerId + 1;
	const FSlateSpline& SplineRef = Spline.Get();
	if (SplineRef.Points.Num() > 1)
	{
		const ESlateDrawEffect DrawEffect = ShouldBeEnabled(bParentEnabled) ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;
		const FColor TintColor = SplineRef.Brush.TintColor.GetColor(InWidgetStyle).ToFColorSRGB();

		if (SplineRef.Brush.GetResourceObject()->IsValidLowLevel())
		{
			PaintSplineBrush(AllottedGeometry, OutDrawElements, LayerId, DrawEffect, TintColor);
		}
		else
		{
			PaintSpline(AllottedGeometry, OutDrawElements, NextLayerId, DrawEffect, TintColor);	
		}
	}
	return NextLayerId;
}

void SSpline::PaintSpline(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements,
                          int32 LayerId, ESlateDrawEffect DrawEffect, const FColor& TintColor) const
{
	const FSlateSpline& SplineRef = Spline.Get();
	for (int i = 0; i < SplineRef.Points.Num() - 1; i++)
	{
		FSlateSplinePoint SegmentStart = SplineRef.Points.Last();
		FSlateSplinePoint SegmentEnd = SplineRef.Points[0];
		if(SplineRef.bIsLinear)
		{
			SegmentStart.Direction = FVector2D::ZeroVector;
			SegmentEnd.Direction = FVector2D::ZeroVector;
		}
		
		if(ArePointsValid(SegmentStart.Location, SegmentEnd.Location))
		{
			FSlateDrawElement::MakeSpline(
				OutDrawElements,
				LayerId + 1,
				AllottedGeometry.ToPaintGeometry(),
				SegmentStart.Location,
				SegmentStart.Direction,
				SegmentEnd.Location,
				SegmentEnd.Direction,
				SplineRef.Brush.GetImageSize().X,
				DrawEffect,
				TintColor);	
		}
	}
	
	if (SplineRef.bIsClosedLoop)
	{
		DrawSplineSegment(SplineRef.Points.Last(), SplineRef.Points[0], OutDrawElements, LayerId, AllottedGeometry, DrawEffect, TintColor);
	}
}

void SSpline::DrawSplineSegment(const FSlateSplinePoint& SegmentStart, const FSlateSplinePoint& SegmentEnd, FSlateWindowElementList& OutDrawElements, uint32
                                LayerId, const FGeometry& AllottedGeometry, ESlateDrawEffect DrawEffects, const FLinearColor& TintColor) const
{
	const FSlateSpline& SplineRef = Spline.Get();
	if(ArePointsValid(SegmentStart.Location, SegmentEnd.Location))
	{
		FSlateDrawElement::MakeSpline(
			OutDrawElements,
			LayerId + 1,
			AllottedGeometry.ToPaintGeometry(),
			SegmentStart.Location,
			SplineRef.bIsLinear ? FVector2D::ZeroVector : SegmentStart.Direction,
			SegmentEnd.Location,
			SplineRef.bIsLinear ? FVector2D::ZeroVector : SegmentEnd.Direction,
			SplineRef.Brush.GetImageSize().X,
			DrawEffects,
			TintColor);	
	}
}

void SSpline::PaintSplineBrush(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements,
                               int32 LayerId, ESlateDrawEffect DrawEffect, const FColor& TintColor) const
{
}

bool SSpline::ArePointsValid(const FVector2D& A, const FVector2D& B)
{
	return A.X != -FLT_MAX && A.Y != -FLT_MAX && B.X != -FLT_MAX && B.Y != -FLT_MAX;
}