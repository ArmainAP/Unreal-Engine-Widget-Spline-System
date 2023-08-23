// Copyright to Kat Code Labs, SRL. All Rights Reserved.

#pragma once
#include "Data/SlatePaintContext.h"
#include "Data/SlateSpline.h"

DECLARE_DELEGATE_OneParam(FOnSplineDataChanged, const FSlateSpline&)

class WIDGETSPLINESYSTEMEDITOR_API SSplineWidgetEditPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSplineWidgetEditPanel)
	: _SplineData()
	, _OnSplineDataChanged()
		{ }
		SLATE_ATTRIBUTE(FSlateSpline, SplineData)
		SLATE_EVENT(FOnSplineDataChanged, OnSplineDataChanged)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	virtual void OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void OnMouseLeave(const FPointerEvent& MouseEvent) override;
	virtual void OnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent) override;

protected:
	virtual void PaintSplineSimple(const FSlatePaintContext& InPaintContext) const;
	virtual void PaintSplineBrush(const FSlatePaintContext& InPaintContext) const;
	virtual void PaintSplinePoints(const FSlatePaintContext& InPaintContext) const;
	virtual void PaintSplineTangent(const FSlatePaintContext& InPaintContext, FSlateSplinePoint SplinePoint) const;

	virtual void ComputeTangentPoints(const FSlateSplinePoint& InSplinePoint, FVector2D& OutArrive, FVector2D& OutLeave) const;
	
	TSharedRef<SWidget> CreateZoomToFitButton();
	bool ZoomToFit(bool bFitHorizontal, bool bFitVertical);
	static void AdjustRangeToFitPoints(double& InMin, double& InMax, int32 PointsCount);
	void SetZoomTransform(const FVector2D& InMin, const FVector2D& InMax, const FVector2D& LocalSize,
	                      bool bFitHorizontal,
	                      bool bFitVertical);

	static bool IsWithinBounds(const FVector2D& Position, const FVector2D& Point);
	int GetSplinePointUnderPosition(const FVector2D& LocalPosition) const;
	int GetSplineTangentUnderPosition(const FVector2D& LocalPosition, bool& bIsArrival) const;

	void CreateContextMenu(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent);

protected:
	static constexpr int32 INVALID_INDEX = -1;
	
	TAttribute<FSlateSpline> SplineData;
	FOnSplineDataChanged OnSplineDataChanged;

	struct FSplineEditPanelTransform
	{
		FVector2D Offset = FVector2D::ZeroVector;
		float Scale = 1.0;

		FVector2D LocalToInput(FVector2D Local) const
		{
			return (Scale != 0.0f) ? (Local / Scale + Offset) : Offset;
		}

		FVector2D InputToLocal(FVector2D Input) const
		{
			return (Input - Offset) * Scale;
		}
	} TransformInfo;

	int SelectedPointIndex = 0;
	bool bIsSelectedTangentArrival = false;
	bool bIsPanelFocused = false;

	enum class EDragState
	{
		None,
		PreDrag,
		DragKey,
		DragTangent,
		Pan,
	} DragState = EDragState::None;

	FVector2D LastMouseDownLocation;
	FVector2D PreDragPointLocation;
};
