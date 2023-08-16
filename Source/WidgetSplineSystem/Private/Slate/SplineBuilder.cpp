// Copyright to Kat Code Labs, SRL. All Rights Reserved.

#include "Slate/SplineBuilder.h"

FSplineBuilder::FSplineBuilder(const FVector2D& InSize, const FSlatePaintContext& PaintContext)
	: RenderTransform(PaintContext.GetRenderTransform())
	, SingleColor(PaintContext.TintColor)
	, TextureCoord2(InSize.X, 1.0f)
	, CoordScaleV(InSize.Y * 0.01f)
{
	const float LineThickness = 2 * UE_SQRT_2 + TextureCoord2.X;
	HalfLineThickness = LineThickness / 2 + TextureCoord2.Y;
}

void FSplineBuilder::BuildBezierGeometry(FSlateSplinePoint SegmentStart, FSlateSplinePoint SegmentEnd, const bool bIsLinear)
{
	if (bIsLinear)
	{
		SegmentStart.Direction = FVector2D::ZeroVector;
		SegmentEnd.Direction = FVector2D::ZeroVector;
	}

	// This is the same value that Unreal inside FSlateSplinePayload::SetHermiteSpline
	constexpr float BezierControlPointScale = 3.0f;
	const FVector2D P1 = SegmentStart.Location + SegmentStart.Direction / BezierControlPointScale;
	const FVector2D P2 = SegmentEnd.Location - SegmentEnd.Direction / BezierControlPointScale;
		
	Subdivide(SegmentStart.Location, P1, P2, SegmentEnd.Location, 1.0f);
}

void FSplineBuilder::Finish(const bool bCloseLoop)
{
	if (NumPointsAdded < 3)
	{
		// Line builder needs at least two line segments (3 points) to
		// complete building its geometry.
		// This will only happen in the case when we have a straight line.
		AppendPoint(LastPointAdded[0]);
	}
	else
	{
		// We have added the last point, but the line builder only builds
		// geometry for the previous line segment. Build geometry for the
		// last line segment.
		const FVector2D LastUp = LastNormal * HalfLineThickness;

		CurrentLength += FVector2D::Distance(LastPointAdded[1], LastPointAdded[0]);
		CurrentCoordV = CurrentLength * CoordScaleV;

		if (bCloseLoop)
		{
			const FSlateRenderTransform& TempRenderTransform = FSlateRenderTransform(1.0f);
			Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(TempRenderTransform, Vertices[0].Position, FVector2f(1.0f, CurrentCoordV), TextureCoord2, SingleColor));
			Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(TempRenderTransform, Vertices[1].Position, FVector2f(0.0f, CurrentCoordV), TextureCoord2, SingleColor));
		}
		else
		{
			Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(RenderTransform, FVector2f(LastPointAdded[0] + LastUp), FVector2f(1.0f, CurrentCoordV), TextureCoord2, SingleColor));
			Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(RenderTransform, FVector2f(LastPointAdded[0] - LastUp), FVector2f(0.0f, CurrentCoordV), TextureCoord2, SingleColor));	
		}

		const int32 NumVerts = Vertices.Num();

		// Counterclockwise winding on triangles
		Indices.Add(NumVerts - 3);
		Indices.Add(NumVerts - 4);
		Indices.Add(NumVerts - 2);

		Indices.Add(NumVerts - 3);
		Indices.Add(NumVerts - 2);
		Indices.Add(NumVerts - 1);
	}
}

void FSplineBuilder::AppendPoint(const FVector2D NewPoint)
	{
		if (NumPointsAdded == 0)
		{
			LastPointAdded[0] = LastPointAdded[1] = NewPoint;
			NumPointsAdded++;
			return;
		}
		
		// We only add vertexes for the previous line segment.
		// This is because we want to average the previous and new normals
		// In order to prevent overlapping line segments on the spline.
		// These occur especially when curvature is high.

		const FVector2D NewNormal = FVector2D(LastPointAdded[0].Y - NewPoint.Y, NewPoint.X - LastPointAdded[0].X).GetSafeNormal();

		if (NumPointsAdded == 2)
		{
			// Once we have two points, we have a normal, so we can generate the first bit of geometry.
			const FVector2D LastUp = LastNormal*HalfLineThickness;

			Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(RenderTransform, FVector2f(LastPointAdded[1] + LastUp), FVector2f(1.0f, CurrentCoordV), TextureCoord2, SingleColor));
			Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(RenderTransform, FVector2f(LastPointAdded[1] - LastUp), FVector2f(0.0f, CurrentCoordV), TextureCoord2, SingleColor));
		}

		if (NumPointsAdded >= 2)
		{
			const FVector2D AveragedUp = (0.5f*(NewNormal + LastNormal)).GetSafeNormal()*HalfLineThickness;

			CurrentLength += FVector2D::Distance(LastPointAdded[1], LastPointAdded[0]);
			CurrentCoordV = CurrentLength * CoordScaleV;

			Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(RenderTransform, FVector2f(LastPointAdded[0] + AveragedUp), FVector2f(1.0f, CurrentCoordV), TextureCoord2, SingleColor));
			Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(RenderTransform, FVector2f(LastPointAdded[0] - AveragedUp), FVector2f(0.0f, CurrentCoordV), TextureCoord2, SingleColor));

			const int32 NumVerts = Vertices.Num();

			// Counterclockwise winding on triangles
			Indices.Add(NumVerts - 3);
			Indices.Add(NumVerts - 4);
			Indices.Add(NumVerts - 2);

			Indices.Add(NumVerts - 3);
			Indices.Add(NumVerts - 2);
			Indices.Add(NumVerts - 1);
		}

		LastPointAdded[1] = LastPointAdded[0];
		LastPointAdded[0] = NewPoint;
		LastNormal = NewNormal;

		++NumPointsAdded;
	}

float FSplineBuilder::ComputeCurviness(const FVector2D P0, const FVector2D P1, const FVector2D P2, const FVector2D P3)
{
	const FVector2D TwoP1Deviations = P0 + P2 - 2 * P1;
	const FVector2D TwoP2Deviations = P1 + P3 - 2 * P2;
	return FMath::Abs(TwoP1Deviations.X) + FMath::Abs(TwoP1Deviations.Y) + FMath::Abs(TwoP2Deviations.X) + FMath::Abs(TwoP2Deviations.Y);
}

void FSplineBuilder::deCasteljauSplit(const FVector2D P0, const FVector2D P1, const FVector2D P2, const FVector2D P3,
	FVector2D OutCurveParams[7])
{
	const FVector2D L1 = (P0 + P1) * 0.5f;
	const FVector2D M = (P1 + P2) * 0.5f;
	const FVector2D R2 = (P2 + P3) * 0.5f;

	const FVector2D L2 = (L1 + M) * 0.5f;
	const FVector2D R1 = (M + R2) * 0.5f;

	const FVector2D L3R0 = (L2 + R1) * 0.5f;

	OutCurveParams[0] = P0;
	OutCurveParams[1] = L1;
	OutCurveParams[2] = L2;
	OutCurveParams[3] = L3R0;
	OutCurveParams[4] = R1;
	OutCurveParams[5] = R2;
	OutCurveParams[6] = P3;
}

void FSplineBuilder::Subdivide(const FVector2D P0, const FVector2D P1, const FVector2D P2, const FVector2D P3,
	float MaxBiasTimesTwo)
{
	const float Curviness = ComputeCurviness(P0, P1, P2, P3);
	if (Curviness > MaxBiasTimesTwo)
	{
		// Split the Bezier into two curves.
		FVector2D TwoCurves[7];
		deCasteljauSplit(P0, P1, P2, P3, TwoCurves);
		// Subdivide left, then right
		Subdivide(TwoCurves[0], TwoCurves[1], TwoCurves[2], TwoCurves[3], MaxBiasTimesTwo);
		Subdivide(TwoCurves[3], TwoCurves[4], TwoCurves[5], TwoCurves[6], MaxBiasTimesTwo);
	}
	else
	{
		AppendPoint(P3);
	}
}
