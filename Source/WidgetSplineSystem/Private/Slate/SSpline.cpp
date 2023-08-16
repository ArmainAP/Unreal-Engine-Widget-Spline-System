// Copyright to Kat Code Labs, SRL. All Rights Reserved.

#include "Slate/SSpline.h"

#include "Data/SlatePaintContext.h"
#include "Slate/SplineBuilder.h"

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
	const FSlateSpline& SplineRef = Spline.Get();
	if (SplineRef.Points.Num() < 2)
	{
		return LayerId;
	}

	const FSlatePaintContext PaintContext(OutDrawElements, AllottedGeometry.ToPaintGeometry(), LayerId + 1,
		ShouldBeEnabled(bParentEnabled) ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect,
		SplineRef.Brush.TintColor.GetColor(InWidgetStyle).ToFColorSRGB());

	if (SplineRef.Brush.GetResourceObject()->IsValidLowLevel())
	{
		PaintSplineBrush(PaintContext);
	}
	else
	{
		PaintSplineSimple(PaintContext);
	}
	
	return PaintContext.LayerId;
}

void SSpline::PaintSplineSimple(const FSlatePaintContext& InPaintContext) const
{
	const FSlateSpline& SplineRef = Spline.Get();
	const auto& DrawSplineSegment = [&](const FSlateSplinePoint& SegmentStart, const FSlateSplinePoint& SegmentEnd){
		const FVector2D& SegmentStartDirection = SplineRef.bIsLinear ? FVector2D::ZeroVector : SegmentStart.Direction;
		const FVector2D& SegmentEndDirection = SplineRef.bIsLinear ? FVector2D::ZeroVector : SegmentEnd.Direction;
		FSlateDrawElement::MakeSpline(
				InPaintContext.OutDrawElements,
				InPaintContext.LayerId,
				InPaintContext.PaintGeometry,
				SegmentStart.Location,
				SegmentStartDirection,
				SegmentEnd.Location,
				SegmentEndDirection,
				SplineRef.Brush.GetImageSize().X,
				InPaintContext.DrawEffect,
				InPaintContext.TintColor);
	};
	
	for (int i = 0; i < SplineRef.Points.Num() - 1; i++)
	{
		DrawSplineSegment(SplineRef.Points[i], SplineRef.Points[i + 1]);
	}
	
	if (SplineRef.bIsClosedLoop)
	{
		DrawSplineSegment(SplineRef.Points.Last(), SplineRef.Points[0]);
	}
}

void SSpline::PaintSplineBrush(const FSlatePaintContext& InPaintContext) const
{
	const FSlateSpline& SplineRef = Spline.Get();
	FSplineBuilder SplineBuilder(SplineRef.Brush.GetImageSize(), InPaintContext);
	
	for (int i = 0; i < SplineRef.Points.Num() - 1; i++)
	{
		SplineBuilder.BuildBezierGeometry(SplineRef.Points[i], SplineRef.Points[i + 1], SplineRef.bIsLinear);
	}
	
	if (SplineRef.bIsClosedLoop)
	{
		SplineBuilder.BuildBezierGeometry(SplineRef.Points.Last(), SplineRef.Points[0], SplineRef.bIsLinear);
	}

	SplineBuilder.Finish(SplineRef.bIsClosedLoop);
	
	
	const FSlateResourceHandle& RenderResourceHandle = FSlateApplication::Get().GetRenderer()->GetResourceHandle(SplineRef.Brush);
	FSlateDrawElement::MakeCustomVerts(InPaintContext.OutDrawElements, InPaintContext.LayerId, RenderResourceHandle, SplineBuilder.GetVertexArray(), SplineBuilder.GetIndexArray(), nullptr, 0, 0, InPaintContext.DrawEffect);
}
