// Copyright 2022 Greg Shynkar. All Rights Reserved

#include "DialogueBuilderEditorModule.h"

#define LOCTEXT_NAMESPACE "DialogueBuilderCustomEditor"

void FDialogueBuilderEditorModule::StartupModule()
{	
	//Registered AssetCategory;
	IAssetTools& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	EAssetTypeCategories::Type DialogueBuilderCategoryType = AssetToolsModule.RegisterAdvancedAssetCategory(TEXT("DialogueBuilder"), LOCTEXT("DialogueBuilderCategory", "DialogueBuilder"));
	TSharedRef<IAssetTypeActions> DialogueBuilderActions = MakeShareable(new FDialogueBuilderActions(DialogueBuilderCategoryType));
	AssetToolsModule.RegisterAssetTypeActions(DialogueBuilderActions);
	
	
	//Registered Visual Parts: Dialogue Nodes, Pin and PinConnectionFactory;
	FEdGraphUtilities::RegisterVisualNodeFactory(MakeShareable(new FDialogueBuilderGraphNodeFactory()));
	FEdGraphUtilities::RegisterVisualPinConnectionFactory(MakeShareable(new FDialogueBuilderGraphPinConnectionFactory()));
}

void FDialogueBuilderEditorModule::ShutdownModule()
{
	// Unregister properties when the module is shutdown;
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.NotifyCustomizationModuleChanged();
	}

	//Unregister Visual Parts: Dialogue Nodes, Pin and PinConnectionFactory;
	FEdGraphUtilities::UnregisterVisualNodeFactory(MakeShareable(new FDialogueBuilderGraphNodeFactory()));
	FEdGraphUtilities::UnregisterVisualPinConnectionFactory(MakeShareable(new FDialogueBuilderGraphPinConnectionFactory()));
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FDialogueBuilderEditorModule, DialogueBuilderEditor)