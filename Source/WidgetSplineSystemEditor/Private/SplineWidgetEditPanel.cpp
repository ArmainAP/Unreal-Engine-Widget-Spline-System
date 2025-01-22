// Copyright to Kat Code Labs, SRL. All Rights Reserved.

#include "SplineWidgetEditPanel.h"

#include "ScopedTransaction.h"
#include "Slate/SplineBuilder.h"
#include "Styling/ToolBarStyle.h"

#define LOCTEXT_NAMESPACE "SSplineWidgetEditPanel"

static const FVector2D PointSize(10.0f);

static const FLinearColor PointColor[2] = { FLinearColor::Blue, FLinearColor::Yellow };

static constexpr float KeyTangentOffsetMin = 30.0f;
static constexpr float KeyTangentOffsetMax = 150.0f;
static constexpr float TangentMaxStrength = 2000.0f;

void SSplineWidgetEditPanel::Construct(const FArguments& InArgs)
{
	SplineData = InArgs._SplineData;
	OnSplineDataChanged = InArgs._OnSplineDataChanged;

	ChildSlot
	[
		CreateZoomToFitButton()
	];

	RegisterActiveTimer(0.f, FWidgetActiveTimerDelegate::CreateLambda([&](double InCurrentTime, float InDeltaTime)
	{
		return ZoomToFit(true, true) ? EActiveTimerReturnType::Stop : EActiveTimerReturnType::Continue;
	}));
}

int32 SSplineWidgetEditPanel::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId,
	const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	// TODO: Refactor duplicated code from SSpline
	const FSlateSpline& SplineRef = SplineData.Get();
	FSlatePaintContext PaintContext(OutDrawElements, AllottedGeometry, LayerId + 1,
		ShouldBeEnabled(bParentEnabled) ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect,
		SplineRef.Brush.TintColor.GetColor(InWidgetStyle).ToFColorSRGB());

	if (SplineRef.Points.Num() > 1)
	{
		if (SplineRef.Brush.GetResourceObject()->IsValidLowLevel())
		{
			PaintSplineBrush(PaintContext);
		}
		else
		{
			PaintSplineSimple(PaintContext);
		}	
	}

	PaintContext.LayerId++;
	PaintSplinePoints(PaintContext);
	
	PaintContext.LayerId++;
	return SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, PaintContext.LayerId, InWidgetStyle, bParentEnabled) + 1;
}

FReply SSplineWidgetEditPanel::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (bIsPanelFocused)
	{
		const float Delta = MouseEvent.GetWheelDelta();
		TransformInfo.Scale = FMath::Clamp(TransformInfo.Scale + Delta * 0.1f, 0.1f, 10.0f);
	
		const FVector2D LocalPosition = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
		const FVector2D InputPosition = TransformInfo.LocalToInput(LocalPosition);
		const FVector2D NewOffset = InputPosition - LocalPosition / TransformInfo.Scale;
		TransformInfo.Offset = NewOffset;
		
		return FReply::Handled();
	}
	
	return FReply::Unhandled();
}

FReply SSplineWidgetEditPanel::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	DragState = EDragState::PreDrag;
	bIsPanelFocused = MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton
					|| MouseEvent.GetEffectingButton() == EKeys::MiddleMouseButton
					|| MouseEvent.GetEffectingButton() == EKeys::RightMouseButton;
	
	if (bIsPanelFocused)
	{
		LastMouseDownLocation = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
		return FReply::Handled().CaptureMouse(SharedThis(this));
	}

	return FReply::Unhandled();
}

FReply SSplineWidgetEditPanel::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (!HasMouseCapture())
	{
		return FReply::Unhandled();
	}

	if (DragState != EDragState::PreDrag)
	{
		DragState = EDragState::None;
		return FReply::Handled().ReleaseMouseCapture();
	}
	
	const FVector2D LocalMousePosition = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	const int HitPointIndex = GetSplinePointUnderPosition(LocalMousePosition);

	auto HandleLeftClick = [&]()
	{
		if (HitPointIndex != INVALID_INDEX)
		{
			SelectedPointIndex = HitPointIndex;
		}
		else
		{
			const int HitTangent = GetSplineTangentUnderPosition(LocalMousePosition, bIsSelectedTangentArrival);
			if (HitTangent != INVALID_INDEX)
			{
				SelectedPointIndex = HitTangent;
			}
		}
	};

	auto HandleRightClick = [&]()
	{
		if (HitPointIndex != INVALID_INDEX)
		{
			SelectedPointIndex = HitPointIndex;
		}
		CreateContextMenu(MyGeometry, MouseEvent);
	};

	const bool bLeftMouseButton = MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton;
	const bool bRightMouseButton = MouseEvent.GetEffectingButton() == EKeys::RightMouseButton;

	if (bLeftMouseButton) HandleLeftClick();
	else if (bRightMouseButton) HandleRightClick();
	
	return FReply::Handled().ReleaseMouseCapture();
}

FReply SSplineWidgetEditPanel::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (!HasMouseCapture())
	{
		return FReply::Unhandled();
	}

	const FVector2D MousePosition = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	if (DragState == EDragState::PreDrag)
	{
		const bool bIsMouseButtonDown = MouseEvent.IsMouseButtonDown(EKeys::MiddleMouseButton)
											|| MouseEvent.IsMouseButtonDown(EKeys::RightMouseButton);
		
		DragState = bIsMouseButtonDown ? EDragState::Pan : EDragState::None;
		if (MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
		{
			if (const int HitPointIndex = GetSplinePointUnderPosition(LastMouseDownLocation);
				HitPointIndex != INVALID_INDEX)
			{
				DragState = EDragState::DragKey;
				SelectedPointIndex = HitPointIndex;
				PreDragPointLocation = MousePosition;
			}
			else if (const int HitTangent = GetSplineTangentUnderPosition(LastMouseDownLocation, bIsSelectedTangentArrival);
					HitTangent != INVALID_INDEX)
			{
				DragState = EDragState::DragTangent;
				SelectedPointIndex = HitTangent;
				PreDragPointLocation = SplineData.Get().Points[HitTangent].Direction;
			}
			else
			{
				DragState = EDragState::Pan;
			}
		}
	}
	
	if (DragState != EDragState::None)
	{
		FSlateSpline NewSplineData = SplineData.Get();
		if (DragState == EDragState::DragKey)
		{
			// Begin a new transaction for undo/redo purposes.
			const FScopedTransaction Transaction(LOCTEXT("MoveSplinePoint", "Moved spline point"));
			NewSplineData.Points[SelectedPointIndex].Location = TransformInfo.LocalToInput(MousePosition);
			OnSplineDataChanged.ExecuteIfBound(NewSplineData);
		}
		else if (DragState == EDragState::DragTangent)
		{
			// Begin a new transaction for undo/redo purposes.
			const FScopedTransaction Transaction(LOCTEXT("MoveTangentPoint", "Moved tangent point"));
			
			const FVector2D KeyLocation = SplineData.Get().Points[SelectedPointIndex].Location;
			const FVector2D KeyLocalLocation = TransformInfo.InputToLocal(KeyLocation);
		
			const float Distance = FMath::Max(KeyTangentOffsetMin, FVector2D::Distance(MousePosition, KeyLocalLocation));
			const float Alpha = FMath::Max(0.01f, (Distance - KeyTangentOffsetMin) / (KeyTangentOffsetMax - KeyTangentOffsetMin));
			const float Strength = TangentMaxStrength * Alpha / TransformInfo.Scale;

			FVector2D NewDirection = bIsSelectedTangentArrival ? (KeyLocalLocation - MousePosition) : (MousePosition - KeyLocalLocation);
			NewDirection.Normalize();
			NewDirection = NewDirection * Strength;
			
			NewSplineData.Points[SelectedPointIndex].Direction = NewDirection;
			OnSplineDataChanged.ExecuteIfBound(NewSplineData);

		}
		else if (DragState == EDragState::Pan)
		{
			const FVector2D ScreenDelta = MouseEvent.GetCursorDelta();
			const FVector2D InputDelta = ScreenDelta / TransformInfo.Scale;
			TransformInfo.Offset -= InputDelta;
		}
	}

	return FReply::Handled();
}

void SSplineWidgetEditPanel::OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	bIsPanelFocused = true;
}

void SSplineWidgetEditPanel::OnMouseLeave(const FPointerEvent& MouseEvent)
{
	if (DragState == EDragState::None)
	{
		bIsPanelFocused = false;
	}
}

void SSplineWidgetEditPanel::OnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent)
{
	DragState = EDragState::None;
}

void SSplineWidgetEditPanel::PaintSplineSimple(const FSlatePaintContext& InPaintContext) const
{
	const FSlateSpline& SplineRef = SplineData.Get();
	const auto& DrawSplineSegment = [&](const FSlateSplinePoint& SegmentStart, const FSlateSplinePoint& SegmentEnd){
		const FVector2D& SegmentStartDirection = SplineRef.bIsLinear ? FVector2D::ZeroVector : SegmentStart.Direction * TransformInfo.Scale;
		const FVector2D& SegmentEndDirection = SplineRef.bIsLinear ? FVector2D::ZeroVector : SegmentEnd.Direction * TransformInfo.Scale;
		FSlateDrawElement::MakeSpline(
				InPaintContext.OutDrawElements,
				InPaintContext.LayerId,
				InPaintContext.AllotedGeometry.ToPaintGeometry(),
				TransformInfo.InputToLocal(SegmentStart.Location),
				SegmentStartDirection,
				TransformInfo.InputToLocal(SegmentEnd.Location),
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

void SSplineWidgetEditPanel::PaintSplineBrush(const FSlatePaintContext& InPaintContext) const
{
	const FSlateSpline& SplineRef = SplineData.Get();
	FSplineBuilder SplineBuilder(SplineRef.Brush.GetImageSize(), InPaintContext);
	
	for (int i = 0; i < SplineRef.Points.Num() - 1; i++)
	{
		FSlateSplinePoint SegmentStart =  SplineRef.Points[i];
		SegmentStart.Location = TransformInfo.InputToLocal(SegmentStart.Location);
		SegmentStart.Direction = SegmentStart.Direction * TransformInfo.Scale;

		FSlateSplinePoint SegmentEnd =  SplineRef.Points[i+1];
		SegmentEnd.Location = TransformInfo.InputToLocal(SegmentEnd.Location);
		SegmentEnd.Direction = SegmentEnd.Direction * TransformInfo.Scale;
		
		SplineBuilder.BuildBezierGeometry(SegmentStart, SegmentEnd, SplineRef.bIsLinear);
	}
	
	if (SplineRef.bIsClosedLoop)
	{
		FSlateSplinePoint SegmentStart = SplineRef.Points.Last();
		SegmentStart.Location = TransformInfo.InputToLocal(SegmentStart.Location);
		SegmentStart.Direction = SegmentStart.Direction * TransformInfo.Scale;

		FSlateSplinePoint SegmentEnd = SplineRef.Points[0];
		SegmentEnd.Location = TransformInfo.InputToLocal(SegmentEnd.Location);
		SegmentEnd.Direction = SegmentEnd.Direction * TransformInfo.Scale;
		
		SplineBuilder.BuildBezierGeometry(SegmentStart, SegmentEnd, SplineRef.bIsLinear);
	}

	SplineBuilder.Finish(SplineRef.bIsClosedLoop);
	
	
	const FSlateResourceHandle& RenderResourceHandle = FSlateApplication::Get().GetRenderer()->GetResourceHandle(SplineRef.Brush);
	FSlateDrawElement::MakeCustomVerts(InPaintContext.OutDrawElements, InPaintContext.LayerId, RenderResourceHandle, SplineBuilder.GetVertexArray(), SplineBuilder.GetIndexArray(), nullptr, 0, 0, InPaintContext.DrawEffect);
}

void SSplineWidgetEditPanel::PaintSplinePoints(const FSlatePaintContext& InPaintContext) const
{
	static const FSlateBrush* KeyBrush = FAppStyle::GetBrush("CurveEd.CurveKey");
	
	const FSlateSpline& SplineRef = SplineData.Get();

	for (int32 i = 0; i < SplineRef.Points.Num(); ++i)
	{
		const FVector2D KeyLocation = TransformInfo.InputToLocal(SplineRef.Points[i].Location);
		const FVector2D KeyIconLocation = KeyLocation - PointSize / 2;

		const bool IsSelected = SelectedPointIndex == i;
		FSlateDrawElement::MakeBox(
			InPaintContext.OutDrawElements,
			InPaintContext.LayerId,
			InPaintContext.AllotedGeometry.ToPaintGeometry(KeyIconLocation, PointSize),
			KeyBrush,
			InPaintContext.DrawEffect,
			PointColor[IsSelected]
		);

		if (IsSelected)
		{
			PaintSplineTangent(InPaintContext, SplineRef.Points[i]);
		}
	}
}

void SSplineWidgetEditPanel::PaintSplineTangent(const FSlatePaintContext& InPaintContext, FSlateSplinePoint SplinePoint) const
{
	SplinePoint.Location = TransformInfo.InputToLocal(SplinePoint.Location);
	SplinePoint.Direction = SplinePoint.Direction * TransformInfo.Scale;
	FVector2D ArriveTangentLocation, LeaveTangentLocation;
	ComputeTangentPoints(SplinePoint, ArriveTangentLocation, LeaveTangentLocation);
	const FVector2D ArriveTangentIconLocation = ArriveTangentLocation - PointSize / 2;
	
	TArray<FVector2D> LinePoints;
	LinePoints.Add(SplinePoint.Location);
	LinePoints.Add(ArriveTangentLocation);
	FSlateDrawElement::MakeLines(
		InPaintContext.OutDrawElements,
		InPaintContext.LayerId,
		InPaintContext.PaintGeometry,
		LinePoints,
		InPaintContext.DrawEffect,
		PointColor[true]
	);

	LinePoints[1] = LeaveTangentLocation;
	FSlateDrawElement::MakeLines(
		InPaintContext.OutDrawElements,
		InPaintContext.LayerId,
		InPaintContext.PaintGeometry,
		LinePoints,
		InPaintContext.DrawEffect,
		PointColor[true]
	);

	const FSlateBrush* TangentBrushSelected = FAppStyle::GetBrush("CurveEd.TangentSelected");
	const FSlateBrush* ArrowImage = FAppStyle::GetBrush(TEXT("Graph.Arrow"));
	
	FSlateDrawElement::MakeBox(
		InPaintContext.OutDrawElements,
		InPaintContext.LayerId,
		InPaintContext.AllotedGeometry.ToPaintGeometry(ArriveTangentIconLocation, PointSize),
		TangentBrushSelected,
		InPaintContext.DrawEffect,
		PointColor[true]
	);

	const FVector2D TangentDirection = LeaveTangentLocation - ArriveTangentLocation;
	const float AngleInRadians = TangentDirection.IsNearlyZero() ? 0.0f : FMath::Atan2(TangentDirection.Y, TangentDirection.X);
	const float ZoomFactor = 1.3 * PointSize.X / ArrowImage->ImageSize.X;
	const FVector2D ArrowRadius = ArrowImage->ImageSize * ZoomFactor * 0.5f;
	const FVector2D ArrowDrawLocation = LeaveTangentLocation - ArrowRadius;

	FSlateDrawElement::MakeRotatedBox(
		InPaintContext.OutDrawElements,
		InPaintContext.LayerId,
		InPaintContext.AllotedGeometry.ToPaintGeometry(ArrowDrawLocation, ArrowImage->ImageSize * ZoomFactor),
		ArrowImage,
		InPaintContext.DrawEffect,
		AngleInRadians,
		TOptional<FVector2D>(),
		FSlateDrawElement::RelativeToElement,
		PointColor[true]
	);
}

void SSplineWidgetEditPanel::ComputeTangentPoints(const FSlateSplinePoint& InSplinePoint, FVector2D& OutArrive,
	FVector2D& OutLeave) const
{
	FVector2D ArriveDirectionNormalized = InSplinePoint.Direction;
	ArriveDirectionNormalized.Normalize();

	FVector2D LeaveDirectionNormalized = InSplinePoint.Direction;
	LeaveDirectionNormalized.Normalize();

	float Alpha = FVector2D::Distance(InSplinePoint.Direction, FVector2D::ZeroVector) / TangentMaxStrength;
	Alpha = FMath::Clamp(Alpha, 0.01f, 1.0f);

	const float KeyTangentOffset = FMath::Lerp(KeyTangentOffsetMin, KeyTangentOffsetMax, Alpha);
	OutArrive = -ArriveDirectionNormalized * KeyTangentOffset + InSplinePoint.Location;
	OutLeave = LeaveDirectionNormalized * KeyTangentOffset + InSplinePoint.Location;
}

TSharedRef<SWidget> SSplineWidgetEditPanel::CreateZoomToFitButton()
{
	const FToolBarStyle& ToolBarStyle = FAppStyle::Get().GetWidgetStyle<FToolBarStyle>("EditorViewportToolBar");
	return SNew(SBorder)
		.VAlign(VAlign_Top)
		.HAlign(HAlign_Left)
		.BorderImage(FAppStyle::GetBrush("NoBorder"))
		.DesiredSizeScale(FVector2D(256.0f, 32.0f))
		.Padding(FMargin(2, 2, 0, 0))
		[
			SNew(SButton)
			.ButtonStyle(&ToolBarStyle.ButtonStyle)
			.ToolTipText(LOCTEXT("ZoomToFit", "Zoom To Fit"))
			.OnClicked_Lambda([&]()->FReply
			{
				ZoomToFit(true, true);
				return FReply::Handled();
			})
			.ContentPadding(ToolBarStyle.ButtonPadding)
			.ContentPadding(1)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("ZoomToFit", "Zoom To Fit"))
				.ColorAndOpacity(FSlateColor::UseForeground())
			]
		];
}

bool SSplineWidgetEditPanel::ZoomToFit(bool bFitHorizontal, bool bFitVertical)
{
    FVector2D InMin(FLT_MAX, FLT_MAX);
    FVector2D InMax(-FLT_MAX, -FLT_MAX);
    const TArray<FSlateSplinePoint>& Points = SplineData.Get().Points;

    // Compute bounds of spline points.
    for (const FSlateSplinePoint& Point : Points)
    {
        InMin.X = FMath::Min(Point.Location.X, InMin.X);
        InMax.X = FMath::Max(Point.Location.X, InMax.X);
        InMin.Y = FMath::Min(Point.Location.Y, InMin.Y);
        InMax.Y = FMath::Max(Point.Location.Y, InMax.Y);
    }

    AdjustRangeToFitPoints(InMin.X, InMax.X, Points.Num());
    AdjustRangeToFitPoints(InMin.Y, InMax.Y, Points.Num());

    const FVector2D LocalSize = GetCachedGeometry().GetLocalSize();
    if (LocalSize == FVector2D::ZeroVector)
    {
        return false;
    }

    SetZoomTransform(InMin, InMax, LocalSize, bFitHorizontal, bFitVertical);

    return true;
}

void SSplineWidgetEditPanel::AdjustRangeToFitPoints(double& InMin, double& InMax, int32 PointsCount)
{
	constexpr float	FitMargin = 0.25f;

	if (PointsCount > 0)
    {
		constexpr float	MinViewRange = 100.0f;
        float Size = InMax - InMin;
        if (Size < MinViewRange)
        {
            InMin -= MinViewRange * 0.5f;
            InMax += MinViewRange * 0.5f;
            Size = InMax - InMin;
        }

        InMin -= FitMargin * Size;
        InMax += FitMargin * Size;
    }
    else
    {
	    constexpr float DefaultZoomRange = 200.0f;
	    InMin = -FitMargin * 2.0f;
        InMax = (DefaultZoomRange + FitMargin) * 2.0;
    }
}

void SSplineWidgetEditPanel::SetZoomTransform(const FVector2D& InMin, const FVector2D& InMax, const FVector2D& LocalSize, bool bFitHorizontal, bool bFitVertical)
{
    if (bFitHorizontal || bFitVertical)
    {
        FVector2D Scale(
            bFitHorizontal ? LocalSize.X / (InMax.X - InMin.X) : TransformInfo.Scale,
            bFitVertical ? LocalSize.Y / (InMax.Y - InMin.Y) : TransformInfo.Scale
        );

        if (bFitHorizontal && bFitVertical)
        {
            Scale.X = Scale.Y = FMath::Min(Scale.X, Scale.Y);
        }

        const FVector2D LocalCenter = LocalSize / 2;
        const FVector2D InputCenter = (InMax + InMin) / 2;
        const FVector2D Offset = InputCenter - LocalCenter / Scale;

        TransformInfo.Scale = bFitHorizontal ? Scale.X : Scale.Y;
        TransformInfo.Offset = Offset;
    }
}

bool SSplineWidgetEditPanel::IsWithinBounds(const FVector2D& Position, const FVector2D& Point)
{
	return Position.X > Point.X - 0.5f * PointSize.X &&
		Position.X < Point.X + 0.5f * PointSize.X &&
		Position.Y > Point.Y - 0.5f * PointSize.Y &&
		Position.Y < Point.Y + 0.5f * PointSize.Y;
}

int SSplineWidgetEditPanel::GetSplinePointUnderPosition(const FVector2D& LocalPosition) const
{
	const FSlateSpline& SplineRef = SplineData.Get();

	for (int32 i = 0; i < SplineRef.Points.Num(); ++i)
	{
		const FVector2D PointLocalLocation = TransformInfo.InputToLocal(SplineRef.Points[i].Location);
		if (IsWithinBounds(LocalPosition, PointLocalLocation))
		{
			return i;
		}
	}

	return INVALID_INDEX;
}

int SSplineWidgetEditPanel::GetSplineTangentUnderPosition(const FVector2D& LocalPosition, bool& bIsArrival) const
{
	int TangentIndex  = INVALID_INDEX;

	const FSlateSpline& SplineRef = SplineData.Get();
	for (int32 i = 0; i < SplineRef.Points.Num(); ++i)
	{
		FSlateSplinePoint SplinePoint = SplineRef.Points[i];
		SplinePoint.Location = TransformInfo.InputToLocal(SplinePoint.Location);
		SplinePoint.Direction = SplinePoint.Direction * TransformInfo.Scale;

		FVector2D ArriveTangent, LeaveTangent;
		ComputeTangentPoints(SplinePoint, ArriveTangent, LeaveTangent);

		if (IsWithinBounds(LocalPosition, ArriveTangent))
		{
			TangentIndex  = i;
			bIsArrival = true;
			break;
		}
		
		if (IsWithinBounds(LocalPosition, LeaveTangent))
		{
			TangentIndex  = i;
			bIsArrival = false;
			break;
		}
	}
	
	return TangentIndex;
}

void SSplineWidgetEditPanel::CreateContextMenu(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent)
{
	FMenuBuilder MenuBuilder(true, NULL);
	
	MenuBuilder.BeginSection("Actions", LOCTEXT("Actions", "Actions"));
	
	FUIAction AddPointAction = FUIAction(FExecuteAction::CreateLambda([=, this]()
	{
		const FScopedTransaction Transaction(LOCTEXT("AddNewSplinePoint", "Add New Spline Point"));

		const FVector2D ScreenMousePosition = InMouseEvent.GetScreenSpacePosition();
		const FVector2D LocalMousePosition = InMyGeometry.AbsoluteToLocal(ScreenMousePosition);
		const FSlateSplinePoint NewPoint(TransformInfo.LocalToInput(LocalMousePosition), FVector2D(1.0f, 0.0f));

		FSlateSpline NewSplineInfo = SplineData.Get();
		SelectedPointIndex = NewSplineInfo.Points.Add(NewPoint);

		OnSplineDataChanged.ExecuteIfBound(NewSplineInfo);
	}));
			
	MenuBuilder.AddMenuEntry(LOCTEXT("AddPoint", "Add point"),
		LOCTEXT("AddPointToolTip", "Add a new spline point at clicked mouse position"),
		FSlateIcon(), AddPointAction);
		
	if (SelectedPointIndex != -1)
	{
		FUIAction Action = FUIAction(FExecuteAction::CreateLambda([&]()
		{
			const FScopedTransaction Transaction(LOCTEXT("DeleteSplinePoint", "Delete Spline Point"));

			FSlateSpline SplineRef = SplineData.Get(); 
			if (SelectedPointIndex != -1 && SplineRef.Points.Num() > SelectedPointIndex)
			{
				SplineRef.Points.RemoveAt(SelectedPointIndex);
				SelectedPointIndex = SplineRef.Points.Num() - 1;
				OnSplineDataChanged.ExecuteIfBound(SplineRef);
			}
		}));
		
		MenuBuilder.AddMenuEntry(LOCTEXT("DeletePoint", "Delete Point"),
			LOCTEXT("DeletePointToolTip", "Delete the selected point"),
			FSlateIcon(), Action);
	}
	MenuBuilder.EndSection();

	FWidgetPath WidgetPath = InMouseEvent.GetEventPath() != nullptr ? *InMouseEvent.GetEventPath() : FWidgetPath();
	FSlateApplication::Get().PushMenu(SharedThis(this), WidgetPath, MenuBuilder.MakeWidget(), FSlateApplication::Get().GetCursorPos(), FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu));
}

#undef LOCTEXT_NAMESPACE
