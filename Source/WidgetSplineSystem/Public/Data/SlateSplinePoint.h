// Copyright to Kat Code Labs, SRL. All Rights Reserved.

#pragma once

#include "SlateSplinePoint.generated.h"

USTRUCT(BlueprintType)
struct WIDGETSPLINESYSTEM_API FSlateSplinePoint
{
	GENERATED_BODY()

	FSlateSplinePoint() = default;
	FSlateSplinePoint(const FVector2D InLocation, FVector2D InDirection) : Location(InLocation), Direction(InDirection)
	{}

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D Location = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D Direction = FVector2D::ZeroVector;
};
