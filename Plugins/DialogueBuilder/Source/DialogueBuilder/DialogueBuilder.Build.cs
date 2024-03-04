// Copyright 1998-2022 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class DialogueBuilder : ModuleRules
{
	public DialogueBuilder(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] 
			{
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
                "CoreUObject"
            }
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
                "Core",
                "CoreUObject",
                "Engine"
            }
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
			}
			);
	}
}
