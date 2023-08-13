// Copyright to Kat Code Labs, SRL. All Rights Reserved.

#pragma once

#include "Data/SlateSpline.h"

class WIDGETSPLINESYSTEM_API SSpline : public SLeafWidget
{
protected:
	TAttribute<FSlateSpline> Spline;

public:
	SLATE_BEGIN_ARGS(SSpline) : _Spline() {}
		SLATE_ATTRIBUTE(FSlateSpline, Spline);
	SLATE_END_ARGS()
	
	void Construct(const FArguments& InArguments);
	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

protected:
	virtual void PaintSpline(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId, ESlateDrawEffect DrawEffect, const FColor& TintColor) const;
	virtual void DrawSplineSegment(const FSlateSplinePoint& SegmentStart, const FSlateSplinePoint& SegmentEnd, FSlateWindowElementList& OutDrawElements, uint32
	                               LayerId, const FGeometry& AllottedGeometry, ESlateDrawEffect DrawEffects, const FLinearColor& TintColor) const;
	
	virtual void PaintSplineBrush(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId, ESlateDrawEffect
	                              DrawEffect, const FColor& TintColor) const;

	static bool ArePointsValid(const FVector2D& A, const FVector2D& B);
};
