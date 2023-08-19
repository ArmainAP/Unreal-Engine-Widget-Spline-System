// Copyright to Kat Code Labs, SRL. All Rights Reserved.

#include "Data/SlateSplineCurves.h"

void FSlateSplineCurves::UpdateSpline(const FSlateSpline& InSplineRef, int32 InReparamStepsPerSegment,
	bool bLoopPositionOverride, float LoopPosition, const FVector2D& Scale2D)
{
	ReparamStepsPerSegment = InReparamStepsPerSegment;

	const EInterpCurveMode CurveMode = InSplineRef.bIsLinear ? CIM_Linear : CIM_CurveUser;
	Position.Points.Reset(InSplineRef.Points.Num());
	for(int i = 0; i < InSplineRef.Points.Num(); i++)
	{
		Position.Points.Emplace(i, InSplineRef.Points[i].Location, InSplineRef.Points[i].Direction, InSplineRef.Points[i].Direction, CurveMode);
	}

	if(InSplineRef.bIsClosedLoop)
	{
		if (bLoopPositionOverride)
		{
			Position.SetLoopKey(LoopPosition);	
		}
		else
		{
			const float LastKey = Position.Points.Num() > 0 ? Position.Points.Last().InVal : 0.0f;
			Position.SetLoopKey(LastKey + 1.0f);
		}
 	}
	else
	{
		Position.ClearLoopKey();
	}

	const int SegmentCount = InSplineRef.bIsClosedLoop ? Position.Points.Num() : Position.Points.Num() - 1;
	if (SegmentCount > -1)
	{
		ReparamTable.Points.Reset(SegmentCount * ReparamStepsPerSegment + 1);

		float AccumulatedLength = 0.0f;
		for (int SegmentIndex = 0; SegmentIndex < SegmentCount; SegmentIndex++)
		{
			for (int Step = 0; Step < ReparamStepsPerSegment; Step++)
			{
				const float Param = Step / ReparamStepsPerSegment;
				const float SegmentLength = Step == 0 ? 0.0f : GetSegmentLength(SegmentIndex, Param, InSplineRef.bIsClosedLoop, Scale2D);
				ReparamTable.Points.Emplace(SegmentLength + AccumulatedLength, SegmentIndex + Param, 0.0f, 0.0f, CIM_Linear);
			}
			AccumulatedLength += GetSegmentLength(SegmentIndex, 1.0f, InSplineRef.bIsClosedLoop, Scale2D);
		}

		ReparamTable.Points.Emplace(AccumulatedLength, SegmentCount, 0.0f, 0.0f, CIM_Linear);
		++Version;
	}
}

float FSlateSplineCurves::GetSegmentLength(const int32 Index, const float Param, bool bClosedLoop,
	const FVector2D& Scale2D) const
{
	const int32 NumPoints = Position.Points.Num();
	const int32 LastPoint = NumPoints - 1;

	check(Index >= 0 && ((bClosedLoop && Index < NumPoints) || (!bClosedLoop && Index < LastPoint)));
	check(Param >= 0.0f && Param <= 1.0f);

	// Evaluate the length of a Hermite spline segment.
	// This calculates the integral of |dP/dt| dt, where P(t) is the spline equation with components (x(t), y(t), z(t)).
	// This isn't solvable analytically, so we use a numerical method (Legendre-Gauss quadrature) which performs very well
	// with functions of this type, even with very few samples.  In this case, just 5 samples is sufficient to yield a
	// reasonable result.

	struct FLegendreGaussCoefficient
	{
		float Abscissa;
		float Weight;
	};

	static const FLegendreGaussCoefficient LegendreGaussCoefficients[] =
	{
		{ 0.0f, 0.5688889f },
		{ -0.5384693f, 0.47862867f },
		{ 0.5384693f, 0.47862867f },
		{ -0.90617985f, 0.23692688f },
		{ 0.90617985f, 0.23692688f }
	};

	const auto& StartPoint = Position.Points[Index];
	const auto& EndPoint = Position.Points[Index == LastPoint ? 0 : Index + 1];

	const auto& P0 = StartPoint.OutVal;
	const auto& T0 = StartPoint.LeaveTangent;
	const auto& P1 = EndPoint.OutVal;
	const auto& T1 = EndPoint.ArriveTangent;

	// Special cases for linear or constant segments
	if (StartPoint.InterpMode == CIM_Linear)
	{
		return ((P1 - P0) * Scale2D).Size() * Param;
	}
	else if (StartPoint.InterpMode == CIM_Constant)
	{
		return 0.0f;
	}

	// Cache the coefficients to be fed into the function to calculate the spline derivative at each sample point as they are constant.
	const FVector2D Coeff1 = ((P0 - P1) * 2.0f + T0 + T1) * 3.0f;
	const FVector2D Coeff2 = (P1 - P0) * 6.0f - T0 * 4.0f - T1 * 2.0f;
	const FVector2D Coeff3 = T0;

	const float HalfParam = Param * 0.5f;

	float Length = 0.0f;
	for (const auto& LegendreGaussCoefficient : LegendreGaussCoefficients)
	{
		// Calculate derivative at each Legendre-Gauss sample, and perform a weighted sum
		const float Alpha = HalfParam * (1.0f + LegendreGaussCoefficient.Abscissa);
		const FVector2D Derivative = ((Coeff1 * Alpha + Coeff2) * Alpha + Coeff3) * Scale2D;
		Length += Derivative.Size() * LegendreGaussCoefficient.Weight;
	}
	Length *= HalfParam;

	return Length;
}

float FSlateSplineCurves::GetSplineLength() const
{
	return (ReparamTable.Points.Num() > 0) ? ReparamTable.Points.Last().InVal : 0.0f; 
}
