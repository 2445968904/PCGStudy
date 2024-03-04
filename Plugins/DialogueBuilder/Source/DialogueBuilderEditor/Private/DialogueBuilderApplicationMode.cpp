// Copyright 2022 Greg Shynkar. All Rights Reserved

#include "DialogueBuilderApplicationMode.h"

FDialogueBuilderApplicationMode::FDialogueBuilderApplicationMode(TSharedPtr<class FDialogueBuilderCustomEditor> InDialogueBuilderEditor)
	:FBlueprintEditorApplicationMode(InDialogueBuilderEditor, FDialogueBuilderCustomEditorModes::DialogueBuilderEditorMode1, FDialogueBuilderCustomEditorModes::GetLocalizedMode, false, false)
{
	DialogueBuilderEditor = InDialogueBuilderEditor;

	// Create the tab factories
	StardardTabFactories.RegisterFactory(MakeShareable(new FDialogueBuilderGraphTabFactory(InDialogueBuilderEditor)));

	//Create a default layout
	TabLayout =
		FTabManager::NewLayout("Standalone_BlueprintEditor_Layout_v17")
		->AddArea
		(
			FTabManager::NewPrimaryArea()->SetOrientation(Orient_Vertical)
			/*
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.186721f)
				->SetHideTabWell(true)
				->AddTab(InDialogueBuilderEditor->GetToolbarTabId(), ETabState::OpenedTab)
			)
			*/
			->Split
			(
				FTabManager::NewSplitter()->SetOrientation(Orient_Horizontal)
				->Split
				(
					FTabManager::NewSplitter()->SetOrientation(Orient_Vertical)
					->SetSizeCoefficient(0.15f)
					->Split
					(
						FTabManager::NewStack()->SetSizeCoefficient(1.f)
						->AddTab(FBlueprintEditorTabs::MyBlueprintID, ETabState::OpenedTab)
					)
				)
				
				->Split
				(
					FTabManager::NewSplitter()->SetOrientation(Orient_Vertical)
					->SetSizeCoefficient(0.70f)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.80f)
						->AddTab("Document", ETabState::ClosedTab)
					)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.20f)
						->AddTab(FBlueprintEditorTabs::CompilerResultsID, ETabState::ClosedTab)
						->AddTab(FBlueprintEditorTabs::FindResultsID, ETabState::ClosedTab)
						->AddTab(FBlueprintEditorTabs::BookmarksID, ETabState::ClosedTab)
					)
				)
				->Split
				(
					FTabManager::NewSplitter()->SetOrientation(Orient_Vertical)
					->SetSizeCoefficient(0.15f)
					->Split
					(
						FTabManager::NewStack()->SetSizeCoefficient(1.f)
						->AddTab(FBlueprintEditorTabs::DetailsID, ETabState::OpenedTab)
						->AddTab(FBlueprintEditorTabs::PaletteID, ETabState::ClosedTab)
						->AddTab(FBlueprintEditorTabs::DefaultEditorID, ETabState::ClosedTab)
					)
				)
			)
		);
		

	FBlueprintEditorModule& BlueprintEditorModule = FModuleManager::LoadModuleChecked<FBlueprintEditorModule>("Kismet");
	BlueprintEditorModule.OnRegisterTabsForEditor().Broadcast(StardardTabFactories, FDialogueBuilderCustomEditorModes::DialogueBuilderEditorMode1, InDialogueBuilderEditor);

	LayoutExtender = MakeShared<FLayoutExtender>();
	BlueprintEditorModule.OnRegisterLayoutExtensions().Broadcast(*LayoutExtender);

	if (UToolMenu* Toolbar = InDialogueBuilderEditor->RegisterModeToolbarIfUnregistered(GetModeName()))
	{
		InDialogueBuilderEditor->GetToolbarBuilder()->AddCompileToolbar(Toolbar);
		InDialogueBuilderEditor->GetToolbarBuilder()->AddBlueprintGlobalOptionsToolbar(Toolbar);
		InDialogueBuilderEditor->GetToolbarBuilder()->AddDebuggingToolbar(Toolbar);
		InDialogueBuilderEditor->GetToolbarBuilder()->AddScriptingToolbar(Toolbar);
		InDialogueBuilderEditor->GetToolbarBuilder()->AddNewToolbar(Toolbar);
	}
}


void FDialogueBuilderApplicationMode::RegisterTabFactories(TSharedPtr<FTabManager> InTabManager)
{
	TSharedPtr<FDialogueBuilderCustomEditor> Editor = DialogueBuilderEditor.Pin();
	// Tool bar tab
	Editor->RegisterToolbarTab(InTabManager.ToSharedRef());
	
	// Graph tab
	Editor->GetDocumentManager()->RegisterDocumentFactory(MakeShareable(new FDialogueBuilderGraphTabFactory(Editor)));
	
	// Other tabs
	Editor->PushTabFactories(CoreTabFactories);
	Editor->PushTabFactories(BlueprintEditorOnlyTabFactories);
	Editor->PushTabFactories(BlueprintEditorTabFactories);

	FApplicationMode::RegisterTabFactories(InTabManager);
}

void FDialogueBuilderApplicationMode::PostActivateMode()
{
	// Reopen any documents that were open when the blueprint was last saved
	TSharedPtr<FBlueprintEditor> BP = MyBlueprintEditor.Pin();
	BP->RestoreEditedObjectState();
	BP->SetupViewForBlueprintEditingMode();

	DialogueBuilderEditor.Pin()->InvokeDialogueBuilderGraphTab();

	FApplicationMode::PostActivateMode();
}
