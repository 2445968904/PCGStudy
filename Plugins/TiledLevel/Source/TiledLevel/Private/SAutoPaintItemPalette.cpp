// Copyright 2023 PufStudio. All Rights Reserved.


#include "SAutoPaintItemPalette.h"

#include "AutoPaintRule.h"
#include "SlateOptMacros.h"
#include "TiledLevelCommands.h"
#include "TiledLevelEditorLog.h"
#include "Kismet/KismetMathLibrary.h"
#include "TiledLevelEditor/TiledLevelEdMode.h"

#define LOCTEXT_NAMESPACE "AutoPaintRuleEditor"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

FAutoPaintItemViewData::FAutoPaintItemViewData(const FName& InItemName, UAutoPaintRule* InParentRule)
	: ItemName(InItemName), ParentRule(InParentRule)
{
}

FAutoPaintItem* FAutoPaintItemViewData::GetItem() const
{
	if (ParentRule)
	{
		return ParentRule->Items.FindByKey(ItemName);
	}
	return nullptr;
}

void SAutoPaintItem::Construct(const FArguments& InArgs, TSharedPtr<FAutoPaintItemViewData> InItem, const TSharedRef<STableViewBase>& InOwnerTableView)
{
	MyData = InItem;
	FLinearColor TextColor =  UKismetMathLibrary::LinearColor_GetLuminance(MyData->GetItem()->PreviewColor) > 0.5f ? FLinearColor::Black : FLinearColor::White;
	BG = MakeShareable(new FSlateColorBrush(MyData->GetItem()->PreviewColor));
	STableRow<TSharedPtr<FAutoPaintItemViewData>>::Construct(
		STableRow<TSharedPtr<FAutoPaintItemViewData>>::FArguments()
		.ShowSelection(true)
		.Padding(6.f)
		[
			SNew(SBorder)
			.BorderImage(BG.Get())
			.Padding(12, 4, 0, 4)
			[
				SNew(STextBlock)
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 18))
				.Text(FText::FromName(MyData->GetItem()->ItemName))
				.ColorAndOpacity(FSlateColor(TextColor))	
			]
		]
		,InOwnerTableView);
}

FReply SAutoPaintItem::OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
	{
		return FReply::Handled().BeginDragDrop(FAutoPaintItemDragDropOp::New(MyData->GetItem()->ItemName));
	}
	return FReply::Unhandled();
}

void SAutoPaintItem::OnDragEnter(FGeometry const& MyGeometry, FDragDropEvent const& DragDropEvent)
{
	TSharedPtr<FAutoPaintItemDragDropOp> DragDropOp = DragDropEvent.GetOperationAs<FAutoPaintItemDragDropOp>();
	if (DragDropOp.IsValid())
	{
		if (DragDropOp->DragItemName != MyData->ItemName)
		{
			DragDropOp->SetValidTarget(true);
		}
		else
		{
			DragDropOp->SetValidTarget(false);
		}
	}
}

void SAutoPaintItem::OnDragLeave(FDragDropEvent const& DragDropEvent)
{
	TSharedPtr<FAutoPaintItemDragDropOp> DragDropOp = DragDropEvent.GetOperationAs<FAutoPaintItemDragDropOp>();
	if (DragDropOp.IsValid())
	{
		DragDropOp->SetValidTarget(false);
	}
}

FReply SAutoPaintItem::OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	TSharedPtr<FAutoPaintItemDragDropOp> DragDropOp = DragDropEvent.GetOperationAs<FAutoPaintItemDragDropOp>();
	if (DragDropOp.IsValid() && DragDropOp->DragItemName != MyData->ItemName)
	{
		const FScopedTransaction Transaction(LOCTEXT("AutoPaintRuleEditor_ReorderItem", "Reorder Item"));
		MyData->ParentRule->Modify();
		MyData->ParentRule->ReorderItem(DragDropOp->DragItemName, MyData->ItemName);
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

// Drag drop op

void FAutoPaintItemDragDropOp::SetValidTarget(bool IsValidTarget)
{
	if (IsValidTarget)
	{
		CurrentHoverText = LOCTEXT("PlaceItemHere", "Place Item Here");
		CurrentIconBrush = FAppStyle::GetBrush("Graph.ConnectorFeedback.OK");
	}
	else
	{
		CurrentHoverText = LOCTEXT("CannotPlaceHere", "Cannot Place Here");
		CurrentIconBrush = FAppStyle::GetBrush("Graph.ConnectorFeedback.Error");
	}
}

void FAutoPaintItemDragDropOp::OnDragged(const FDragDropEvent& DragDropEvent)
{
	MouseCursor = EMouseCursor::GrabHandClosed;
	FDecoratedDragDropOp::OnDragged(DragDropEvent);
}

TSharedRef<FAutoPaintItemDragDropOp> FAutoPaintItemDragDropOp::New(FName InItemName)
{
	TSharedRef<FAutoPaintItemDragDropOp> Operation = MakeShareable(new FAutoPaintItemDragDropOp);
	Operation->DragItemName = InItemName;
	Operation->SetValidTarget(false);
	Operation->SetupDefaults();
	Operation->Construct();
	return Operation;
}

// end of drag drop op

void SAutoPaintItemPalette::Construct(const FArguments& InArgs, UAutoPaintRule* InRule, FTiledLevelEdMode* InEdMode)
{
	Rule = InRule;
	EdMode = InEdMode;
	if (Rule)
	{
		Rule->RefreshAutoPalette_Delegate.BindSP(this, &SAutoPaintItemPalette::UpdatePalette, true);
	} 

	const TSharedPtr<SVerticalBox> Content = SNew(SVerticalBox)
	+ SVerticalBox::Slot()
	.FillHeight(1.f)
	[
		SAssignNew(ItemsListWidget, SListView<TSharedPtr<FAutoPaintItemViewData>>)
		.Visibility_Lambda([this](){ return IsValid(Rule)? EVisibility::Visible: EVisibility::Collapsed; })
		.ListItemsSource(&Items)
		.SelectionMode(ESelectionMode::Single)
		.OnGenerateRow(this, &SAutoPaintItemPalette::GenerateItemRow)
		.OnSelectionChanged(this, &SAutoPaintItemPalette::OnSelectionChanged)
		.OnContextMenuOpening(this, &SAutoPaintItemPalette::ConstructItemContextMenu)
		.HeaderRow
		(
			SNew(SHeaderRow)
			+SHeaderRow::Column(AutoPaintPaletteColumns::ColumnID_Item)
			.HeaderContent()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("AutoPaintItemPaletteTitle", "Auto Paint Item Palette"))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
			]
		)
	]
	
	+ SVerticalBox::Slot()
	[
		SNew(STextBlock)
		.Visibility_Lambda([this](){ return IsValid(Rule)? EVisibility::Collapsed : EVisibility::Visible; })
		.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
		.ColorAndOpacity(FLinearColor::Red)
		.Text(LOCTEXT("NoAutopaintRule_Label", "Auto paint rule is not set!"))
	];
	
	UpdatePalette(true);
	ChildSlot
	[
		Content.ToSharedRef()
	];
	
}

void SAutoPaintItemPalette::UpdatePalette(bool bRebuildItem)
{
	if (bRebuildItem && IsValid(Rule))
	{
		Items.Empty();
		for (auto item: Rule->Items)
		{
			Items.Add(MakeShareable(new FAutoPaintItemViewData(item.ItemName, Rule)));
		}
	}
	ItemsListWidget->RequestListRefresh();
	ItemsListWidget->ClearSelection();
}


void SAutoPaintItemPalette::SelectItem(FName NewItemName)
{
	for (auto Data : Items)
	{
		if (Data->ItemName == NewItemName)
		{
			ItemsListWidget->SetSelection(Data);
			return;
		}
	}
}

void SAutoPaintItemPalette::ChangeRule(UAutoPaintRule* NewRule)
{
	if (Rule)
		Rule->RefreshAutoPalette_Delegate.Unbind();
	Rule = NewRule;
	if (Rule)
	{
		Rule->RefreshAutoPalette_Delegate.BindSP(this, &SAutoPaintItemPalette::UpdatePalette, true);
		UpdatePalette(true);
	}
}

FAutoPaintItem* SAutoPaintItemPalette::GetSelectedAutoPaintItem() const
{
	if (IsAnyItemSelected())
	{
		return ItemsListWidget->GetSelectedItems()[0]->GetItem();
	}
	return nullptr;
}


TSharedPtr<SWidget> SAutoPaintItemPalette::ConstructItemContextMenu()
{
	const FTiledLevelCommands& TiledLevelCommands = FTiledLevelCommands::Get();
	
	FMenuBuilder MenuBuilder(true, ParentCommandList);
	MenuBuilder.BeginSection("AutoPaintPaletteItemOptions", LOCTEXT("AutoPaintPaletteItemOptions", "Options"));
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("ClearAction", "Clear in this floor"),
			LOCTEXT("ClearAction_Tooltip", "Clear auto paint for this item in this floor"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Delete"),
			FUIAction(FExecuteAction::CreateSP(this, &SAutoPaintItemPalette::ClearItemInThisFloor)),
			NAME_None
			);
		MenuBuilder.AddMenuEntry(
			LOCTEXT("ClearAllAction", "Clear across all floors"),
			LOCTEXT("ClearAllAction_Tooltip", "Clear auto paint for this item for all floors"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Delete"),
			FUIAction(FExecuteAction::CreateSP(this, &SAutoPaintItemPalette::ClearItemInAllFloors)),
			NAME_None
			);
	}
	MenuBuilder.EndSection();
	if (EdMode && EdMode->IsInRuleAssetEditor)
	{
		MenuBuilder.BeginSection("Create", LOCTEXT("CreateOptions", "Create"));
		{
			MenuBuilder.AddMenuEntry(TiledLevelCommands.CreateNewRuleItem,
				NAME_None,
				TAttribute<FText>(), 
				TAttribute<FText>(),
				FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Plus")
			);
		}
		MenuBuilder.EndSection();
	}
	return MenuBuilder.MakeWidget();
}

void SAutoPaintItemPalette::ClearItemInThisFloor()
{
	Rule->ClearItemDataInFloor.Execute(ItemsListWidget->GetSelectedItems()[0]->ItemName);
}

void SAutoPaintItemPalette::ClearItemInAllFloors()
{
	Rule->EmptyItemData.Execute(ItemsListWidget->GetSelectedItems()[0]->ItemName);
}

TSharedRef<ITableRow> SAutoPaintItemPalette::GenerateItemRow(TSharedPtr<FAutoPaintItemViewData> InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SAutoPaintItem, InItem, OwnerTable);
}


void SAutoPaintItemPalette::OnSelectionChanged(TSharedPtr<FAutoPaintItemViewData> AutoPaintItemViewData, ESelectInfo::Type Arg)
{
	if (IsAnyItemSelected())
	{
		Rule->ActiveItemName = ItemsListWidget->GetSelectedItems()[0]->ItemName;
		Rule->SelectionState = EAutoPaintEditorSelectionState::Item;
	}
	else
	{
		Rule->ActiveItemName = FName("Empty");
		Rule->SelectionState = EAutoPaintEditorSelectionState::None;
	}
}

bool SAutoPaintItemPalette::IsAnyItemSelected() const
{
	return ItemsListWidget->GetNumItemsSelected() > 0;
}


END_SLATE_FUNCTION_BUILD_OPTIMIZATION

#undef LOCTEXT_NAMESPACE