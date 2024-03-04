// Copyright 2023 PufStudio. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "AutoPaintRule.h"
#include "Brushes/SlateColorBrush.h"
#include "DragAndDrop/DecoratedDragDropOp.h"
#include "Widgets/SCompoundWidget.h"

class FTiledLevelEdMode;
class UAutoPaintRule;

class FAutoPaintItemViewData
{
public:
	FAutoPaintItemViewData(const FName& InItemName, UAutoPaintRule* InParentRule);
	FName ItemName;
	UAutoPaintRule* ParentRule;
	FAutoPaintItem* GetItem() const;
};

class SAutoPaintItem : public STableRow<TSharedPtr<FAutoPaintItemViewData>>
{
public:
	SLATE_BEGIN_ARGS(SAutoPaintItem) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedPtr<FAutoPaintItemViewData> InItem, const TSharedRef<STableViewBase>& InOwnerTableView);
	virtual FReply OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void OnDragEnter(FGeometry const& MyGeometry, FDragDropEvent const& DragDropEvent) override;
	virtual void OnDragLeave(FDragDropEvent const& DragDropEvent) override;
	virtual FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;

private:
	TSharedPtr<FAutoPaintItemViewData> MyData;
	TSharedPtr<FSlateColorBrush> BG;
	
};

namespace AutoPaintPaletteColumns
{
	static const FName ColumnID_Item("Item");
};


class FAutoPaintItemDragDropOp : public FDecoratedDragDropOp
{
public:
	DRAG_DROP_OPERATOR_TYPE(FAutoPaintItemDragDropOp, FDecoratedDragDropOp)

	void SetValidTarget(bool IsValidTarget);
	
	FName DragItemName;
	
	// FDragDropOperation interface
	virtual void OnDragged(const FDragDropEvent& DragDropEvent) override;
	// End of FDragDropOperation interface

	static TSharedRef<FAutoPaintItemDragDropOp> New(FName InItemName);
};



DECLARE_MULTICAST_DELEGATE_TwoParams(FOnPaintItemChanged, const FAutoPaintItem&, const FAutoPaintItem&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPaintItemRemoved, const FName&);


class TILEDLEVEL_API SAutoPaintItemPalette : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAutoPaintItemPalette) {};
	SLATE_END_ARGS();

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs, UAutoPaintRule* InRule, FTiledLevelEdMode* InEdMode);
	void UpdatePalette(bool bRebuildItem = false);
	void SelectItem(FName NewItemName);
	void ChangeRule(UAutoPaintRule* NewRule);
	FAutoPaintItem* GetSelectedAutoPaintItem() const;
	FOnPaintItemChanged OnPaintItemChanged;
	FOnPaintItemRemoved OnPaintItemRemoved;
	TSharedPtr<FUICommandList> ParentCommandList;

private:
	UAutoPaintRule* Rule = nullptr;
	FTiledLevelEdMode* EdMode = nullptr;
	TSharedPtr<SListView<TSharedPtr<FAutoPaintItemViewData>>> ItemsListWidget;
	TArray<TSharedPtr<FAutoPaintItemViewData>> Items;
	TSharedPtr<SWidget> ConstructItemContextMenu();
	void ClearItemInThisFloor();
	void ClearItemInAllFloors();
	TSharedRef<ITableRow> GenerateItemRow(TSharedPtr<FAutoPaintItemViewData> InItem, const TSharedRef<STableViewBase>& OwnerTable);
	void OnSelectionChanged(TSharedPtr<FAutoPaintItemViewData> AutoPaintItemViewData, ESelectInfo::Type Arg);
	bool IsAnyItemSelected() const;
};
