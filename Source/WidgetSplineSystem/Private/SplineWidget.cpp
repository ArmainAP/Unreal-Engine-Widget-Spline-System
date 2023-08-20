// Copyright to Kat Code Labs, SRL. All Rights Reserved.

#include "SplineWidget.h"

void USplineWidget::OnWidgetRebuilt()
{
	UpdateSpline();
}

TSharedRef<SWidget> USplineWidget::RebuildWidget()
{
	SlateSpline = SNew(SSpline).Spline_UObject(this, &USplineWidget::GetSplineData);
	return SlateSpline.ToSharedRef();
}

void USplineWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	SlateSpline.Reset();
}

void USplineWidget::UpdateSpline()
{
	SplineCurves.UpdateSpline(SplineData);
}
