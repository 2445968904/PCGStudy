//$ Copyright 2015-23, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

namespace UnrealBuildTool.Rules
{
    public class DungeonArchitectEditor : ModuleRules
    {
        public DungeonArchitectEditor(ReadOnlyTargetRules Target) : base(Target)
        {
            bLegacyPublicIncludePaths = false;
            PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
            ShadowVariableWarningLevel = WarningLevel.Error;
            IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_2;
            
            PublicIncludePaths.AddRange(new string[] {
                    // ... add public include paths required here ...
            });

            PrivateIncludePaths.AddRange(new[] {
                "DungeonArchitectEditor/Private"
            });

            PrivateIncludePathModuleNames.AddRange(new[] {
                "Settings",
                "AssetTools",
                "MessageLog"
            });

            PublicDependencyModuleNames.AddRange(new string[] {
                
            });

            PrivateDependencyModuleNames.AddRange(new[] {
                "DungeonArchitectRuntime",
                
                "AddContentDialog",
                "AdvancedPreviewScene",
                "ApplicationCore",
                "AssetRegistry",
                "ContentBrowser",
                "Core",
                "CoreUObject",
                "EditorFramework",
                "EditorInteractiveToolsFramework",
                "EditorScriptingUtilities",
                "EditorStyle",
                "EditorWidgets",
                "Engine",
                "Foliage",
                "GeometryFramework",
                "GeometryScriptingCore",
                "GeometryScriptingEditor",
                "GraphEditor",
                "InputCore",
                "InteractiveToolsFramework",
                "Json",
                "JsonUtilities",
                "Kismet",
                "KismetWidgets",
                "LevelEditor",
                "MainFrame",
                "PlacementMode",
                "Projects",
                "PropertyEditor",
                "RenderCore",
                "RHI",
                "Slate",
                "SlateCore",
                "ToolMenus",
                "UnrealEd",
                "WorkspaceMenuStructure",
            });

            DynamicallyLoadedModuleNames.AddRange(new string[] {
                // ... add any modules that your module loads dynamically here ...
            });
        }
    }
}
