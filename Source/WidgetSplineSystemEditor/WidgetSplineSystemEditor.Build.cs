// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class WidgetSplineSystemEditor : ModuleRules
{
	public WidgetSplineSystemEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] 
			{ 
				"Core",
				"CoreUObject",
				"Engine", 
				"WidgetSplineSystem",
				"InputCore"
			});
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Slate", "SlateCore", "UnrealEd"
			});
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				
			});
	}
}
