 // Copyright 2023 Leartes Studios. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class ObjectDistribution : ModuleRules
{
	public ObjectDistribution(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PrivatePCHHeaderFile = "Public/ObjectDistribution.h";
        
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public"));
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Private"));
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public/Data"));
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Private/Data"));
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public/Development"));
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Private/Development"));
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public/Editor"));
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Private/Editor"));
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public/ObjectDistribution"));
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Private/ObjectDistribution"));
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public/Slate"));
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Private/Slate"));
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public/UI"));
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Private/UI"));
        
        PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
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
				"CoreUObject",
				"Engine",
				"InputCore",
				"Blutility",
				"PropertyEditor"
			}
			);
		PrivateDependencyModuleNames.AddRange(
            new string[]
            {
	            "Slate",
	            "SlateCore",
	            "UnrealEd",
	            "EditorSubsystem",
	            "UMG",
	            "UMGEditor",
	            "EditorScriptingUtilities",
	            "EditorStyle",
	            "EditorWidgets" //Drop Target
            }
            );
        
        DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
