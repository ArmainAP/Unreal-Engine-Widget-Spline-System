// Copyright to Kat Code Labs, SRL. All Rights Reserved.

#pragma once

// Based on FSplineBuilder from Runtime/SlateCore/Private/Rendering/ElementBatcher.cpp

#include "Data/SlatePaintContext.h"
#include "Data/SlateSplinePoint.h"

/** Utility class for building a strip of triangles for a spline. */
struct FSplineBuilder
{
	FSplineBuilder(const FVector2D& InSize, const FSlatePaintContext& PaintContext);

	void BuildBezierGeometry(const FSlateSplinePoint& SegmentStart, const FSlateSplinePoint& SegmentEnd, const bool bIsLinear);
	void Finish(bool bCloseLoop);

	TArray<FSlateVertex>& GetVertexArray()
	{
		return Vertices;
	}

	TArray<SlateIndex>& GetIndexArray()
	{
		return Indices;
	}
	
private:
	void AppendPoint(const FVector2D NewPoint);


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
	static float ComputeCurviness(const FVector2D P0, const FVector2D P1, const FVector2D P2, const FVector2D P3);



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
	static void deCasteljauSplit(const FVector2D P0, const FVector2D P1, const FVector2D P2, const FVector2D P3, FVector2D OutCurveParams[7]);

	void Subdivide(const FVector2D P0, const FVector2D P1, const FVector2D P2, const FVector2D P3, float MaxBiasTimesTwo = 2.0f);

	
private:
	const FSlateRenderTransform& RenderTransform;
	const FColor SingleColor;

	const FVector2f TextureCoord2;
	const float CoordScaleV;
	
	FVector2D LastPointAdded[2];
	FVector2D LastNormal;
	float HalfLineThickness;
	int32 NumPointsAdded = 0;
	float CurrentLength = 0.0f;
	float CurrentCoordV = 0.0f;

	TArray<FSlateVertex> Vertices;
	TArray<SlateIndex> Indices;
};