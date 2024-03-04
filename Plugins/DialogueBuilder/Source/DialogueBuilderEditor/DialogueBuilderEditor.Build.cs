// Copyright 1998-2022 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class DialogueBuilderEditor : ModuleRules
{
	public DialogueBuilderEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
            }
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
            }
            );
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
                "Core",
                "Engine",
                "CoreUObject",
                "UnrealEd",
                "AIGraph",
                "Kismet",  // for FWorkflowCentricApplication
                "Slate",
                "SlateCore",
                "EditorStyle",
                "GraphEditor",
                "BlueprintGraph",
                "PropertyEditor",
                "ToolMenus",
                "InputCore",
				"ApplicationCore",
				"DialogueBuilder"
            }
            );
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
                "Core",
                "CoreUObject",
                "Engine",
            }
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
			}
			);
	}
}
