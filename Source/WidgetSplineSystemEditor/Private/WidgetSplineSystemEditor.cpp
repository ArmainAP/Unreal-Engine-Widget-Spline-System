// Copyright to Kat Code Labs, SRL. All Rights Reserved.

#include "WidgetSplineSystemEditor.h"
#include "Logging.h"
#include "SplineWidget.h"
#include "SplineWidgetDetailsCustomization.h"

#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "FWidgetSplineSystemEditor"

void FWidgetSplineSystemEditor::StartupModule()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomClassLayout(USplineWidget::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&FSplineWidgetDetailCustomization::MakeInstance));
}

void FWidgetSplineSystemEditor::ShutdownModule()
{
	if (!UObjectInitialized())
	{
		return;
	}

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.UnregisterCustomClassLayout(USplineWidget::StaticClass()->GetFName());
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FWidgetSplineSystemEditor, WidgetSplineSystemEditor);