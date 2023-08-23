// Copyright to Kat Code Labs, SRL. All Rights Reserved.

#include "SplineWidgetDetailsCustomization.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "SplineWidget.h"
#include "SplineWidgetEditPanel.h"

#define LOCTEXT_NAMESPACE "SplineWidgetDetails"

TSharedRef<IDetailCustomization> FSplineWidgetDetailCustomization::MakeInstance()
{
	return MakeShareable(new FSplineWidgetDetailCustomization);
}

void FSplineWidgetDetailCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	const TArray<TWeakObjectPtr<UObject>>& SelectedObjects = DetailLayout.GetSelectedObjects();
	if (SelectedObjects.Num() != 1)
	{
		return;
	}
	
	USplineWidget* SplineWidget = nullptr;
	for (const TWeakObjectPtr<UObject> Object : SelectedObjects)
	{
		if (USplineWidget* TestSplineWidget = Cast<USplineWidget>(Object))
		{
			SplineWidget = TestSplineWidget;
			break;
		}
	}

	if (!SplineWidget)
	{
		return;
	}

	const TSharedPtr<IPropertyHandle> PropertySplineInfo = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(USplineWidget, SplineData), USplineWidget::StaticClass());
	check(PropertySplineInfo->IsValidHandle());

	// Make sure the EditSpline category is right below the Appearance category
	IDetailCategoryBuilder& EditSplineCategory = DetailLayout.EditCategory("SplineWidget", FText::GetEmpty(), ECategoryPriority::TypeSpecific);

	EditSplineCategory.AddCustomRow(LOCTEXT("SplineEditPanel", "Spline Edit Panel"))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBox)
				[
					SNew(SSplineWidgetEditPanel)
					.SplineData_UObject(SplineWidget, &USplineWidget::GetSplineData)
					.Clipping(EWidgetClipping::ClipToBounds)
					.OnSplineDataChanged(FOnSplineDataChanged::CreateLambda([=](const FSlateSpline& NewSplineInfo)
					{
						FString TextValue;
						FSlateSpline::StaticStruct()->ExportText(TextValue, &NewSplineInfo, &NewSplineInfo, nullptr, PPF_None, nullptr);
						const FPropertyAccess::Result Result = PropertySplineInfo->SetValueFromFormattedString(TextValue, EPropertyValueSetFlags::NotTransactable);
						check(Result == FPropertyAccess::Success);
					}))	
				]
			]
		];
}

#undef LOCTEXT_NAMESPACE