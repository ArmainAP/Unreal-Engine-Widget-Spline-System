// Copyright to Kat Code Labs, SRL. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "Slate/SSpline.h"
#include "Data/SlateSplineCurves.h"
#include "SplineWidget.generated.h"

/**
 * 
 */
UCLASS()
class WIDGETSPLINESYSTEM_API USplineWidget : public UWidget
{
	GENERATED_BODY()

protected:
	virtual void OnWidgetRebuilt() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
	
#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override
	{
		return NSLOCTEXT("Spline", "Spline", "Spline");
	};
#endif

public:
	UFUNCTION(BlueprintCallable, Category = Spline)
	virtual void UpdateSpline();
	
	FSlateSpline GetSplineInfo() const { return SplineData; }
	
protected:
	TSharedPtr<SSpline> SlateSpline;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FSlateSpline SplineData = FSlateSpline();

	UPROPERTY(Transient)
	FSlateSplineCurves SplineCurves;
};
