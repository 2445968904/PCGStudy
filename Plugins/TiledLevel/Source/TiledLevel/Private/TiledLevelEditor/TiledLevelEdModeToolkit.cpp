// Copyright 2022 PufStudio. All Rights Reserved.

#include "TiledLevelEdModeToolkit.h"

#include "AutoPaintRule.h"
#include "TiledLevelEdMode.h"
#include "EditorModeManager.h"
#include "SAutoPaintItemPalette.h"
#include "STiledPalette.h"
#include "TiledItemSet.h"
#include "TiledLevelAsset.h"
#include "TiledLevelCommands.h"
#include "TiledLevelEditorLog.h"
#include "TiledLevelItem.h"
#include "TiledLevelSettings.h"
#include "TiledLevelStyle.h"
#include "Toolkits/ToolkitManager.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Input/SSlider.h"
#include "WorkflowOrientedApp/SContentReference.h"


#define LOCTEXT_NAMESPACE "TiledLevel"

FTiledLevelEdModeToolkit::FTiledLevelEdModeToolkit(class FTiledLevelEdMode* InOwningMode)
{
	EdMode = InOwningMode;
}

void FTiledLevelEdModeToolkit::Init(const TSharedPtr<IToolkitHost>& InitToolkitHost)
{
	PaletteSwitcher = SNew(SWidgetSwitcher)
	+ SWidgetSwitcher::Slot()
	[
		SAssignNew(ItemPalette, STiledPalette, nullptr, EdMode, CurrentItemSetPtr.Get())
	]
	+ SWidgetSwitcher::Slot()
	[
		SAssignNew(AutoItemPalette, SAutoPaintItemPalette, CurrentAutoPaintRulePtr.Get(), EdMode)
	];
	
	
	ToolkitWidget = SNew(SVerticalBox)
	+ SVerticalBox::Slot()
	.AutoHeight()
	[
		SNew(SErrorText)
		.ErrorText(LOCTEXT("TileSizeError_Message", "Tile Size Not Confirmed yet!"))
		.ToolTipText(LOCTEXT("TileSizeError_Tooltip", "You must confirm desired tile size before editing this level!"))
		.Visibility(this, &FTiledLevelEdModeToolkit::GetErrorMessageVisibility)
	]
	// toolbar
	+ SVerticalBox::Slot()
	.AutoHeight()
	[
		BuildToolbar()
	]
	// load / change set
	+ SVerticalBox::Slot()
	.AutoHeight()
	[
		BuildAssetChooser()
	]
	// Set Palette
	+SVerticalBox::Slot()
	.FillHeight(1.0)
	.VAlign(VAlign_Fill)
	[
		PaletteSwitcher.ToSharedRef()
	];
	
	FModeToolkit::Init(InitToolkitHost);
}

FName FTiledLevelEdModeToolkit::GetToolkitFName() const
{
	return FName("TiledLevelEdMode");
}

FText FTiledLevelEdModeToolkit::GetBaseToolkitName() const
{
	return LOCTEXT("Toolkit", "TiledLevel");
}

class FEdMode* FTiledLevelEdModeToolkit::GetEditorMode() const
{
	return EdMode;
}

TSharedPtr<SWidget> FTiledLevelEdModeToolkit::GetInlineContent() const
{
	return ToolkitWidget;
}

// void FTiledLevelEdModeToolkit::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
// {
// 	FModeToolkit::RegisterTabSpawners(InTabManager);
// }
//
// void FTiledLevelEdModeToolkit::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
// {
// 	FModeToolkit::UnregisterTabSpawners(InTabManager);
// }

UObject* FTiledLevelEdModeToolkit::GetCurrentItemSet() const
{
	return CurrentItemSetPtr.Get();
}

UObject* FTiledLevelEdModeToolkit::GetCurrentRule() const
{
	return CurrentAutoPaintRulePtr.Get();
}

void FTiledLevelEdModeToolkit::LoadItemSet(UTiledItemSet* NewSet)
{
	CurrentItemSetPtr = NewSet;
	ItemPalette->ChangeSet(NewSet);
}

void FTiledLevelEdModeToolkit::LoadAutoPaintRule(UAutoPaintRule* NewRule)
{
	CurrentAutoPaintRulePtr = NewRule;
	if (NewRule)
	{
		AutoItemPalette->ChangeRule(NewRule);
	}
}


EVisibility FTiledLevelEdModeToolkit::GetErrorMessageVisibility() const
{
	return EdMode->IsReadyToEdit()? EVisibility::Collapsed : EVisibility::Visible;
}

TSharedRef<SWidget> FTiledLevelEdModeToolkit::BuildToolbar()
{
	const FTiledLevelCommands& Commands = FTiledLevelCommands::Get();
	FToolBarBuilder BrushToolbar(ToolkitCommands, FMultiBoxCustomization::None);
	{
		BrushToolbar.AddToolBarButton(Commands.SelectSelectTool);
		BrushToolbar.AddComboButton(
            FUIAction(),
            FOnGetContent::CreateSP(this, &FTiledLevelEdModeToolkit::MakeSelectionOptions),
            FText::GetEmpty(),
            LOCTEXT("SelectionToolOptionTooltip", "Configure select tool")
		);
		BrushToolbar.AddToolBarButton(Commands.SelectPaintTool);
		BrushToolbar.AddToolBarButton(Commands.SelectEraserTool);
		BrushToolbar.AddComboButton(
            FUIAction(),
            FOnGetContent::CreateSP(this, &FTiledLevelEdModeToolkit::MakeEraserOptions),
            FText::GetEmpty(),
            LOCTEXT("EraserOptionTooltip", "Configure eraser tool")
		);
		BrushToolbar.AddToolBarButton(Commands.SelectFillTool);
		BrushToolbar.AddComboButton(
            FUIAction(),
            FOnGetContent::CreateSP(this, &FTiledLevelEdModeToolkit::MakeFillOptions),
            FText::GetEmpty(),
            LOCTEXT("FillToolOptionTooltip", "Configure fill tool")
		);
		BrushToolbar.AddToolBarButton(Commands.SelectEyedropperTool);
		BrushToolbar.AddToolBarButton(Commands.ToggleAutoPaint);
		BrushToolbar.AddComboButton(
			FUIAction(
				FExecuteAction(),
				FCanExecuteAction::CreateLambda([this]()
				{
					return EdMode->ActiveAsset->ViewAsAutoPaint;
				})
				),
			FOnGetContent::CreateRaw(this, &FTiledLevelEdModeToolkit::MakeAutoPaintOptions ),
			FText::GetEmpty(),
			LOCTEXT("AutoPaintOptions_Tooltip", "Auto paint options")
			);
		BrushToolbar.AddSeparator();
		BrushToolbar.AddComboButton(
			FUIAction(),
			FOnGetContent::CreateRaw(this, &FTiledLevelEdModeToolkit::MakeStepOptions ),
			LOCTEXT("Step_Label", "Step"),
			LOCTEXT("Step_Tooltip", "Set step control options"),
			FSlateIcon(FTiledLevelStyle::GetStyleSetName(), "TiledLevel.StepControl")
			);
		BrushToolbar.AddToolBarButton(Commands.ToggleGrid);
	}
	FToolBarBuilder EditingToolbar(ToolkitCommands, FMultiBoxCustomization::None);
	{
		// TODO: dev it later... 
		// EditingToolbar.AddToolBarButton(Commands.ToggleEditViewMode);
		EditingToolbar.AddToolBarButton(Commands.RotateCW);
		EditingToolbar.AddToolBarButton(Commands.RotateCCW);
		EditingToolbar.AddToolBarButton(Commands.MirrorX);
		EditingToolbar.AddToolBarButton(Commands.MirrorY);
		EditingToolbar.AddToolBarButton(Commands.MirrorZ);
		EditingToolbar.AddToolBarButton(Commands.EditUpperFloor);
		EditingToolbar.AddToolBarButton(Commands.EditLowerFloor);
		EditingToolbar.AddToolBarButton(Commands.MultiMode);
		EditingToolbar.AddToolBarButton(Commands.Snap);
	}

	return
	SNew(SBorder)
	.BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
	.Padding(FMargin(6.f, 3.f))
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.HAlign(HAlign_Center)
		.AutoHeight()
		[
			BrushToolbar.MakeWidget()
		]
		+ SVerticalBox::Slot()
		.HAlign(HAlign_Center)
		.AutoHeight()
		[
			EditingToolbar.MakeWidget()
		]
	];
	
}

TSharedRef<SWidget> FTiledLevelEdModeToolkit::BuildAssetChooser()
{
	return SAssignNew(AssetChooserSwitcher, SWidgetSwitcher)
	+ SWidgetSwitcher::Slot() // Item Set
	[
		 SNew(SHorizontalBox)
		 +SHorizontalBox::Slot()
		 .FillWidth(1.0f)
		 .HAlign(HAlign_Right)
		 .VAlign(VAlign_Center)
		 .Padding(FMargin(0.0f, 0.0f, 4.0f, 0.0f))
		 [
			  SNew(STextBlock)
			  .Text(LOCTEXT("CurrentTiledItemSetToEditWith", "Active Tiled Item Set"))
		 ]
		 +SHorizontalBox::Slot()
		 .HAlign(HAlign_Right)
		 .AutoWidth()
		 [
			  SNew(SContentReference)
			  .WidthOverride(140.0f)
			  .IsEnabled_Lambda([this](){ return EdMode->IsReadyToEdit(); })
			  .AssetReference(this, &FTiledLevelEdModeToolkit::GetCurrentItemSet)
			  .OnSetReference(this, &FTiledLevelEdModeToolkit::OnChangeItemSet)
			  .AllowedClass(UTiledItemSet::StaticClass())
			  .OnShouldFilterAsset(this, &FTiledLevelEdModeToolkit::OnShouldFilterAsset)
			  .AllowSelectingNewAsset(true) // TODO: This will make loading so slow, but not set it will not find proper stuff??
			  .AllowClearingReference(false)
		 ]
	]
	+ SWidgetSwitcher::Slot() // Auto Paint Rule
	[
		 SNew(SHorizontalBox)
		 +SHorizontalBox::Slot()
		 .FillWidth(1.0f)
		 .HAlign(HAlign_Right)
		 .VAlign(VAlign_Center)
		 .Padding(FMargin(0.0f, 0.0f, 4.0f, 0.0f))
		 [
			  SNew(STextBlock)
			  .Text(LOCTEXT("CurrentAutoPaintRuleToUse", "Active Auto Paint Rule"))
		 ]
		 +SHorizontalBox::Slot()
		 .HAlign(HAlign_Right)
		 .AutoWidth()
		 [
			 SNew(SContentReference)
			 .WidthOverride(140.0f)
			 .IsEnabled_Lambda([this](){ return EdMode->IsReadyToEdit(); })
			 .AssetReference(this, &FTiledLevelEdModeToolkit::GetCurrentRule)
			 .OnSetReference(this, &FTiledLevelEdModeToolkit::OnChangeRule)
			 .AllowedClass(UAutoPaintRule::StaticClass())
			 .OnShouldFilterAsset(this, &FTiledLevelEdModeToolkit::OnShouldFilterRuleAsset)
			 .AllowSelectingNewAsset(true) // TODO: This will make loading so slow, but not set it will not find proper stuff??
			 .AllowClearingReference(false)
		 ]
	];
}

TOptional<int> FTiledLevelEdModeToolkit::GetFloorsNum() const
{
    return EdMode->ActiveAsset->TiledFloors.Num();
}

TSharedRef<SWidget> FTiledLevelEdModeToolkit::MakeSelectionOptions()
{
	FMenuBuilder MenuBuilder(true, nullptr);
	MenuBuilder.BeginSection("StepSize", LOCTEXT("StepSizeHeading", "Step Size"));
    {
        MenuBuilder.AddWidget(
            SNew(SNumericEntryBox<int>)
                .MinDesiredValueWidth(80.f)
                .MinValue(1)
                .MaxValue(this, &FTiledLevelEdModeToolkit::GetFloorsNum)
                .AllowSpin(true)
                .MinSliderValue(1)
                .MaxSliderValue(this, &FTiledLevelEdModeToolkit::GetFloorsNum)
                .ToolTipText(LOCTEXT("FloorsToSelectTooltip", "How many floors are selected?"))
                .Value(this, &FTiledLevelEdModeToolkit::GetSelectionSizeValue)
                .OnValueChanged(this, &FTiledLevelEdModeToolkit::OnSelectionSizeChanged)
                .OnValueCommitted(this, &FTiledLevelEdModeToolkit::OnSelectionSizeCommitted),
            LOCTEXT("SelectionFloorSizeLabel", "Floors to select"));
    	MenuBuilder.AddMenuEntry(
			 LOCTEXT("SelectAllFloorLabel", "Select all floors?"),
			 LOCTEXT("SelectAllFloorTooltip", "Should select all floors (below floors are incluided)"),
			 FSlateIcon(),
			 FUIAction(
				 FExecuteAction::CreateSP(this, &FTiledLevelEdModeToolkit::SetSelectionForAll),
				 FCanExecuteAction(),
				 FIsActionChecked::CreateSP(this, &FTiledLevelEdModeToolkit::GetSelectionForAll)
			 ),
			 NAME_None,
			 EUserInterfaceActionType::ToggleButton
		 );

    }
    return MenuBuilder.MakeWidget();
}

TOptional<int> FTiledLevelEdModeToolkit::GetSelectionSizeValue() const
{
    return EdMode->FloorSelectCount;
}

void FTiledLevelEdModeToolkit::OnSelectionSizeChanged(int NewSelectionSize) const
{
    EdMode->FloorSelectCount = NewSelectionSize;
}

void FTiledLevelEdModeToolkit::OnSelectionSizeCommitted(int NewSelectionSize, ETextCommit::Type CommitType) const
{
    EdMode->FloorSelectCount = NewSelectionSize;
}

void FTiledLevelEdModeToolkit::SetSelectionForAll() const
{
    EdMode->bSelectAllFloors = !EdMode->bSelectAllFloors;
}

bool FTiledLevelEdModeToolkit::GetSelectionForAll() const
{
    return EdMode->bSelectAllFloors;
}

TOptional<int> FTiledLevelEdModeToolkit::GetStepSizeValue() const
{
	return GetMutableDefault<UTiledLevelSettings>()->StepSize;
}

void FTiledLevelEdModeToolkit::OnStepSizeChanged(int InValue)
{
	GetMutableDefault<UTiledLevelSettings>()->StepSize = InValue;
	FIntPoint StartPoint = GetMutableDefault<UTiledLevelSettings>()->StartPoint;
	if (StartPoint.X >= InValue)
		GetMutableDefault<UTiledLevelSettings>()->StartPoint.X = InValue - 1;
	if (StartPoint.Y >= InValue)
		GetMutableDefault<UTiledLevelSettings>()->StartPoint.Y = InValue - 1;
}

void FTiledLevelEdModeToolkit::OnStepSizeCommitted(int InValue, ETextCommit::Type CommitType)
{
	GetMutableDefault<UTiledLevelSettings>()->StepSize = InValue;
	FIntPoint StartPoint = GetMutableDefault<UTiledLevelSettings>()->StartPoint;
	if (StartPoint.X >= InValue)
		GetMutableDefault<UTiledLevelSettings>()->StartPoint.X = InValue - 1;
	if (StartPoint.Y >= InValue)
		GetMutableDefault<UTiledLevelSettings>()->StartPoint.Y = InValue - 1;
}


TSharedRef<SWidget> FTiledLevelEdModeToolkit::MakeEraserOptions()
{
	TSharedRef<SWidget> X_Label = SNumericEntryBox<int>::BuildLabel(FText::FromString("X"), FLinearColor::White, SNumericEntryBox<int>::RedLabelBackgroundColor);
    TSharedRef<SWidget> Y_Label = SNumericEntryBox<int>::BuildLabel(FText::FromString("Y"), FLinearColor::White, SNumericEntryBox<int>::GreenLabelBackgroundColor);
    TSharedRef<SWidget> Z_Label = SNumericEntryBox<int>::BuildLabel(FText::FromString("Z"), FLinearColor::White, SNumericEntryBox<int>::BlueLabelBackgroundColor);

    FMenuBuilder MenuBuilder(true, nullptr);
    MenuBuilder.BeginSection("EraserSnapTarget", LOCTEXT("EraserSnapTargetHeading", "Eraser Target"));
    {
        MenuBuilder.AddMenuEntry(
            LOCTEXT("EraserTagetLabel_Any", "Any"),
            LOCTEXT("EraserTargetToolTip_Any", "Erase all items around brush"),
            FSlateIcon(),
            FUIAction(
                FExecuteAction::CreateSP(this, &FTiledLevelEdModeToolkit::SetEraserSnapTarget, EPlacedType::Any),
                FCanExecuteAction::CreateSP(this, &FTiledLevelEdModeToolkit::CanSetEraserSnapTarget, EPlacedType::Any),
                FIsActionChecked::CreateSP(this, &FTiledLevelEdModeToolkit::GetEraserSnapTargetChecked, EPlacedType::Any)
            ),
            NAME_None,
            EUserInterfaceActionType::RadioButton
        );
		MenuBuilder.AddMenuEntry(
             LOCTEXT("EraserTagetLabel_Block", "Block"),
             LOCTEXT("EraserTargetToolTip_Block", "Erase Block only"),
             FSlateIcon(),
             FUIAction(
                 FExecuteAction::CreateSP(this, &FTiledLevelEdModeToolkit::SetEraserSnapTarget, EPlacedType::Block),
                 FCanExecuteAction::CreateSP(this, &FTiledLevelEdModeToolkit::CanSetEraserSnapTarget, EPlacedType::Block),
                 FIsActionChecked::CreateSP(this, &FTiledLevelEdModeToolkit::GetEraserSnapTargetChecked, EPlacedType::Block)
             ),
             NAME_None,
             EUserInterfaceActionType::RadioButton
         );
        MenuBuilder.AddMenuEntry(
            LOCTEXT("EraserTagetLabel_Floor", "Floor"),
            LOCTEXT("EraserTargetToolTip_Floor", "Erase floor only"),
            FSlateIcon(),
            FUIAction(
                FExecuteAction::CreateSP(this, &FTiledLevelEdModeToolkit::SetEraserSnapTarget, EPlacedType::Floor),
                FCanExecuteAction::CreateSP(this, &FTiledLevelEdModeToolkit::CanSetEraserSnapTarget, EPlacedType::Floor),
                FIsActionChecked::CreateSP(this, &FTiledLevelEdModeToolkit::GetEraserSnapTargetChecked, EPlacedType::Floor)
            ),
            NAME_None,
            EUserInterfaceActionType::RadioButton
        );
        MenuBuilder.AddMenuEntry(
            LOCTEXT("EraserTagetLabel_Wall", "Wall"),
            LOCTEXT("EraserTargetToolTip_Wall", "Erase wall only"),
            FSlateIcon(),
            FUIAction(
                FExecuteAction::CreateSP(this, &FTiledLevelEdModeToolkit::SetEraserSnapTarget, EPlacedType::Wall),
                FCanExecuteAction::CreateSP(this, &FTiledLevelEdModeToolkit::CanSetEraserSnapTarget, EPlacedType::Wall),
                FIsActionChecked::CreateSP(this, &FTiledLevelEdModeToolkit::GetEraserSnapTargetChecked, EPlacedType::Wall)
            ),

            NAME_None,
            EUserInterfaceActionType::RadioButton
        );
        MenuBuilder.AddMenuEntry(
            LOCTEXT("EraserTagetLabel_Edge", "Edge"),
            LOCTEXT("EraserTargetToolTip_Edge", "Erase edge only"),
            FSlateIcon(),
            FUIAction(
                FExecuteAction::CreateSP(this, &FTiledLevelEdModeToolkit::SetEraserSnapTarget, EPlacedType::Edge),
                FCanExecuteAction::CreateSP(this, &FTiledLevelEdModeToolkit::CanSetEraserSnapTarget, EPlacedType::Edge),
                FIsActionChecked::CreateSP(this, &FTiledLevelEdModeToolkit::GetEraserSnapTargetChecked, EPlacedType::Edge)
            ),
            NAME_None,
            EUserInterfaceActionType::RadioButton
        );
        MenuBuilder.AddMenuEntry(
            LOCTEXT("EraserTagetLabel_Pillar", "Pillar"),
            LOCTEXT("EraserTargetToolTip_Pillar", "Erase pillar only"),
            FSlateIcon(),
            FUIAction(
                FExecuteAction::CreateSP(this, &FTiledLevelEdModeToolkit::SetEraserSnapTarget, EPlacedType::Pillar),
                FCanExecuteAction::CreateSP(this, &FTiledLevelEdModeToolkit::CanSetEraserSnapTarget, EPlacedType::Pillar),
                FIsActionChecked::CreateSP(this, &FTiledLevelEdModeToolkit::GetEraserSnapTargetChecked, EPlacedType::Pillar)
            ),

            NAME_None,
            EUserInterfaceActionType::RadioButton
        );
        MenuBuilder.AddMenuEntry(
            LOCTEXT("EraserTagetLabel_Point", "Point"),
            LOCTEXT("EraserTargetToolTip_Point", "Erase point only"),
            FSlateIcon(),
            FUIAction(
                FExecuteAction::CreateSP(this, &FTiledLevelEdModeToolkit::SetEraserSnapTarget, EPlacedType::Point),
                FCanExecuteAction::CreateSP(this, &FTiledLevelEdModeToolkit::CanSetEraserSnapTarget, EPlacedType::Point),
                FIsActionChecked::CreateSP(this, &FTiledLevelEdModeToolkit::GetEraserSnapTargetChecked, EPlacedType::Point)
            ),
            NAME_None,
            EUserInterfaceActionType::RadioButton
        );

        MenuBuilder.AddWidget(
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            [
                SNew(SNumericEntryBox<int>)
                .AllowSpin(true)
				.MinSliderValue(1)
				.MaxSliderValue(16)
				.Visibility(this, &FTiledLevelEdModeToolkit::GetEraserExtentInputVisibility, EAxis::X)
				.Value(this, &FTiledLevelEdModeToolkit::GetEraserExtentValue, EAxis::X)
				.OnValueChanged(this, &FTiledLevelEdModeToolkit::OnEraserExtentChanged, EAxis::X)
				.OnValueCommitted(this, &FTiledLevelEdModeToolkit::OnEraserExtentCommitted, EAxis::X)
				.Label()[X_Label]
            ]
        + SHorizontalBox::Slot()
            .AutoWidth()
            [
                SNew(SNumericEntryBox<int>)
                .AllowSpin(true)
				.MinSliderValue(1)
				.MaxSliderValue(16)
				.Visibility(this, &FTiledLevelEdModeToolkit::GetEraserExtentInputVisibility, EAxis::Y)
				.Value(this, &FTiledLevelEdModeToolkit::GetEraserExtentValue, EAxis::Y)
				.OnValueChanged(this, &FTiledLevelEdModeToolkit::OnEraserExtentChanged, EAxis::Y)
				.OnValueCommitted(this, &FTiledLevelEdModeToolkit::OnEraserExtentCommitted, EAxis::Y)
				.Label()[Y_Label]
            ]
        + SHorizontalBox::Slot()
            .AutoWidth()
            [
                SNew(SNumericEntryBox<int>)
                .AllowSpin(true)
				.MinSliderValue(1)
				.MaxSliderValue(16)
				.Visibility(this, &FTiledLevelEdModeToolkit::GetEraserExtentInputVisibility, EAxis::Z)
				.Value(this, &FTiledLevelEdModeToolkit::GetEraserExtentValue, EAxis::Z)
				.OnValueChanged(this, &FTiledLevelEdModeToolkit::OnEraserExtentChanged, EAxis::Z)
				.OnValueCommitted(this, &FTiledLevelEdModeToolkit::OnEraserExtentCommitted, EAxis::Z)
				.Label()[Z_Label]
            ]
        , LOCTEXT("EraserExtent", "Extent")
            , true
            );
    	MenuBuilder.AddSeparator();
		MenuBuilder.AddMenuEntry(
            LOCTEXT("ClearOrphanLabel", "Clear Orphan"),
            LOCTEXT("ClearOrphanToolTip", "Clear placements not in current item set (May happen when you change item set)"),
            FSlateIcon(),
            FUIAction(
                FExecuteAction::CreateSP(this, &FTiledLevelEdModeToolkit::ClearOrphanData),
                FCanExecuteAction::CreateSP(this, &FTiledLevelEdModeToolkit::GetOrphanDataExistence))
			);
    }
    MenuBuilder.EndSection();
    return MenuBuilder.MakeWidget();
}

void FTiledLevelEdModeToolkit::SetEraserSnapTarget(EPlacedType SnapTarget)
{
	EdMode->EraserTarget = SnapTarget;
	int M = FMath::Max(EdMode->EraserExtent.X, EdMode->EraserExtent.Y);
	switch (SnapTarget) {
		case EPlacedType::Wall:
			EdMode->EraserExtent.X = M;
			EdMode->EraserExtent.Y = M;
			break;
		case EPlacedType::Pillar:
			EdMode->EraserExtent.X = 1;
			EdMode->EraserExtent.Y = 1;
			break;
		case EPlacedType::Edge:
			EdMode->EraserExtent = FIntVector(M, M, 1);
			break;
		case EPlacedType::Point:
			EdMode->EraserExtent = FIntVector(1);
			break;
		default: ;
	}
	ItemPalette->UpdatePalette();
}

bool FTiledLevelEdModeToolkit::CanSetEraserSnapTarget(EPlacedType SnapTarget)
{
	if (!EdMode->ActiveAsset->GetItemSetAsset()) return false;
	if (SnapTarget == EPlacedType::Any) return true;
	TSet<EPlacedType> ErasingItemPlacedTypes;
	for (const UTiledLevelItem* Item : EdMode->ActiveAsset->GetItemSet())
	{
		if (Item->bIsEraserAllowed)
		{
			ErasingItemPlacedTypes.Add(Item->PlacedType);
		}
		if (ErasingItemPlacedTypes.Num() == 6) break;
	}
	switch (SnapTarget) {
	case EPlacedType::Block:
		return ErasingItemPlacedTypes.Contains(EPlacedType::Block);
	case EPlacedType::Floor:
		return ErasingItemPlacedTypes.Contains(EPlacedType::Floor);
	case EPlacedType::Wall:
		return ErasingItemPlacedTypes.Contains(EPlacedType::Wall);
	case EPlacedType::Pillar:
		return ErasingItemPlacedTypes.Contains(EPlacedType::Pillar);
	case EPlacedType::Edge:
		return ErasingItemPlacedTypes.Contains(EPlacedType::Edge);
	case EPlacedType::Point:
		return ErasingItemPlacedTypes.Contains(EPlacedType::Point);
	default:;
	}
	return false;
}

bool FTiledLevelEdModeToolkit::GetEraserSnapTargetChecked(EPlacedType SnapTarget)
{
	return EdMode->EraserTarget == SnapTarget;
}

EVisibility FTiledLevelEdModeToolkit::GetEraserExtentInputVisibility(EAxis::Type Axis) const
{
	const EPlacedType Target = EdMode->EraserTarget;
	if (Target == EPlacedType::Floor && Axis==EAxis::Z) return EVisibility::Collapsed;
	if (Target == EPlacedType::Wall && Axis== EAxis::Y) return EVisibility::Collapsed;
	if (Target == EPlacedType::Pillar && (Axis == EAxis::X || Axis == EAxis::Y)) return EVisibility::Collapsed;
	if (Target == EPlacedType::Edge && (Axis== EAxis::Y || Axis == EAxis::Z)) return EVisibility::Collapsed;
	if (Target == EPlacedType::Point) return EVisibility::Collapsed;
	return EVisibility::Visible;
}

TOptional<int> FTiledLevelEdModeToolkit::GetEraserExtentValue(EAxis::Type Axis) const
{
    const FIntVector Extent = EdMode->EraserExtent;
    switch (Axis) {
    case EAxis::X: return FMath::Clamp(Extent.X, 1, 16);
    case EAxis::Y: return FMath::Clamp(Extent.Y, 1, 16);
    case EAxis::Z: return FMath::Clamp(Extent.Z, 1, 16);
    default:;
    }
    return 1;
}

void FTiledLevelEdModeToolkit::OnEraserExtentChanged(int InValue, EAxis::Type Axis)
{
    if (InValue < 1) InValue = 1;
    if (InValue > 16) InValue = 16;
    switch (Axis)
    {
    case EAxis::X:
        EdMode->EraserExtent.X = InValue;
        break;
    case EAxis::Y:
    	EdMode->EraserExtent.Y = InValue;
        break;
    case EAxis::Z:
        EdMode->EraserExtent.Z = InValue;
        break;
    default:;
    }
}

void FTiledLevelEdModeToolkit::OnEraserExtentCommitted(int InValue, ETextCommit::Type CommitType, EAxis::Type Axis)
{
    if (InValue < 1) InValue = 1;
    if (InValue > 16) InValue = 16;
    switch (Axis)
    {
    case EAxis::X:
        EdMode->EraserExtent.X = InValue;
        break;
    case EAxis::Y:
        EdMode->EraserExtent.Y = InValue;
        break;
    case EAxis::Z:
        EdMode->EraserExtent.Z = InValue;
        break;
    default:;
    }
}

bool FTiledLevelEdModeToolkit::GetOrphanDataExistence() const
{
	if (EdMode && EdMode->ActiveAsset)
	{
		return EdMode->ActiveAsset->HasOrphanPlacement;
	}
	return false;
}

void FTiledLevelEdModeToolkit::ClearOrphanData()
{
	EdMode->ActiveAsset->ClearOrphan();
}

TSharedRef<SWidget> FTiledLevelEdModeToolkit::MakeFillOptions()
{
    FMenuBuilder MenuBuilder(false, nullptr);
    MenuBuilder.BeginSection("FillModeOption", LOCTEXT("FillModeHeading", "Fill Mode"));
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("FillModeLabel_Tile", "Tile"),
			LOCTEXT("FillModeTip_Tile", "Fill tile and floor items inside boundary"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &FTiledLevelEdModeToolkit::SetFillMode, true),
				FCanExecuteAction(),
				FIsActionChecked::CreateSP(this, &FTiledLevelEdModeToolkit::GetFillMode, true)
			),
			NAME_None,
			EUserInterfaceActionType::RadioButton
		);
    	MenuBuilder.AddMenuEntry(
			 LOCTEXT("FillModeLabel_Edge", "Edge"),
			 LOCTEXT("FillModeTip_Edge", "Fill edge and wall around continuous tiles"),
			 FSlateIcon(),
			 FUIAction(
			 	FExecuteAction::CreateSP(this, &FTiledLevelEdModeToolkit::SetFillMode, false),
				 FCanExecuteAction(),
				 FIsActionChecked::CreateSP(this, &FTiledLevelEdModeToolkit::GetFillMode, false)
			 ),
			 NAME_None,
			 EUserInterfaceActionType::RadioButton
		 );
	}
    MenuBuilder.EndSection();
    MenuBuilder.BeginSection("FillBoundaryOption", LOCTEXT("FillBoundaryOptionHeading", "Fill Boundary"));
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("BoundaryLabel_Tile", "Tile"),
			LOCTEXT("FillBoundaryTip_Tile", "Treat floor and block as fill boundary"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &FTiledLevelEdModeToolkit::SetBoundaryTarget, true),
				FCanExecuteAction::CreateSP(this, &FTiledLevelEdModeToolkit::CanSetBoundaryTarget),
				FIsActionChecked::CreateSP(this, &FTiledLevelEdModeToolkit::GetBoundaryTarget, true)
			),
			NAME_None,
			EUserInterfaceActionType::RadioButton
		);
    	MenuBuilder.AddMenuEntry(
			 LOCTEXT("FillBoundaryLabel_Edge", "Edge"),
			 LOCTEXT("FillBoundaryTip_Edge", "Treat wall and edge as fill boundary"),
			 FSlateIcon(),
			 FUIAction(
				 FExecuteAction::CreateSP(this, &FTiledLevelEdModeToolkit::SetBoundaryTarget, false),
				 FCanExecuteAction::CreateSP(this, &FTiledLevelEdModeToolkit::CanSetBoundaryTarget),
				 FIsActionChecked::CreateSP(this, &FTiledLevelEdModeToolkit::GetBoundaryTarget, false)
			 ),
			 NAME_None,
			 EUserInterfaceActionType::RadioButton
		 );
    	MenuBuilder.AddSeparator();
    	MenuBuilder.AddMenuEntry(
			 LOCTEXT("FillBoundaryLabel_NeedGround", "Need Ground"),
			 LOCTEXT("FillBoundaryTip_NeedGround", "Only fill places where belowground tiles exist"),
			 FSlateIcon(),
			 FUIAction(
				 FExecuteAction::CreateSP(this, &FTiledLevelEdModeToolkit::SetNeedGround),
				 FCanExecuteAction::CreateSP(this, &FTiledLevelEdModeToolkit::CanSetBoundaryTarget),
				 FIsActionChecked::CreateSP(this, &FTiledLevelEdModeToolkit::GetNeedGround)
			 ),
			 NAME_None,
			 EUserInterfaceActionType::ToggleButton
		 );
	}
    MenuBuilder.EndSection();
    return MenuBuilder.MakeWidget();
}

void FTiledLevelEdModeToolkit::SetFillMode(bool IsTile) const
{
	if (IsTile)
	{
		EdMode->IsFillTiles = true;
	}
	else
	{
		EdMode->IsFillTiles = false;
	}
	ItemPalette->UpdatePalette(false);
}

bool FTiledLevelEdModeToolkit::GetFillMode(bool IsTile) const
{
	return EdMode->IsFillTiles == IsTile;
}

void FTiledLevelEdModeToolkit::SetBoundaryTarget(bool IsTile) const
{
	if (IsTile)
	{
		EdMode->IsTileAsFillBoundary = true;
	}
	else
	{
		EdMode->IsTileAsFillBoundary = false;
	}
}

bool FTiledLevelEdModeToolkit::GetBoundaryTarget(bool IsTile) const
{
	return EdMode->IsTileAsFillBoundary == IsTile;
}

bool FTiledLevelEdModeToolkit::CanSetBoundaryTarget() const
{
	return EdMode->IsFillTiles;
}

void FTiledLevelEdModeToolkit::SetNeedGround()
{
	EdMode->NeedGround = !EdMode->NeedGround;
}

bool FTiledLevelEdModeToolkit::GetNeedGround() const
{
	return EdMode->NeedGround;
}

TSharedRef<SWidget> FTiledLevelEdModeToolkit::MakeStepOptions()
{
	TSharedRef<SWidget> X_Label = SNumericEntryBox<int>::BuildLabel(FText::FromString("X"), FLinearColor::White, SNumericEntryBox<int>::RedLabelBackgroundColor);
    TSharedRef<SWidget> Y_Label = SNumericEntryBox<int>::BuildLabel(FText::FromString("Y"), FLinearColor::White, SNumericEntryBox<int>::GreenLabelBackgroundColor);
	
	FMenuBuilder MenuBuilder(true, nullptr);
	MenuBuilder.BeginSection("StepSize", LOCTEXT("StepSizeHeading", "Step Size"));
	{
		MenuBuilder.AddWidget(
			SNew(SNumericEntryBox<int>)
				.MinValue(1)
				.MaxValue(16)
				.AllowSpin(true)
				.MinSliderValue(1)
				.MaxSliderValue(16)
				.Value(this, &FTiledLevelEdModeToolkit::GetStepSizeValue)
				.OnValueChanged(this, &FTiledLevelEdModeToolkit::OnStepSizeChanged)
				.OnValueCommitted(this, &FTiledLevelEdModeToolkit::OnStepSizeCommitted),
				LOCTEXT("StepSizeLabel", "Step Size"));
		MenuBuilder.AddWidget(
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SNumericEntryBox<int>)
				.MinValue(0)
				.MaxValue(this, &FTiledLevelEdModeToolkit::GetMaxStartPoint)
				.AllowSpin(true)
				.MinSliderValue(0)
				.MaxSliderValue(this, &FTiledLevelEdModeToolkit::GetMaxStartPoint)
				.Value(this, &FTiledLevelEdModeToolkit::GetStartPoint, true)
				.OnValueChanged(this, &FTiledLevelEdModeToolkit::OnStartPointChanged, true)
				.OnValueCommitted(this, &FTiledLevelEdModeToolkit::OnStartPointCommitted, true)
				.Label()[X_Label]
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SNumericEntryBox<int>)
				.MinValue(0)
				.MaxValue(this, &FTiledLevelEdModeToolkit::GetMaxStartPoint)
				.AllowSpin(true)
				.MinSliderValue(0)
				.MaxSliderValue(this, &FTiledLevelEdModeToolkit::GetMaxStartPoint)
				.Value(this, &FTiledLevelEdModeToolkit::GetStartPoint, false)
				.OnValueChanged(this, &FTiledLevelEdModeToolkit::OnStartPointChanged, false)
				.OnValueCommitted(this, &FTiledLevelEdModeToolkit::OnStartPointCommitted, false)
				.Label()[Y_Label]
			]
			,
			LOCTEXT("StartPointLabel", "Start Point"));
		MenuBuilder.AddMenuEntry(
            LOCTEXT("ResetStepLabel", "Reset Step"),
            LOCTEXT("RestStepTooltip", "Reset step control settings"),
            FSlateIcon(),
            FUIAction(
                FExecuteAction::CreateLambda([]()
                {
                	GetMutableDefault<UTiledLevelSettings>()->StepSize = 1;
                	GetMutableDefault<UTiledLevelSettings>()->StartPoint = FIntPoint(0);
                }),
                FCanExecuteAction::CreateLambda([]()
                {
                	return GetMutableDefault<UTiledLevelSettings>()->StepSize != 1;
                })
            ),
            NAME_None,
            EUserInterfaceActionType::Button
			);
	}
	MenuBuilder.EndSection();

	return  MenuBuilder.MakeWidget();
}


TOptional<int> FTiledLevelEdModeToolkit::GetMaxStartPoint() const
{
	return GetMutableDefault<UTiledLevelSettings>()->StepSize - 1;
}

TOptional<int> FTiledLevelEdModeToolkit::GetStartPoint(bool IsX) const
{
	FIntPoint StartPoint = GetMutableDefault<UTiledLevelSettings>()->StartPoint;
	return IsX? StartPoint.X : StartPoint.Y;
}

void FTiledLevelEdModeToolkit::OnStartPointChanged(int InValue, bool IsX)
{
	IsX?
		GetMutableDefault<UTiledLevelSettings>()->StartPoint.X = InValue :
		GetMutableDefault<UTiledLevelSettings>()->StartPoint.Y = InValue ;
}

void FTiledLevelEdModeToolkit::OnStartPointCommitted(int InValue, ETextCommit::Type CommitType, bool IsX)
{
	IsX?
		GetMutableDefault<UTiledLevelSettings>()->StartPoint.X = InValue :
		GetMutableDefault<UTiledLevelSettings>()->StartPoint.Y = InValue ;
}

TSharedRef<SWidget> FTiledLevelEdModeToolkit::MakeAutoPaintOptions()
{
	const FTiledLevelCommands& Commands = FTiledLevelCommands::Get();
    FMenuBuilder MenuBuilder(true, GetToolkitCommands());
	MenuBuilder.BeginSection("AutoPaintViewOptions", LOCTEXT("AutoPaintViewOption_Label", "View options"));
    {
		MenuBuilder.AddMenuEntry(Commands.ToggleAutoPaintItemHint);
    	MenuBuilder.AddWidget(SNew(SSlider)
    		.ToolTipText(LOCTEXT("AutoPaintHintOpacitySlider_Tooltip", "adjust opacity"))
    		.MaxValue(1.0f)
    		.MinValue(0.f)
    		.Value(this, &FTiledLevelEdModeToolkit::GetAutoPaintHintOpacity)
    		.OnValueChanged(this, &FTiledLevelEdModeToolkit::SetAutoPaintHintOpacity)
    		, LOCTEXT("HintOpacity_Label", "Hint opacity"));
  //   	MenuBuilder.AddMenuEntry(
		// 	 LOCTEXT("AutoPaintOptionLabel_ShowNormalItems", "Show normal items"),
		// 	 LOCTEXT("AutoPaintOptionTip_ShowNormalItems", "Show the items from normal paint (it's independent from auto paint)"),
		// 	 FSlateIcon(),
		// 	 FUIAction(
		// 		 FExecuteAction::CreateSP(this, &FTiledLevelEdModeToolkit::SetShowNormalPaintInAutoPaint),
		// 		 FCanExecuteAction(),
		// 		 FIsActionChecked::CreateSP(this, &FTiledLevelEdModeToolkit::GetShowNormalPaintInAutoPaint)
		// 	 ),
		// 	 NAME_None,
		// 	 EUserInterfaceActionType::ToggleButton
		// );
    }
	MenuBuilder.EndSection();
	MenuBuilder.BeginSection("AutoPaintUtilityOptions", LOCTEXT("AutoPaintUtilityOption_Label", "Utility options"));
	{
    	// TODO: regenerate button... due to random pick... a button to show different random result is required...
    	MenuBuilder.AddMenuEntry(Commands.RespawnAutoPaintInstances);
		// TODO: block the use of random seed for now...
		// MenuBuilder.AddMenuEntry(
		// 	LOCTEXT("AutoPaintoptionLabel_UseRandomSeed", "Use random seed?"),
		// 	LOCTEXT("AutoPaintoptionTip_UseRandomSeed", "Do you want to have fixed random result?"),
		// 	FSlateIcon(),
		// 	FUIAction(
		// 		FExecuteAction::CreateLambda([this](){ EdMode->ToggleAutoPaintRandomSeed(); }),
		// 		FCanExecuteAction(),
		// 		FIsActionChecked::CreateLambda([this](){ return EdMode->GetAutoPaintIsEnableRandomSeed(); })
		// 		),
		// 	NAME_None,
		// 	EUserInterfaceActionType::ToggleButton
		// 	);
		// MenuBuilder.AddWidget(
		// 	SNew(SSpinBox<int>)
		// 	.IsEnabled_Lambda([this](){return EdMode->GetAutoPaintIsEnableRandomSeed(); })
		// 	.Value_Lambda([this](){ return EdMode->GetAutoPaintRandomSeed(); })
		// 	.OnValueChanged_Lambda([this](int NewValue){ EdMode->SetAutoPaintRandomSeed(NewValue); })
		// 	.OnValueCommitted_Lambda([this](int NewValue, ETextCommit::Type CommitType){ EdMode->SetAutoPaintRandomSeed(NewValue); })
		// 	,LOCTEXT("AutoPaintRandomSeed_Label", "Random Seed")
		// 	),
		MenuBuilder.AddMenuEntry(
			 LOCTEXT("AutoPaintOptionLabel_ConvertAutoPaint", "Convert auto paint to normal"),
			 LOCTEXT("AutoPaintOptionTip_ConvertAutoPaint", "Convert auto paint result to normal instances, then you can edit it manually"),
			 FSlateIcon(),
			 FUIAction(
				 FExecuteAction::CreateSP(this, &FTiledLevelEdModeToolkit::ConvertAutoPaintToNormal)
			 ),
			 NAME_None,
			 EUserInterfaceActionType::Button
		 );

	}
	MenuBuilder.EndSection();
	return MenuBuilder.MakeWidget();
}

float FTiledLevelEdModeToolkit::GetAutoPaintHintOpacity() const
{
	if (!EdMode->ActiveAsset) return 1.f;
	if (!EdMode->ActiveAsset->GetAutoPaintRule()) return 1.f;
	return EdMode->ActiveAsset->GetAutoPaintRule()->HintOpacity;
}

void FTiledLevelEdModeToolkit::SetAutoPaintHintOpacity(float NewOpacity)
{
	if (!EdMode->ActiveAsset) return;
	if (!EdMode->ActiveAsset->GetAutoPaintRule()) return;
	EdMode->ActiveAsset->GetAutoPaintRule()->HintOpacity = NewOpacity;
	EdMode->UpdateAutoPaintHintOpacity();
}

void FTiledLevelEdModeToolkit::ConvertAutoPaintToNormal() const
{
	EdMode->ConvertAutoPaintToNormal();
}

void FTiledLevelEdModeToolkit::OnChangeItemSet(UObject* NewAsset)
{
	if (UTiledItemSet* NewSet = Cast<UTiledItemSet>(NewAsset))
	{
		UTiledItemSet* DupSet = NewSet->CloneItemSet(EdMode->ActiveAsset);
		CurrentItemSetPtr = DupSet;
		if (ItemPalette.IsValid())
		{
			FScopedTransaction Transaction(LOCTEXT("ChangeItemSet", "ChangeItemSet"));
			if (UAutoPaintRule* RuleObj = EdMode->ActiveAsset->GetAutoPaintRule())
			{
				RuleObj->Modify();
			}
			EdMode->ActiveAsset->Modify();
			ItemPalette->ChangeSet(DupSet);
			EdMode->ActiveAsset->SetActiveItemSet(DupSet);
		}
	}
}

bool FTiledLevelEdModeToolkit::OnShouldFilterAsset(const FAssetData& AssetData)
{
	if (UTiledItemSet* ItemSet = Cast<UTiledItemSet>(AssetData.GetAsset()))
	{
		const FVector TestTileSize =  ItemSet->GetTileSize();
		const FVector AssetTileSize = EdMode->ActiveAsset->GetTileSize();
		const bool Matched = FMath::IsNearlyEqual(TestTileSize.X, AssetTileSize.X, 2) && FMath::IsNearlyEqual(TestTileSize.Y, AssetTileSize.Y, 2)
			&& FMath::IsNearlyEqual(TestTileSize.Z, AssetTileSize.Z, 2);
		return !Matched;
	}
	return true;
}

void FTiledLevelEdModeToolkit::OnChangeRule(UObject* Object)
{
	if (UAutoPaintRule* NewRule = Cast<UAutoPaintRule>(Object) )
	{
		if (CurrentAutoPaintRulePtr.Get() != NewRule && AutoItemPalette.IsValid())
		{
			CurrentAutoPaintRulePtr = NewRule;
			FScopedTransaction Transaction(LOCTEXT("ChangeAutoPaintRule", "Change auto paint rule"));
			EdMode->ActiveAsset->Modify();
			AutoItemPalette->ChangeRule(NewRule);
			EdMode->ActiveAsset->SetActiveAutoPaintRule(NewRule);
		}
	}
}

bool FTiledLevelEdModeToolkit::OnShouldFilterRuleAsset(const FAssetData& AssetData) const
{
	if (UAutoPaintRule* Rule = Cast<UAutoPaintRule>(AssetData.GetAsset()))
	{
		return Rule->SourceItemSet->GetTileSize() != CurrentItemSetPtr->GetTileSize();
	}
	return false;
}


#undef LOCTEXT_NAMESPACE
