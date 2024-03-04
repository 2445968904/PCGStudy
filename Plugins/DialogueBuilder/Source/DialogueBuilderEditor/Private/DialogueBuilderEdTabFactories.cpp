// Copyright 2022 Greg Shynkar. All Rights Reserved

#include "DialogueBuilderEdTabFactories.h"

#define LOCTEXT_NAMESPACE "DialogueBuilderCustomEditor"

////FDialogueBuilderDetailsTabFactory Part Start ////

FDialogueBuilderDetailsTabFactory::FDialogueBuilderDetailsTabFactory(TSharedPtr<class FDialogueBuilderCustomEditor> InDialogueBuilderEditor) : FWorkflowTabFactory(FName("Details"), InDialogueBuilderEditor)
, DialogueBuilderEditor(InDialogueBuilderEditor)
{
	TabLabel = LOCTEXT("DetailsLabel", "Details");
	TabIcon = FSlateIcon(FAppStyle::GetAppStyleSetName(), "Kismet.Tabs.Components");
	bIsSingleton = true;
	ViewMenuDescription = LOCTEXT("DetailsView", "Details");
	ViewMenuTooltip = LOCTEXT("DetailsView_ToolTip", "Show the details view");
}

TSharedRef<SWidget> FDialogueBuilderDetailsTabFactory::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	//Create a details panel;
	FDialogueBuilderCustomEditor* DialogueEditor = DialogueBuilderEditor.Pin().Get();
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

	const bool bIsUpdatable = true;
	const bool bIsLockable = false;

	//FDetailsViewArgs DetailsViewArgs(bIsUpdatable, bIsLockable, false, FDetailsViewArgs::HideNameArea, false);
	FDetailsViewArgs DetailsViewArgs = FDetailsViewArgs();
	DetailsViewArgs.NotifyHook = DialogueEditor;
	DetailsViewArgs.DefaultsOnlyVisibility = EEditDefaultsOnlyNodeVisibility::Show;

  	TSharedRef<IDetailsView> DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	DialogueEditor->DetailsViewTab = DetailsView;

	return
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.HAlign(HAlign_Fill)
		[
			DetailsView
		];
}

FText FDialogueBuilderDetailsTabFactory::GetTabToolTipText(const FWorkflowTabSpawnInfo& Info) const
{
	return LOCTEXT("DetailsTabTooltip", "The details tab allows editing of the properties");
}

////FDialogueBuilderDetailsTabFactory Part End ////


////FDialogueBuilderGraphTabFactory Part Start ////

FDialogueBuilderGraphTabFactory::FDialogueBuilderGraphTabFactory(TSharedPtr<FDialogueBuilderCustomEditor> InDialogueBuilderEditor) :
	FDocumentTabFactoryForObjects<UDialogueBuilderEdGraph>(FName("DialogueBuilderGraph"), InDialogueBuilderEditor)
{
	CurDialogueEditorRef = InDialogueBuilderEditor.Get();
	DialogueBuilderEditor = InDialogueBuilderEditor;

	TabLabel = LOCTEXT("DialogueBuilderGraphLabel", "DialogueBuilderGraph");
	TabIcon = FSlateIcon(FAppStyle::GetAppStyleSetName(), "Kismet.Tabs.Components");
	bIsSingleton = true;
	ViewMenuDescription = LOCTEXT("DialogueBuilderGraphView", "DialogueBuilderGraph");
	ViewMenuTooltip = LOCTEXT("DialogueBuilderGraphToolTip", "Dialogue Builder Graph");

}

void FDialogueBuilderGraphTabFactory::OnTabActivated(TSharedPtr<SDockTab> Tab) const
{
	check(DialogueBuilderEditor.IsValid());
	TSharedRef<SGraphEditor> DialogueGraphEditor = StaticCastSharedRef<SGraphEditor>(Tab->GetContent());
	CurDialogueEditorRef->OnGraphEditorFocused(DialogueGraphEditor);
}

void FDialogueBuilderGraphTabFactory::OnTabRefreshed(TSharedPtr<SDockTab> Tab) const
{
	TSharedRef<SGraphEditor> DialogueGraphEditor = StaticCastSharedRef<SGraphEditor>(Tab->GetContent());
	DialogueGraphEditor->NotifyGraphChanged();
}

TAttribute<FText> FDialogueBuilderGraphTabFactory::ConstructTabNameForObject(UDialogueBuilderEdGraph* DocumentID) const
{
	FString Name = "DialogueGraph";
	return TAttribute<FText>(FText::FromString(Name));
}

TSharedRef<SWidget> FDialogueBuilderGraphTabFactory::CreateTabBodyForObject(const FWorkflowTabSpawnInfo& Info, UDialogueBuilderEdGraph* DocumentID) const
{
	check(DocumentID);

	SGraphEditor::FGraphEditorEvents InEvents;
	InEvents.OnSelectionChanged = SGraphEditor::FOnSelectionChanged::CreateSP(CurDialogueEditorRef, &FDialogueBuilderCustomEditor::OnSelectedNodesChanged);
	InEvents.OnNodeDoubleClicked = FSingleNodeEvent::CreateSP(CurDialogueEditorRef, &FDialogueBuilderCustomEditor::OnNodeDoubleClickedDB);

	FGraphAppearanceInfo AppearanceInfo;
	AppearanceInfo.CornerText = FText::FromString("Dialogue");

	// Make title bar
	TSharedRef<SWidget> TitleBarWidget =
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush(TEXT("Graph.TitleBackground")))
		.HAlign(HAlign_Fill)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
		.HAlign(HAlign_Center)
		.FillWidth(1.f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("DialogueBuilderGraphLabel", "Dialogue Builder"))
		.TextStyle(FAppStyle::Get(), TEXT("GraphBreadcrumbButtonText"))
		]
		];
		
	TSharedRef<SGraphEditor> LocGraphEditor = SNew(SGraphEditor)
		.AdditionalCommands(DialogueBuilderEditor.Pin()->GraphEditorCommandsRef)
		.IsEditable(true)
		.Appearance(AppearanceInfo)
		.TitleBar(TitleBarWidget)
		.GraphToEdit(DocumentID)
		.GraphEvents(InEvents);
	
	CurDialogueEditorRef->GraphEditor = LocGraphEditor;
	
	return LocGraphEditor;
}

const FSlateBrush* FDialogueBuilderGraphTabFactory::GetTabIconForObject(const FWorkflowTabSpawnInfo& Info, UDialogueBuilderEdGraph* DocumentID) const
{
	return  FAppStyle::GetBrush(TEXT("Graph.StateNode.Icon"));
}
////FDialogueBuilderGraphTabFactory Part End ////

#undef LOCTEXT_NAMESPACE