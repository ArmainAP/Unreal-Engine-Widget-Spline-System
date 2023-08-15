#pragma once

// Based on FSplineBuilder from Runtime/SlateCore/Private/Rendering/ElementBatcher.cpp

/** Utility class for building a strip of triangles for a spline. */
struct FSplineBuilder
{
	FSplineBuilder(float HalfThickness, const FSlatePaintContext& PaintContext)
		: RenderTransform(PaintContext.GetRenderTransform())
		, LastPointAdded()
		, LastNormal(FVector2f::ZeroVector)
		, HalfLineThickness(HalfThickness)
		, SingleColor(PaintContext.TintColor)
	{}

	void BuildBezierGeometry(const FSlateSplinePoint& SegmentStart, const FSlateSplinePoint& SegmentEnd, const bool bIsLinear)
	{
		const FVector2D& SegmentStartDirection = bIsLinear ? FVector2D::ZeroVector : SegmentStart.Direction;
		const FVector2D& SegmentEndDirection = bIsLinear ? FVector2D::ZeroVector : SegmentEnd.Direction;

		// This is the same value that Unreal inside FSlateSplinePayload::SetHermiteSpline
		constexpr  float BezierControlPointScale = 3.0f;
		const FVector2D P1 = SegmentStart.Location + SegmentStartDirection / BezierControlPointScale;
		const FVector2D P2 = SegmentEnd.Location + SegmentEndDirection / BezierControlPointScale;
		
		Subdivide(SegmentStart.Location, P1, P2, SegmentEnd.Location, 1.0f);
	}

	void Finish()
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
			const FVector2D LastUp = LastNormal*HalfLineThickness;

			Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(RenderTransform, FVector2f(LastPointAdded[0] + LastUp), FVector2f(1.0f, 0.0f), FVector2f::ZeroVector, SingleColor));
			Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(RenderTransform, FVector2f(LastPointAdded[0] - LastUp), FVector2f(-1.0f, 0.0f), FVector2f::ZeroVector, SingleColor));

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

	TArray<FSlateVertex>& GetVertexArray()
	{
		return Vertices;
	}

	TArray<SlateIndex>& GetIndexArray()
	{
		return Indices;
	}
	
private:
	void AppendPoint(const FVector2D NewPoint)
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

			Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(RenderTransform, FVector2f(LastPointAdded[1] + LastUp), FVector2f(1.0f, 0.0f), FVector2f::ZeroVector, SingleColor));
			Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(RenderTransform, FVector2f(LastPointAdded[1] - LastUp), FVector2f(-1.0f, 0.0f), FVector2f::ZeroVector, SingleColor));
		}

		if (NumPointsAdded >= 2)
		{
			const FVector2D AveragedUp = (0.5f*(NewNormal + LastNormal)).GetSafeNormal()*HalfLineThickness;

			Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(RenderTransform, FVector2f(LastPointAdded[0] + AveragedUp), FVector2f(1.0f, 0.0f), FVector2f::ZeroVector, SingleColor));
			Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(RenderTransform, FVector2f(LastPointAdded[0] - AveragedUp), FVector2f(-1.0f, 0.0f), FVector2f::ZeroVector, SingleColor));

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

	/**
	* Based on comp.graphics.algorithms: Adaptive Subdivision of Bezier Curves.
	*
	*   P1 + - - - - + P2
	*     /           \
	* P0 *             * P3
	*
	* In a perfectly flat curve P1 is the midpoint of (P0, P2) and P2 is the midpoint of (P1,P3).
	* Computing the deviation of points P1 and P2 from the midpoints of P0,P2 and P1,P3 provides
	* a simple and reliable measure of flatness.
	*
	* P1Deviation = (P0 + P2)/2 - P1
	* P2Deviation = (P1 + P3)/2 - P2
	*
	* Eliminate divides: same expression but gets us twice the allowable error
	* P1Deviation*2 = P0 + P2 - 2*P1
	* P2Deviation*2 = P1 + P3 - 2*P2
	*
	* Use manhattan distance: 2*Deviation = |P1Deviation.x| + |P1Deviation.y| + |P2Deviation.x| + |P2Deviation.y|
	*
	*/
	static float ComputeCurviness(const FVector2D P0, const FVector2D P1, const FVector2D P2, const FVector2D P3)
	{
		const FVector2D TwoP1Deviations = P0 + P2 - 2 * P1;
		const FVector2D TwoP2Deviations = P1 + P3 - 2 * P2;
		return FMath::Abs(TwoP1Deviations.X) + FMath::Abs(TwoP1Deviations.Y) + FMath::Abs(TwoP2Deviations.X) + FMath::Abs(TwoP2Deviations.Y);
	}


	/**
	* deCasteljau subdivision of Bezier Curves based on reading of Gernot Hoffmann's Bezier Curves.
	*
	*       P1 + - - - - + P2                P1 +
	*         /           \                    / \
	*     P0 *             * P3            P0 *   \   * P3
	*                                              \ /
	*                                               + P2
	*
	*
	* Split the curve defined by P0,P1,P2,P3 into two new curves L0..L3 and R0..R3 that define the same shape.
	*
	* Points L0 and R3 are P0 and P3.
	* First find points L1, M, R2  as the midpoints of (P0,P1), (P1,P2), (P2,P3).
	* Find two more points: L2, R1 defined by midpoints of (L1,M) and (M,R2) respectively.
	* The final points L3 and R0 are both the midpoint of (L2,R1)
	*
	*/
	static void deCasteljauSplit(const FVector2D P0, const FVector2D P1, const FVector2D P2, const FVector2D P3, FVector2D OutCurveParams[7])
	{
		FVector2D L1 = (P0 + P1) * 0.5f;
		FVector2D M = (P1 + P2) * 0.5f;
		FVector2D R2 = (P2 + P3) * 0.5f;

		FVector2D L2 = (L1 + M) * 0.5f;
		FVector2D R1 = (M + R2) * 0.5f;

		FVector2D L3R0 = (L2 + R1) * 0.5f;

		OutCurveParams[0] = P0;
		OutCurveParams[1] = L1;
		OutCurveParams[2] = L2;
		OutCurveParams[3] = L3R0;
		OutCurveParams[4] = R1;
		OutCurveParams[5] = R2;
		OutCurveParams[6] = P3;
	}

	void Subdivide(const FVector2D P0, const FVector2D P1, const FVector2D P2, const FVector2D P3, float MaxBiasTimesTwo = 2.0f)
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
	
private:
	const FSlateRenderTransform& RenderTransform;
	FVector2D LastPointAdded[2];
	FVector2D LastNormal;
	float HalfLineThickness;
	int32 NumPointsAdded = 0;
	FColor SingleColor;

	TArray<FSlateVertex> Vertices;
	TArray<SlateIndex> Indices;
};