// Copyright to Kat Code Labs, SRL. All Rights Reserved.

#pragma once
#include "Data/SlateSpline.h"

// Based on FSplineCurves from SplineComponent.h

USTRUCT(BlueprintType)
struct WIDGETSPLINESYSTEM_API FSlateSplineCurves
{
	GENERATED_BODY()

	/** Spline built from position data. */
	UPROPERTY()
	FInterpCurveVector2D Position;

	/** Input: distance along curve, output: parameter that puts you there. */
	UPROPERTY()
	FInterpCurveFloat ReparamTable;

	UPROPERTY()
	int32 ReparamStepsPerSegment = 0;

	UPROPERTY(transient)
	uint32 Version = 0xffffffff;

	bool operator==(const FSlateSplineCurves& Other) const
	{
		return Position == Other.Position;
	}

	bool operator!=(const FSlateSplineCurves& Other) const
	{
		return !(*this == Other);
	}

	/** 
	 * Update the spline's internal data according to the passed-in params 
	 * @param	InSplineRef				The spline data
	 * @param	bClosedLoop				Whether the spline is to be considered as a closed loop.
	 * @param	bStationaryEndpoints	Whether the endpoints of the spline are considered stationary when traversing the spline at non-constant velocity.  Essentially this sets the endpoints' tangents to zero vectors.
	 * @param	InReparamStepsPerSegment	Number of steps per spline segment to place in the reparameterization table
	 * @param	bLoopPositionOverride	Whether to override the loop position with LoopPosition
	 * @param	LoopPosition			The loop position to use instead of the last key
	 * @param	Scale2D					The world scale to override
	 */
	void UpdateSpline(const FSlateSpline& InSplineRef, int32 InReparamStepsPerSegment = 10, bool bLoopPositionOverride = false, float
	                  LoopPosition = 0.0f, const FVector2D& Scale2D = FVector2D(1.0f));

	/** Returns the length of the specified spline segment up to the parametric value given */
	float GetSegmentLength(const int32 Index, const float Param, bool bClosedLoop = false, const FVector2D& Scale2D = FVector2D(1.0f)) const;

	/** Returns total length along this spline */
	float GetSplineLength() const;
};