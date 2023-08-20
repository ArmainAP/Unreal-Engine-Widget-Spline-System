// Fill out your copyright notice in the Description page of Project Settings.

#include "WidgetSplineSystemEditor.h"
#include "Logging.h"

#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "FWidgetSplineSystemEditor"

void FWidgetSplineSystemEditor::StartupModule()
{
}

void FWidgetSplineSystemEditor::ShutdownModule()
{
	
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FWidgetSplineSystemEditor, WidgetSplineSystemEditor);