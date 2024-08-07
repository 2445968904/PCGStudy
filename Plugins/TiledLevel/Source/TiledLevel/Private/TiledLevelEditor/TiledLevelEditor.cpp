// Copyright 2022 PufStudio. All Rights Reserved.

#include "TiledLevelEditor.h"
#include "AdvancedPreviewScene.h"
#include "EditorModeManager.h"
#include "SAdvancedPreviewDetailsTab.h"
#include "SSingleObjectDetailsPanel.h"
#include "STiledLevelEditorViewport.h"
#include "TiledLevel.h"
#include "TiledLevelAsset.h"
#include "TiledLevelCommands.h"
#include "TiledLevelEditorLog.h"
#include "TiledLevelEditorUtility.h"
#include "Kismet/GameplayStatics.h"

#define LOCTEXT_NAMESPACE "TiledLevelEditor"

/////////////////////////////////////////////////

const FName TiledLevelEditorAppName = FName(TEXT("TiledLevelEditorApp"));

///////////////////////////////////////////////////////

struct FTiledLevelEditorTabs
{
	static const FName ToolboxID;
	static const FName ViewportID;
	static const FName DetailsID;
	static const FName PreviewSettingsID;
};

const FName FTiledLevelEditorTabs::ToolboxID(TEXT("Toolbox"));
const FName FTiledLevelEditorTabs::ViewportID(TEXT("Viewport"));
const FName FTiledLevelEditorTabs::DetailsID(TEXT("Details"));
const FName FTiledLevelEditorTabs::PreviewSettingsID(TEXT("PreviewSettings"));
//////////////////////////////////////////////////////


class STiledLevelPropertiesTabBody : public SSingleObjectDetailsPanel
{
public:
	SLATE_BEGIN_ARGS(STiledLevelPropertiesTabBody) {}
	SLATE_END_ARGS()

private:
	TWeakPtr<class FTiledLevelEditor> TiledLevelEditorPtr;

public:
	void Construct(const FArguments& InArgs, TSharedPtr<FTiledLevelEditor> InEditor)
	{
		TiledLevelEditorPtr = InEditor;
		SSingleObjectDetailsPanel::Construct(
			SSingleObjectDetailsPanel::FArguments().HostCommandList(InEditor->GetToolkitCommands()).HostTabManager(InEditor->GetTabManager()),
			true,
			true
		);
	}

	virtual UObject* GetObjectToObserve() const override
	{
		return TiledLevelEditorPtr.Pin()->GetTiledLevelBeingEdited();	
	}

	virtual TSharedRef<SWidget> PopulateSlot(TSharedRef<SWidget> PropertyEditorWidget) override
	{
		
		UE_LOG(LogTemp,Warning,TEXT("%s"),*PropertyEditorWidget->StaticWidgetClass().GetWidgetType().ToString());
		return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.FillHeight(1)
		[
			//SNew(STextBlock)
			PropertyEditorWidget
		];
	}
};

/////////////////////////////////////////////////////
FTiledLevelEditor::FTiledLevelEditor()
	: TiledLevelBeingEdited(nullptr)
{
}

FTiledLevelEditor::~FTiledLevelEditor()
{
}

void FTiledLevelEditor::InitTiledLevelEditor(const EToolkitMode::Type Mode,
	const TSharedPtr<IToolkitHost>& InitToolkitHost, class UTiledLevelAsset* InTiledLevel)
{
	GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->CloseOtherEditors(InTiledLevel, this);
 	TiledLevelBeingEdited = InTiledLevel;

	BindCommands();
    
	ViewportPtr = SNew(STiledLevelEditorViewport, SharedThis(this));
	ToolboxPtr = SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
        .Padding(0.f);
	
	const TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout("Standalone_TiledLevelEditor_Rel1")
	->AddArea
	(
		// Create a vertical area and spawn the toolbar
		FTabManager::NewPrimaryArea()->SetOrientation(Orient_Horizontal)
		->Split
		(
			FTabManager::NewStack()
			->SetSizeCoefficient(0.3f)
			->SetHideTabWell(true)
			->AddTab(FTiledLevelEditorTabs::ToolboxID, ETabState::OpenedTab)
		)
		->Split
		(
			FTabManager::NewStack()
			->SetSizeCoefficient(0.5f)
			->SetHideTabWell(true)
			->AddTab(FTiledLevelEditorTabs::ViewportID, ETabState::OpenedTab)
		)
		->Split
		(
			FTabManager::NewStack()
			->SetSizeCoefficient(0.2f)
			->SetHideTabWell(true)
			->AddTab(FTiledLevelEditorTabs::DetailsID, ETabState::OpenedTab)
			->AddTab(FTiledLevelEditorTabs::PreviewSettingsID,ETabState::OpenedTab)
		)
	);
	
 	FAssetEditorToolkit::InitAssetEditor(
 		Mode,
 		InitToolkitHost,
 		TiledLevelEditorAppName,
 		StandaloneDefaultLayout,
 		true,
 		true,
 		InTiledLevel
 	);

	ViewportPtr->ActivateEdMode();

	ExtendToolbar();
	RegenerateMenusAndToolbars();
}

void FTiledLevelEditor::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(FText::FromString("Tiled Level Editor"));
 	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);
	
 	InTabManager->RegisterTabSpawner(FTiledLevelEditorTabs::ToolboxID, FOnSpawnTab::CreateSP(this, &FTiledLevelEditor::SpawnTab_Toolbox))
 		.SetDisplayName(LOCTEXT("ToolboxTabLabel", "Toolbox"))
 		.SetGroup(WorkspaceMenuCategory.ToSharedRef())
 		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Modes"));
	
 	InTabManager->RegisterTabSpawner(FTiledLevelEditorTabs::ViewportID, FOnSpawnTab::CreateSP(this, &FTiledLevelEditor::SpawnTab_Viewport))
 		.SetDisplayName(LOCTEXT("ViewportTabLabel", "Viewport"))
 		.SetGroup(WorkspaceMenuCategory.ToSharedRef())
 		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Viewports"));
	
 	InTabManager->RegisterTabSpawner(FTiledLevelEditorTabs::DetailsID, FOnSpawnTab::CreateSP(this, &FTiledLevelEditor::SpawnTab_Details))
 		.SetDisplayName(LOCTEXT("PropertiesTabLabel", "Details"))
 		.SetGroup(WorkspaceMenuCategory.ToSharedRef())
 		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));
	
 	InTabManager->RegisterTabSpawner(FTiledLevelEditorTabs::PreviewSettingsID, FOnSpawnTab::CreateSP(this, &FTiledLevelEditor::SpawnTab_PreviewSettings))
 		.SetDisplayName(LOCTEXT("PropertiesTabLabel", "Preview Settings"))
 		.SetGroup(WorkspaceMenuCategory.ToSharedRef())
 		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));
	
}

void FTiledLevelEditor::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);
	
 	InTabManager->UnregisterTabSpawner(FTiledLevelEditorTabs::ToolboxID);
 	InTabManager->UnregisterTabSpawner(FTiledLevelEditorTabs::ViewportID);
 	InTabManager->UnregisterTabSpawner(FTiledLevelEditorTabs::DetailsID);
	InTabManager->UnregisterTabSpawner(FTiledLevelEditorTabs::PreviewSettingsID);
}

FName FTiledLevelEditor::GetToolkitFName() const
{
	return FName("Tiled Level Editor");
}

FText FTiledLevelEditor::GetBaseToolkitName() const
{
	return FText::FromString("Tiled Level Editor");
}

FText FTiledLevelEditor::GetToolkitName() const
{
	const bool bDirtyState = TiledLevelBeingEdited->GetOutermost()->IsDirty();
	return FText::Format(LOCTEXT("TiledItemSetEditorAppLabel", "{0}{1}"),
		FText::FromString(TiledLevelBeingEdited->GetName()),
		bDirtyState ? FText::FromString(TEXT("*")) : FText::GetEmpty());
}

FText FTiledLevelEditor::GetToolkitToolTipText() const
{
	return GetToolTipTextForObject(TiledLevelBeingEdited);
}

FString FTiledLevelEditor::GetWorldCentricTabPrefix() const
{
	return FString("TiledLevelEditor");
}

FLinearColor FTiledLevelEditor::GetWorldCentricTabColorScale() const
{
	return FColor::White;
}

void FTiledLevelEditor::OnToolkitHostingStarted(const TSharedRef<IToolkit>& Toolkit)
{
	TSharedPtr<SWidget> InlineContent = Toolkit->GetInlineContent();
	if (InlineContent.IsValid())
	{
		ToolboxPtr->SetContent(InlineContent.ToSharedRef());
	}
}

void FTiledLevelEditor::OnToolkitHostingFinished(const TSharedRef<IToolkit>& Toolkit)
{
	ToolboxPtr->SetContent(SNullWidget::NullWidget);
}

void FTiledLevelEditor::SaveAsset_Execute()
{
	FAssetEditorToolkit::SaveAsset_Execute();
	UpdateLevels();
}

void FTiledLevelEditor::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(TiledLevelBeingEdited);
}

void FTiledLevelEditor::BindCommands()
{
	const TSharedRef<FUICommandList>& CommandList = GetToolkitCommands();
	const FTiledLevelCommands& Commands = FTiledLevelCommands::Get();

	CommandList->MapAction(
		Commands.UpdateLevels,
		FExecuteAction::CreateSP(this, &FTiledLevelEditor::UpdateLevels),
		FCanExecuteAction::CreateSP(this, &FTiledLevelEditor::CanUpdateLevels)
	);
	CommandList->MapAction(
		Commands.MergeToStaticMesh,
		FExecuteAction::CreateSP(this, &FTiledLevelEditor::MergeToStaticMesh),
		FCanExecuteAction()
	);
}

void FTiledLevelEditor::ExtendToolbar()
{
	struct Local
	{
		static void FillToolbar(FToolBarBuilder& ToolbarBuilder)
		{
			ToolbarBuilder.BeginSection("Sync");
			{
				ToolbarBuilder.AddToolBarButton(FTiledLevelCommands::Get().UpdateLevels);
			}
			ToolbarBuilder.EndSection();
			ToolbarBuilder.BeginSection("Convert");
			{
				ToolbarBuilder.AddToolBarButton(FTiledLevelCommands::Get().MergeToStaticMesh);
			}
			ToolbarBuilder.EndSection();

		}
	};

	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);

	ToolbarExtender->AddToolBarExtension(
		"Asset",
		EExtensionHook::After,
		ToolkitCommands,
		FToolBarExtensionDelegate::CreateStatic(&Local::FillToolbar)
		);

	AddToolbarExtender(ToolbarExtender);
}

TSharedRef<SDockTab> FTiledLevelEditor::SpawnTab_Toolbox(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.Label(LOCTEXT("ToolboxTab_Title", "Toolbox"))
		[
			ToolboxPtr.ToSharedRef()
		];
}

TSharedRef<SDockTab> FTiledLevelEditor::SpawnTab_Viewport(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.Label(LOCTEXT("ViewportTab_Title", "Viewport"))
		[
			ViewportPtr.ToSharedRef()
		];
}

TSharedRef<SDockTab> FTiledLevelEditor::SpawnTab_Details(const FSpawnTabArgs& Args)
{
	TSharedPtr<FTiledLevelEditor> TiledLevelEditorPtr = SharedThis(this);
	
	return SNew(SDockTab)
		.Label(LOCTEXT("DetailsTab_Title", "Details"))
		[
			SNew(STiledLevelPropertiesTabBody, TiledLevelEditorPtr)
		];
}

TSharedRef<SDockTab> FTiledLevelEditor::SpawnTab_PreviewSettings(const FSpawnTabArgs& Args)
{
	TSharedPtr<FAdvancedPreviewScene> PreviewScene;
	TSharedPtr<SWidget> SettingsWidget = SNullWidget::NullWidget;
	if (ViewportPtr.IsValid())
	{
		PreviewScene = ViewportPtr->GetPreviewScene();
	}
	if (PreviewScene.IsValid())
	{
		TSharedPtr<SAdvancedPreviewDetailsTab> PreviewSettingsWidget = SNew(SAdvancedPreviewDetailsTab, PreviewScene.ToSharedRef());
		PreviewSettingsWidget->Refresh();
		SettingsWidget = PreviewSettingsWidget;
	}

	return SNew(SDockTab)
	.Label(LOCTEXT("PreviewSettings_Title", "Preview Settings"))
	[
		SettingsWidget.ToSharedRef()
	];
}

void FTiledLevelEditor::UpdateLevels()
{
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GLevelEditorModeTools().GetWorld(), ATiledLevel::StaticClass(), FoundActors);
	for (AActor* Actor : FoundActors)
	{
		if (ATiledLevel* Atl = Cast<ATiledLevel>(Actor))
		{
			if (Atl->GetAsset() == TiledLevelBeingEdited)
			{
				Atl->ResetAllInstance();
			}
		}
	}
}

bool FTiledLevelEditor::CanUpdateLevels() const
{
	if (!TiledLevelBeingEdited->GetOutermost()->IsDirty()) return false;
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GLevelEditorModeTools().GetWorld(), ATiledLevel::StaticClass(), FoundActors);
	for (AActor* Actor : FoundActors)
	{
		if (Cast<ATiledLevel>(Actor)->GetAsset() == TiledLevelBeingEdited )
		{
			return true;
		}
	}
	return false;
}

void FTiledLevelEditor::MergeToStaticMesh()
{
	FTiledLevelEditorUtility::MergeTiledLevelAsset(TiledLevelBeingEdited);
}
#undef LOCTEXT_NAMESPACE
