// Copyright 2022 PufStudio. All Rights Reserved.

#include "AutoPaintRuleEditor.h"
#include "EdMode.h"
#include "SAdvancedPreviewDetailsTab.h"

#include "TiledLevelCommands.h"
#include "TiledLevelEditorLog.h"
#include "AutoPaintHelper.h"
#include "SAutoPaintItemPalette.h"
#include "AutoPaintRule.h"
#include "SAutoPaintRuleEditorViewport.h"
#include "SAutoPaintRuleDetailsEditor.h"
#include "SSingleObjectDetailsPanel.h"
#include "TiledLevel.h"
#include "Dialogs/Dialogs.h"
#include "Framework/Commands/GenericCommands.h"
#include "Internationalization/Regex.h"
#include "Kismet/GameplayStatics.h"
#include "Serialization/JsonTypes.h"
#include "TiledLevelEditor/TiledLevelEdMode.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/Colors/SColorPicker.h"

#define LOCTEXT_NAMESPACE "AutoPaintRuleEditor"


//////////////////////////////////////////////

const FName AutoPaintRuleEditorAppName = FName(TEXT("AutoPaintRuleEditorApp"));

///////////////////////////////////////////////////////
struct FAutoPaintRuleEditorTabs
{
	static const FName ToolboxID;
	static const FName RulesID;
	static const FName PreviewID;
	static const FName PreviewSettingsID;
};

const FName FAutoPaintRuleEditorTabs::ToolboxID(TEXT("Palette"));
const FName FAutoPaintRuleEditorTabs::RulesID(TEXT("Rules"));
const FName FAutoPaintRuleEditorTabs::PreviewID(TEXT("Preview"));
const FName FAutoPaintRuleEditorTabs::PreviewSettingsID(TEXT("PreviewSettings"));
//////////////////////////////////////////////////////


/////////////////////////////////////////////////////

class SAutoPaintRuleEditorDetails : public SSingleObjectDetailsPanel
{
public:
	SLATE_BEGIN_ARGS(SAutoPaintRuleEditorDetails) {}
	SLATE_END_ARGS()

private:
	TWeakPtr<class FAutoPaintRuleEditor> EditorPtr;

public:
	void Construct(const FArguments& InArgs, TSharedPtr<FAutoPaintRuleEditor> InEditor)
	{
		EditorPtr = InEditor;
		SSingleObjectDetailsPanel::Construct(
			SSingleObjectDetailsPanel::FArguments()
			.HostCommandList(InEditor->GetToolkitCommands())
			.HostTabManager(InEditor->GetTabManager()),
			true,
			true
		);
	}

	virtual UObject* GetObjectToObserve() const override
	{
		return EditorPtr.Pin()->GetTempAsset();	
	}

	virtual TSharedRef<SWidget> PopulateSlot(TSharedRef<SWidget> PropertyEditorWidget) override
	{
		return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.FillHeight(1)
		[
			PropertyEditorWidget
		];
	}
};

//////////////////////////////////////////////////
///
FAutoPaintRuleEditor::FAutoPaintRuleEditor()
{
}

FAutoPaintRuleEditor::~FAutoPaintRuleEditor()
{
}

void FAutoPaintRuleEditor::InitRuleEditor(const EToolkitMode::Type Mode,
                                                 const TSharedPtr<IToolkitHost>& InitToolkitHost,
                                                 UAutoPaintRule* InRule)
{
	GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->CloseOtherEditors(InRule, this);
	RuleBeingEdited = InRule;
	RuleBeingEdited->SelectionState = EAutoPaintEditorSelectionState::None;
	RuleBeingEdited->RequestForTransaction.BindRaw(this, &FAutoPaintRuleEditor::MakeTransactionForAsset);
	CommandList = MakeShareable(new FUICommandList);
	GetToolkitCommands()->Append(CommandList.ToSharedRef());
	BindCommands();
	ViewportPtr = SNew(SAutoPaintRuleEditorViewport, SharedThis(this));
	ToolboxPtr = SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(0.f);
	
		// Layout
	const TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout("Standalone_AutoPaintRuleEditor_13207")
	->AddArea
	(
		FTabManager::NewPrimaryArea()->SetOrientation(Orient_Horizontal)
		->Split
		(
			FTabManager::NewSplitter()
			->SetOrientation(Orient_Horizontal)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.3)
				->SetHideTabWell(true)
				->AddTab(FAutoPaintRuleEditorTabs::ToolboxID, ETabState::OpenedTab)
			)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.3)
				->SetHideTabWell(true)
				->AddTab(FAutoPaintRuleEditorTabs::RulesID, ETabState::OpenedTab)
			)
			->Split
			(
				FTabManager::NewStack()
				->SetHideTabWell(true)
				->AddTab(FAutoPaintRuleEditorTabs::PreviewID, ETabState::OpenedTab)
			)
			->Split
			(
				FTabManager::NewStack()
				->AddTab(FAutoPaintRuleEditorTabs::PreviewSettingsID, ETabState::SidebarTab, ESidebarLocation::Right,
					0.14f)
			)
		)
	);

	InitAssetEditor(
		Mode,
		InitToolkitHost,
		AutoPaintRuleEditorAppName,
		StandaloneDefaultLayout,
		true,
		true,
		InRule
	);

	ExtendToolbar();
	ExtendMenu();
	RegenerateMenusAndToolbars();
	ViewportPtr->ActivateEdMode();
	RuleBeingEdited->RequestForUpdateMatchHint.BindRaw(this, &FAutoPaintRuleEditor::UpdateMatchedRuleHint);

}

void FAutoPaintRuleEditor::SetContentFromEdMode(FTiledLevelEdMode* InEdMode)
{
	EdMode = InEdMode;
	PalettePtr = EdMode->GetAutoPalettePtr();
	PalettePtr->ParentCommandList = CommandList;
	ToolboxPtr->SetContent(EdMode->GetToolkit()->GetInlineContent().ToSharedRef());
}

UTiledLevelAsset* FAutoPaintRuleEditor::GetTempAsset()
{
	UWorld* EditorWorld = ViewportPtr->GetViewportClient()->GetWorld();
	if (ATiledLevel* TempLevel = Cast<ATiledLevel>(UGameplayStatics::GetActorOfClass(EditorWorld, ATiledLevel::StaticClass())))
		return TempLevel->GetAsset();
	return nullptr;
}

void FAutoPaintRuleEditor::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(
		LOCTEXT("WorkspaceMenu_TiledElementSetEditor", "Tiled Element Set Editor"));
	TSharedRef<FWorkspaceItem> WorkspaceMenuCategoryRef = WorkspaceMenuCategory.ToSharedRef();

	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);
	InTabManager->RegisterTabSpawner(FAutoPaintRuleEditorTabs::ToolboxID,
	                                 FOnSpawnTab::CreateSP(this, &FAutoPaintRuleEditor::SpawnTab_Toolbox))
	            .SetDisplayName(LOCTEXT("DetailTab_Description", "Palette"))
	            .SetGroup(WorkspaceMenuCategoryRef)
	            .SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));
	InTabManager->RegisterTabSpawner(FAutoPaintRuleEditorTabs::PreviewID,
	                                 FOnSpawnTab::CreateSP(this, &FAutoPaintRuleEditor::SpawnTab_Preview))
	            .SetDisplayName(LOCTEXT("PreviewTab_Description", "Preview Viewport"))
	            .SetTooltipText(LOCTEXT("PreviewTab_Tooltip", "Preview Rule"))
	            .SetGroup(WorkspaceMenuCategoryRef)
	            .SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Viewports"));
	InTabManager->RegisterTabSpawner(FAutoPaintRuleEditorTabs::RulesID,
	                                 FOnSpawnTab::CreateSP(this, &FAutoPaintRuleEditor::SpawnTab_Rules))
	            .SetDisplayName(LOCTEXT("DetailTab_Description", "Rule Detail"))
	            .SetGroup(WorkspaceMenuCategoryRef)
	            .SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));
	InTabManager->RegisterTabSpawner(FAutoPaintRuleEditorTabs::PreviewSettingsID,
	                                 FOnSpawnTab::CreateSP(this, &FAutoPaintRuleEditor::SpawnTab_PreviewSettings))
	            .SetDisplayName(LOCTEXT("PropertiesTabLabel", "Preview Settings"))
	            .SetGroup(WorkspaceMenuCategory.ToSharedRef())
	            .SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));
}

void FAutoPaintRuleEditor::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);

	InTabManager->UnregisterTabSpawner(FAutoPaintRuleEditorTabs::ToolboxID);
	InTabManager->UnregisterTabSpawner(FAutoPaintRuleEditorTabs::RulesID);
	InTabManager->UnregisterTabSpawner(FAutoPaintRuleEditorTabs::PreviewID);
	InTabManager->UnregisterTabSpawner(FAutoPaintRuleEditorTabs::PreviewSettingsID);
}

FName FAutoPaintRuleEditor::GetToolkitFName() const
{
	return FName("TiledItemSetEditor");
}

FText FAutoPaintRuleEditor::GetBaseToolkitName() const
{
	return LOCTEXT("TiledItemSetEditorAppLabelBase", "Tiled Item Set Editor");
}

FText FAutoPaintRuleEditor::GetToolkitName() const
{
	const bool bDirtyState = RuleBeingEdited->GetOutermost()->IsDirty();
	return FText::Format(
		LOCTEXT("TiledItemSetEditorAppLabel", "{0}{1}"), FText::FromString(RuleBeingEdited->GetName()),
		bDirtyState ? FText::FromString(TEXT("*")) : FText::GetEmpty());
}

FText FAutoPaintRuleEditor::GetToolkitToolTipText() const
{
	return GetToolTipTextForObject(RuleBeingEdited);
}

FLinearColor FAutoPaintRuleEditor::GetWorldCentricTabColorScale() const
{
	return FLinearColor::White;
}

FString FAutoPaintRuleEditor::GetWorldCentricTabPrefix() const
{
	return TEXT("AutoPaintRuleEditor");
}

void FAutoPaintRuleEditor::OnToolkitHostingStarted(const TSharedRef<IToolkit>& Toolkit)
{
	TSharedPtr<SWidget> InlineContent = Toolkit->GetInlineContent();
	if (InlineContent.IsValid())
	{
		ToolboxPtr->SetContent(InlineContent.ToSharedRef());
	}
}

void FAutoPaintRuleEditor::OnToolkitHostingFinished(const TSharedRef<IToolkit>& Toolkit)
{
	ToolboxPtr->SetContent(SNullWidget::NullWidget);
}

void FAutoPaintRuleEditor::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(RuleBeingEdited);
}

void FAutoPaintRuleEditor::MakeTransactionForAsset(FText TransactionMessage)
{
	if (!RuleBeingEdited) return;
	FScopedTransaction Transaction(TransactionMessage);
	RuleBeingEdited->Modify();
}

void FAutoPaintRuleEditor::BindCommands()
{
	// const TSharedRef<FUICommandList>& CommandList = GetToolkitCommands();
	const FTiledLevelCommands& TiledLevelCommands = FTiledLevelCommands::Get();
	const FGenericCommands& GenericCommands = FGenericCommands::Get();

	CommandList->MapAction(TiledLevelCommands.RespawnAutoPaintInstances,
		FUIAction(FExecuteAction::CreateLambda([this](){if (EdMode) EdMode->UpdateAutoPaintPlacements();}))
	);

	CommandList->MapAction(TiledLevelCommands.CreateNewRuleItem,
		FUIAction(FExecuteAction::CreateLambda([this](){this->AddNewItem();}))
	);
	CommandList->MapAction(TiledLevelCommands.CreateNewRuleGroup,
		FUIAction(FExecuteAction::CreateLambda([this](){this->AddNewRuleGroup();}))
	);
	
	CommandList->MapAction(
		GenericCommands.Duplicate,
		FUIAction(
			FExecuteAction::CreateSP(this, &FAutoPaintRuleEditor::DuplicateRule),
			FCanExecuteAction::CreateLambda([this](){return RuleBeingEdited->SelectionState == EAutoPaintEditorSelectionState::Rule;})
		)
	);
	CommandList->MapAction(
		GenericCommands.Copy,
		FUIAction(
			FExecuteAction::CreateSP(this, &FAutoPaintRuleEditor::CopyRule),
			FCanExecuteAction::CreateLambda([this](){return RuleBeingEdited->SelectionState == EAutoPaintEditorSelectionState::Rule;})
		)
	);
	CommandList->MapAction(
		GenericCommands.Paste,
		FUIAction(
			FExecuteAction::CreateSP(this, &FAutoPaintRuleEditor::PasteRule),
			FCanExecuteAction::CreateSP(this, &FAutoPaintRuleEditor::CanPasteRule)
		)
	);
	CommandList->MapAction(
		GenericCommands.Delete,
		FUIAction(
			 FExecuteAction::CreateSP(this, &FAutoPaintRuleEditor::OnRemoveSomething),
			 FCanExecuteAction::CreateSP(this, &FAutoPaintRuleEditor::CanRemoveSomething)
		)
	);
}

void FAutoPaintRuleEditor::ExtendMenu()
{
}

void FAutoPaintRuleEditor::OnConfirmCreateOrEditItem(FAutoPaintItem* EditingItem)
{
	if (CandidateNewItemName.IsEmptyOrWhitespace()) return;
	bool ShouldCheckName = true;
	if (EditingItem && EditingItem->ItemName.ToString() == CandidateNewItemName.ToString() )
	{
		ShouldCheckName = false;
	}
	// if (EditingItem->ItemName.ToString() != CandidateNewItemName.ToString() || !EditingItem)
	if (ShouldCheckName)
	{
		// handle name duplication issue
		const FString PatternString(TEXT("_(\\d+)$"));
		const FRegexPattern Pattern(PatternString);
		TArray<FName> ItemNames;
		for (auto Item : RuleBeingEdited->Items)
		{
			ItemNames.Add(Item.ItemName);
		}

		int Version = 0;
		while (ItemNames.Contains(FName(CandidateNewItemName.ToString())))
		{
			FRegexMatcher Matcher = FRegexMatcher(Pattern, CandidateNewItemName.ToString());
			if (Matcher.FindNext())
			{
				CandidateNewItemName = FText::FromString(CandidateNewItemName.ToString().Replace(*Matcher.GetCaptureGroup(0), TEXT("")));
			}
			CandidateNewItemName = FText::FromString(CandidateNewItemName.ToString() + TEXT("_") + FString::FromInt(Version));
			Version++;
		}
	}

	// make transaction
	if (EditingItem)
	{
		FScopedTransaction Transaction(LOCTEXT("EditAutoPaintItem", "Edit auto paint item"));
		RuleBeingEdited->Modify();
		PalettePtr->OnPaintItemChanged.Broadcast(
			 FAutoPaintItem(EditingItem->ItemName, EditingItem->PreviewColor), FAutoPaintItem(FName(CandidateNewItemName.ToString()), CandidateNewItemColor));
		EditingItem->ItemName = FName(CandidateNewItemName.ToString());
		EditingItem->PreviewColor = CandidateNewItemColor;
	}
	else
	{
		FScopedTransaction Transaction(LOCTEXT("AddAutoPaintItem", "Add auto paint item"));
		RuleBeingEdited->Modify();
		RuleBeingEdited->Items.Add(FAutoPaintItem(FName(CandidateNewItemName.ToString()), CandidateNewItemColor));
	}
	PalettePtr->UpdatePalette(true);
}

void FAutoPaintRuleEditor::AddNewItem()
{
	// make dialog
	SGenericDialogWidget::FArguments DialogArgs;
	DialogArgs.OnOkPressed_Lambda([this](){
		OnConfirmCreateOrEditItem();
	});
	SGenericDialogWidget::OpenDialog(
		LOCTEXT("AddNewItemDialog_title", "Add new auto paint item"),
		MakeDialogContent_CreateOrEditItem(),
		DialogArgs,
		false
		);
}

// TODO: Edit auto paint item like tiled level item... instead of pop an additional widget...
/*
 * Need to change FAutoPaintItem to UAutoPaintItem... since SSinglePropertyView or SDetailView require an object...
 * It's too deep to reach the struct from it parent object (UAutoPaintRule) ...
 */
TSharedRef<SWidget> FAutoPaintRuleEditor::MakeDialogContent_CreateOrEditItem(bool IsCreate)
{
	if (IsCreate)
	{
		CandidateNewItemName = FText::FromName("Auto");
		CandidateNewItemColor = FLinearColor::Red;
	}
	else
	{
		const FAutoPaintItem* ItemToEdit = PalettePtr->GetSelectedAutoPaintItem();
		CandidateNewItemName = FText::FromName(ItemToEdit->ItemName);
		CandidateNewItemColor = ItemToEdit->PreviewColor;
	}
	
	FColorPickerArgs PickerArgs;
	PickerArgs.bIsModal = true;
	PickerArgs.InitialColor = CandidateNewItemColor;
	PickerArgs.OnColorCommitted = FOnLinearColorValueChanged::CreateLambda([=](const FLinearColor& NewColor)
	{
		CandidateNewItemColor = NewColor;
	});
	
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("NewItemName_Label", "Item Name: "))
			]
			+ SHorizontalBox::Slot()
			[
				SNew(SEditableTextBox)
				.Text(CandidateNewItemName)
				.OnTextCommitted_Lambda([this](const FText& NewName, ETextCommit::Type CommitType)
				{
					CandidateNewItemName = NewName;
				})
			]
		]
		+ SVerticalBox::Slot()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("NewItemColor_Label", "Item Color: "))
			]
			+ SHorizontalBox::Slot()
			[
				SNew(SButton)
				.OnClicked_Lambda([=]()
				{
					OpenColorPicker(PickerArgs);
					return FReply::Handled();
				})
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				[
					SNew(SColorBlock)
					.Color_Lambda([this](){ return CandidateNewItemColor; })
				]
			]
		];
}

TSharedRef<SWidget> FAutoPaintRuleEditor::MakeDialogContent_CreateOrEditGroup(bool IsCreate)
{
	if (IsCreate)
	{
		CandidateNewGroupName = FText::FromString("Default");
	}
	else
	{
		CandidateNewGroupName = FText::FromName(DetailsEditorPtr->GetSelectedAutoPaintGroup()->GroupName);
	}
	
	return SNew(SHorizontalBox)
	+ SHorizontalBox::Slot()
	.VAlign(VAlign_Center)
	.AutoWidth()
	[
		SNew(STextBlock)
		.Text(LOCTEXT("NewGroupName_Label", "New Group Name: "))
	]
	+ SHorizontalBox::Slot()
	.HAlign(HAlign_Left)
	.AutoWidth()
	[
		SNew(SBox)
		.HeightOverride(32)
		.WidthOverride(128)
		[
			SNew(SEditableTextBox)
			.Text(CandidateNewGroupName)
			.OnTextCommitted_Lambda([this](const FText& Text, ETextCommit::Type Arg)
			{
				CandidateNewGroupName = Text;
			})	
		]
	];
}

void FAutoPaintRuleEditor::OnConfirmCreateOrEditGroup(FAutoPaintRuleGroup* EditingGroup)
{
	if (CandidateNewGroupName.IsEmptyOrWhitespace()) return;
	// handle name duplication issue
	const FString PatternString(TEXT("_(\\d+)$"));
	const FRegexPattern Pattern(PatternString);
	TArray<FName> GroupNames;
	for (auto Group : RuleBeingEdited->Groups)
	{
		GroupNames.Add(Group.GroupName);
	}

	int Version = 0;
	while (GroupNames.Contains(FName(CandidateNewGroupName.ToString())))
	{
		FRegexMatcher Matcher = FRegexMatcher(Pattern, CandidateNewGroupName.ToString());
		if (Matcher.FindNext())
		{
			CandidateNewGroupName = FText::FromString(CandidateNewGroupName.ToString().Replace(*Matcher.GetCaptureGroup(0), TEXT("")));
		}
		CandidateNewGroupName = FText::FromString(CandidateNewGroupName.ToString() + TEXT("_") + FString::FromInt(Version));
		Version++;
	}

	if (EditingGroup)
	{
		FScopedTransaction Transaction(LOCTEXT("EditAutoPaintRuleGroup", "Edit auto paint rule group"));
		RuleBeingEdited->Modify();
		EditingGroup->GroupName = FName(CandidateNewGroupName.ToString());
	}
	else
	{
		FScopedTransaction Transaction(LOCTEXT("AddAutoPaintRuleGroup", "Add auto paint rule group"));
		RuleBeingEdited->Modify();
		RuleBeingEdited->Groups.Add(FAutoPaintRuleGroup(FName(CandidateNewGroupName.ToString())));
	}
	DetailsEditorPtr->UpdateEditor();
}

void FAutoPaintRuleEditor::AddNewRuleGroup()
{
	// make dialog
	SGenericDialogWidget::FArguments DialogArgs;
	DialogArgs.OnOkPressed_Lambda([this]()
	{
		OnConfirmCreateOrEditGroup();
	});
	SGenericDialogWidget::OpenDialog(
		LOCTEXT("AddNewRuleGroup_title", "Add new rule group"),
		MakeDialogContent_CreateOrEditGroup(),
		DialogArgs,
	false
	);
}

void FAutoPaintRuleEditor::DuplicateRule() const
{
	FName SelectedGroupName;
	int RuleIdx;
	DetailsEditorPtr->GetSelectedItemRuleInfo(SelectedGroupName, RuleIdx);
	UAutoPaintItemRule* NewRule = NewObject<UAutoPaintItemRule>(RuleBeingEdited, NAME_None, RF_Transactional);
	NewRule->DeepCopy(*RuleBeingEdited->Groups.FindByKey(SelectedGroupName)->Rules[RuleIdx]);
	FScopedTransaction Transaction(LOCTEXT("DuplicateAutoRule", "Duplicate auto rule"));
	RuleBeingEdited->Groups.FindByKey(SelectedGroupName)->Rules.Insert(NewRule, RuleIdx);
	DetailsEditorPtr->UpdateEditor();
}


bool FAutoPaintRuleEditor::CanEditSomething() const
{
	if (!DetailsEditorPtr.IsValid() || !PalettePtr.IsValid() || !IsValid(RuleBeingEdited)) return false;
	if (RuleBeingEdited->SelectionState == EAutoPaintEditorSelectionState::Item)
	{
		if (PalettePtr->GetSelectedAutoPaintItem())
			return PalettePtr->GetSelectedAutoPaintItem()->ItemName != FName("Empty");
		return false;
	}
	return RuleBeingEdited->SelectionState == EAutoPaintEditorSelectionState::Group;
}

void FAutoPaintRuleEditor::OnEditSelectedItemOrGroup()
{
	SGenericDialogWidget::FArguments DialogArgs;
	if (FAutoPaintItem* SelectedItem = PalettePtr->GetSelectedAutoPaintItem())
	{
		DialogArgs.OnOkPressed_Raw(this, &FAutoPaintRuleEditor::OnConfirmCreateOrEditItem, SelectedItem);
		SGenericDialogWidget::OpenDialog(
			LOCTEXT("EditItemDialog_title", "Edit auto paint item"),
			MakeDialogContent_CreateOrEditItem(false),
			DialogArgs,
			false
			);
	}
	else if (FAutoPaintRuleGroup* SelectedGroup = DetailsEditorPtr->GetSelectedAutoPaintGroup())
	{
		DialogArgs.OnOkPressed_Raw(this, &FAutoPaintRuleEditor::OnConfirmCreateOrEditGroup, SelectedGroup);
		SGenericDialogWidget::OpenDialog(
			LOCTEXT("EditGroupDialog_title", "Edit auto paint rule group"),
			MakeDialogContent_CreateOrEditGroup(false),
			DialogArgs,
			false
			);
		
	}
}

void FAutoPaintRuleEditor::OnRemoveSomething()
{
	if (RuleBeingEdited->SelectionState == EAutoPaintEditorSelectionState::Rule) // remove rule
	{
		FScopedTransaction Transaction(LOCTEXT("DeleteAutoRule", "Delete auto rule"));
		RuleBeingEdited->Modify();
		FName GroupName;
		int RuleIdx;
		DetailsEditorPtr->GetSelectedItemRuleInfo(GroupName, RuleIdx);
		RuleBeingEdited->Groups.FindByKey(GroupName)->Rules.RemoveAt(RuleIdx);
		DetailsEditorPtr->UpdateEditor();
	}
	else if (RuleBeingEdited->SelectionState == EAutoPaintEditorSelectionState::Group) // remove whole group
	{
		FScopedTransaction Transaction(LOCTEXT("DeleteAutoRuleGroup", "Delete auto rule group"));
		RuleBeingEdited->Modify();
		RuleBeingEdited->Groups.RemoveAt(RuleBeingEdited->Groups.IndexOfByKey(DetailsEditorPtr->GetSelectedAutoPaintGroup()->GroupName));
		DetailsEditorPtr->UpdateEditor();
	}
	else // remove item
	{
		// notify the asset? how?
		FScopedTransaction Transaction(LOCTEXT("DeleteAutoItem", "Delete auto item"));
		RuleBeingEdited->Modify();
		const FName NameToRemove = PalettePtr->GetSelectedAutoPaintItem()->ItemName;
		RuleBeingEdited->Items.RemoveAt(RuleBeingEdited->Items.IndexOfByKey(NameToRemove));
		for (auto& Group : RuleBeingEdited->Groups)
		{
			for (auto& Rule : Group.Rules)
			{
				TArray<FIntVector> KeysToRemove;
				for (auto [k, v] : Rule->AdjacencyRules)
				{
					if (v.AutoPaintItemName == NameToRemove)
						KeysToRemove.Add(k);
				}
				for (auto k : KeysToRemove)
					Rule->AdjacencyRules.Remove(k);
			}
		}
		PalettePtr->OnPaintItemRemoved.Broadcast(NameToRemove);
		PalettePtr->UpdatePalette(true);
		DetailsEditorPtr->UpdateEditor();
	}
}

bool FAutoPaintRuleEditor::CanRemoveSomething() const
{
	if (!DetailsEditorPtr.IsValid() || !PalettePtr.IsValid()) return false;
	switch (RuleBeingEdited->SelectionState) {
		case EAutoPaintEditorSelectionState::None:
			return false;
	case EAutoPaintEditorSelectionState::Item:
		if (PalettePtr->GetSelectedAutoPaintItem())
			return PalettePtr->GetSelectedAutoPaintItem()->ItemName != FName("Empty");
		return false;
	case EAutoPaintEditorSelectionState::Group:
		return RuleBeingEdited->Groups.Num() > 1;
	case EAutoPaintEditorSelectionState::Rule:
		return true;
	default:
		return false;
	}
}

void FAutoPaintRuleEditor::CopyRule()
{
	CopiedItemRule = DetailsEditorPtr->GetSelectedAutoPaintRule();
}

void FAutoPaintRuleEditor::PasteRule() const
{
	FScopedTransaction Transaction(LOCTEXT("PasteAutoItem", "Paste auto item"));
	RuleBeingEdited->Modify();
	DetailsEditorPtr->GetSelectedAutoPaintRule()->DeepCopy(*CopiedItemRule);
	DetailsEditorPtr->UpdateEditor();
}

bool FAutoPaintRuleEditor::CanPasteRule() const
{
	return CopiedItemRule != nullptr && RuleBeingEdited->SelectionState == EAutoPaintEditorSelectionState::Rule;
}

void FAutoPaintRuleEditor::FillToolbar(FToolBarBuilder& ToolbarBuilder)
{
	const FTiledLevelCommands& TiledLevelCommands = FTiledLevelCommands::Get();
	const FGenericCommands& GenericCommands = FGenericCommands::Get();
	
	ToolbarBuilder.BeginSection("Add");
	{
		ToolbarBuilder.AddToolBarButton(TiledLevelCommands.CreateNewRuleItem,
			NAME_None,
			TAttribute<FText>(), 
			TAttribute<FText>(),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Plus")
		);
		ToolbarBuilder.AddToolBarButton(TiledLevelCommands.CreateNewRuleGroup,
			NAME_None,
			TAttribute<FText>(), 
			TAttribute<FText>(),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Plus")
		);
		
	}
	ToolbarBuilder.EndSection();
	ToolbarBuilder.BeginSection("Common");
	{
		ToolbarBuilder.AddToolBarButton(
			FUIAction(
				FExecuteAction::CreateSP(this, &FAutoPaintRuleEditor::OnEditSelectedItemOrGroup),
				FCanExecuteAction::CreateSP(this, &FAutoPaintRuleEditor::CanEditSomething)
				),
			NAME_None,
			LOCTEXT("EditSomething_Label", "Edit"),
			LOCTEXT("EditSometime_Tooltip", "Edit selected item/group"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Edit")
			);
		ToolbarBuilder.AddToolBarButton(GenericCommands.Delete, NAME_None, TAttribute<FText>(),
			LOCTEXT("RemoveSometime_Tooltip", "Remove selected item/group/rule"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Delete"));
	}
	ToolbarBuilder.EndSection();
	ToolbarBuilder.BeginSection("Auto rule detail");
	{
		
		ToolbarBuilder.AddToolBarButton(GenericCommands.Duplicate, NAME_None, TAttribute<FText>(),
			LOCTEXT("DuplicateRule_Tooltip", "Duplicate selected rule"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Duplicate"));
		ToolbarBuilder.AddToolBarButton(GenericCommands.Copy, NAME_None, TAttribute<FText>(),
			LOCTEXT("CopyRule_tooltip", "Copy selected rule"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "GenericCommands.Copy"));
		ToolbarBuilder.AddToolBarButton(GenericCommands.Paste, NAME_None, TAttribute<FText>(),
			LOCTEXT("PasteRule_Tooltip", "Paste selected rule"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "GenericCommands.Paste"));
	}
	ToolbarBuilder.EndSection();
	ToolbarBuilder.BeginSection("Instances Utils");
	{
		ToolbarBuilder.AddToolBarButton(TiledLevelCommands.RespawnAutoPaintInstances, NAME_None, TAttribute<FText>(),
			TAttribute<FText>(), FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Refresh"));
	}
	ToolbarBuilder.EndSection();
}

void FAutoPaintRuleEditor::ExtendToolbar()
{
	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
	ToolbarExtender->AddToolBarExtension(
		"Asset",
		EExtensionHook::After,
		ToolkitCommands,
		FToolBarExtensionDelegate::CreateSP(this, &FAutoPaintRuleEditor::FillToolbar)
	);
	AddToolbarExtender(ToolbarExtender);
}

void FAutoPaintRuleEditor::UpdateMatchedRuleHint(UAutoPaintItemRule* TargetItemRule) const
{
	if (!ViewportPtr.IsValid()) return;
	if (!ViewportPtr->GetViewportClient().IsValid()) return;
	const UWorld* EditorWorld = ViewportPtr->GetViewportClient()->GetWorld();
	if (!EditorWorld) return;
	TArray<AActor*> Out;
	UGameplayStatics::GetAllActorsOfClass(EditorWorld, AAutoPaintHelper::StaticClass(), Out);
	if (AAutoPaintHelper* Helper = Cast<AAutoPaintHelper>(UGameplayStatics::GetActorOfClass(EditorWorld, AAutoPaintHelper::StaticClass())))
	{
		 Helper->UpdateRulePreview(TargetItemRule);
		if (!TargetItemRule || !RuleBeingEdited->GetMatchedPositions().Find(TargetItemRule))
		{
			Helper->UpdateMatchedHint({});
		}
		else
		{
			Helper->UpdateMatchedHint(RuleBeingEdited->GetMatchedPositions()[TargetItemRule]);
		}
	}
}

TSharedRef<SDockTab> FAutoPaintRuleEditor::SpawnTab_Toolbox(const FSpawnTabArgs& SpawnTabArgs)
{
	TSharedRef<SDockTab> SpawnedTab =
		SNew(SDockTab)
		.Label(LOCTEXT("Toolbox_Title", "Toolbox"))
		.TabColorScale(GetTabColorScale())
		[
			ToolboxPtr.ToSharedRef()
		];
	return SpawnedTab;
}


TSharedRef<SDockTab> FAutoPaintRuleEditor::SpawnTab_Rules(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.Label(LOCTEXT("RulesTab_Title", "Rules"))
	[
		SAssignNew(DetailsEditorPtr, SAutoPaintRuleDetailsEditor, RuleBeingEdited, GetToolkitCommands())
	];
}

TSharedRef<SDockTab> FAutoPaintRuleEditor::SpawnTab_Preview(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.Label(LOCTEXT("PreviewTab_Title", "Preview"))
		[
			ViewportPtr.ToSharedRef()
		];
}

TSharedRef<SDockTab> FAutoPaintRuleEditor::SpawnTab_PreviewSettings(const FSpawnTabArgs& Args)
{
	TSharedPtr<FAdvancedPreviewScene> PreviewScene;
	TSharedPtr<SWidget> SettingsWidget = SNullWidget::NullWidget;
	if (ViewportPtr.IsValid())
	{
		PreviewScene = ViewportPtr->GetPreviewScene();
	}
	if (PreviewScene.IsValid())
	{
		TSharedPtr<SAdvancedPreviewDetailsTab> PreviewSettingsWidget = SNew(
			SAdvancedPreviewDetailsTab, PreviewScene.ToSharedRef());
		PreviewSettingsWidget->Refresh();
		SettingsWidget = PreviewSettingsWidget;
	}

	return SNew(SDockTab)
	.Label(LOCTEXT("PreviewSettings_Title", "Preview Settings"))
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(STextBlock)
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
			.Text(LOCTEXT("PreviewLevelTitle", "Level"))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SAutoPaintRuleEditorDetails, SharedThis(this))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 12, 0, 0)
		[
			SNew(STextBlock)
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
			.Text(LOCTEXT("PreviewSceneTitle", "Scene"))
		]
		+ SVerticalBox::Slot()
		.FillHeight(2.0)
		[
			SettingsWidget.ToSharedRef()
		]
	];
}


#undef LOCTEXT_NAMESPACE
