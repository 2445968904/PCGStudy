// Copyright 2023 PufStudio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AutoPaintRule.h"
#include "DragAndDrop/DecoratedDragDropOp.h"
#include "Widgets/SCompoundWidget.h"

class UAutoPaintItemRule;
class SAutoPaintRuleDetailsEditor;
class SUniformGridPanel;

class FRuleDetailViewData
{
public:
	FRuleDetailViewData(UAutoPaintRule* InRule, int InGroupIdx, int InRuleIdx);
	UAutoPaintItemRule* GetItemRule() const;
	FAutoPaintRuleGroup* GetGroup() const;
	bool IsGroup() const;
	int GroupIdx;
	int RuleIdx = INDEX_NONE;
	UAutoPaintRule* Rule;

	bool operator==(const FRuleDetailViewData Other) const
	{
		return Other.Rule == Rule && Other.GroupIdx == GroupIdx && Other.RuleIdx == RuleIdx;
	}
};

class FAutoPaintRuleDragDropOp : public FDecoratedDragDropOp
{
public:
	DRAG_DROP_OPERATOR_TYPE(FAutoPaintRuleDragDropOp, FDecoratedDragDropOp)

	void SetValidTarget(bool IsValidTarget);

	TSharedPtr<FRuleDetailViewData> Data;
	
	// FDragDropOperation interface
	virtual void OnDragged(const FDragDropEvent& DragDropEvent) override;
	// End of FDragDropOperation interface

	static TSharedRef<FAutoPaintRuleDragDropOp> New(const TSharedPtr<FRuleDetailViewData>& InData);
	
};

class SAdjacencyRule : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAdjacencyRule) {}
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs, const TSharedPtr<FRuleDetailViewData>& InData, TSharedPtr<SAutoPaintRuleDetailsEditor> InParentEditor);
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	void Update();
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	TSharedPtr<SAutoPaintRuleDetailsEditor> ParentEditor;
	TSharedPtr<FRuleDetailViewData> Data;
	TSharedPtr<FSlateBrush> Brush_SpawnedItemAreaHint;
	TSharedPtr<SUniformGridPanel> ButtonGrids;
	const float ButtonSize = 24.f;
	int ViewFloor = 0;
	int NumGrids = 1;
	bool ShouldShowSpawnedAreaHint = true;
	FSlateColor GetAdjRuleColor(FIntVector AdjPoint) const;
	const FSlateBrush* GetButtonImage(FIntVector AdjPoint) const;
	FText GetButtonTooltip(FIntVector AdjPoint) const;
	FSlateColor GetButtonImageColor(FIntVector AdjPoint) const;
	FIntVector HoveredGridPoint{-100};
	FButtonStyle ButtonStyle;
	void OnGridButtonHovered(FIntVector AdjPoint);
	void OnGridButtonUnHovered();
	FReply TogglePositiveItem();
	void ToggleNegativeItem();
	void ToggleEmptyItem();

};

class SImpactFloorSelect : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SImpactFloorSelect) {}
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs, const TSharedPtr<FRuleDetailViewData>& InData,
		const TSharedPtr<SAdjacencyRule>& InControllingWidget);
	void Update();

private:
	TSharedPtr<FRuleDetailViewData> Data;
	TSharedPtr<SAdjacencyRule> ControllingWidget;
	TSharedPtr<SVerticalBox> FloorsWidget;
	TSharedPtr<SVerticalBox> Content;
	void UpdateFloorsWidget();
};

class SSpawnRuleWidget : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SSpawnRuleWidget) {}
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs, const TSharedPtr<FRuleDetailViewData>& InData,
		const TSharedPtr<SAdjacencyRule>& InAdjacencyRuleWidget);

private:
	TSharedPtr<SVerticalBox> TooltipContent;
	TSharedPtr<SUniformGridPanel> ToSpawnItemsPreview;
	void UpdateTooltipContent() const;
	void UpdateToSpawnItemsPreview() const;
	TSharedPtr<FRuleDetailViewData> Data;
	TSharedPtr<FAssetThumbnailPool> ThumbnailPool;
	FSlateBrush Brush;
	FReply OnEditSpawnDetailClicked();
	TSharedPtr<FStructOnScope> TargetedStructData;
	TSharedPtr<SAdjacencyRule> AdjacencyRuleWidget;
};

class SRuleRow : public STableRow<TSharedPtr<FRuleDetailViewData>>
{
	SLATE_BEGIN_ARGS(SRuleRow) {}
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs, TSharedPtr<FRuleDetailViewData> InItem, TSharedPtr<SAutoPaintRuleDetailsEditor> InParentEditor,
		const TSharedRef<STableViewBase>& InOwnerTableView);
	virtual FReply OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void OnDragEnter(FGeometry const& MyGeometry, FDragDropEvent const& DragDropEvent) override;
	virtual void OnDragLeave(FDragDropEvent const& DragDropEvent) override;
	virtual FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
	
private:
	TSharedPtr<FRuleDetailViewData> Data;
	const FVector2d BtnSize = {24.f, 24.f};
	FButtonStyle ButtonStyle;
	FSlateBrush DisabledBrush;
	FSlateBrush DropHintBrush;
	bool ShouldShowDropHint = false;
	TSharedRef<SWidget> MakeGroupWidget();
	ECheckBoxState IsGroupEnabled() const;
	void OnGroupEnabledChanged(ECheckBoxState CheckBoxState);
	FReply OnAddRuleClicked();
	FReply OnEditStepClicked();
	EVisibility GetVisibilityByEnabledState(bool bVisibleWhenEnabled) const;
	TSharedRef<SWidget> MakeRuleDetailWidget();
	TSharedPtr<SAutoPaintRuleDetailsEditor> ParentEditor;
	TSharedPtr<SAdjacencyRule> AdjacencyRuleWidget;
	TSharedPtr<SImpactFloorSelect> FloorSelectWidget;
	TSharedPtr<SSpawnRuleWidget> TargetItemsSelectWidget;
};

class TILEDLEVEL_API SAutoPaintRuleDetailsEditor : public SCompoundWidget, public FEditorUndoClient, public FNotifyHook
{
public:
	SLATE_BEGIN_ARGS(SAutoPaintRuleDetailsEditor){}
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs, UAutoPaintRule* InRule, TSharedPtr<class FUICommandList> InParentCommandList);
	void UpdateEditor();
	UAutoPaintItemRule* GetSelectedAutoPaintRule() const;
	FAutoPaintRuleGroup* GetSelectedAutoPaintGroup() const;
	void GetSelectedItemRuleInfo(FName& ParentGroupName, int& RuleIdxInGroup) const;
	TArray<FName> GetOpenedGroups();
	void MakeTreeItemExpand(TSharedPtr<FRuleDetailViewData> Data);

private:
	bool IsAnySelected() const;
	UAutoPaintRule* Rule = nullptr;
	TSharedPtr<FUICommandList> ParentCommandList;
	TArray<TSharedPtr<FRuleDetailViewData>> ViewData;
	TSharedPtr<STreeView<TSharedPtr<FRuleDetailViewData>>> RuleTreeWidget;
	TSharedRef<ITableRow> GenerateItemRow(TSharedPtr<FRuleDetailViewData> InData, const TSharedRef<STableViewBase>& OwnerTable);
	TSharedPtr<SWidget> ConstructContextMenu() const;
	void OnGetChildren(TSharedPtr<FRuleDetailViewData> InData, TArray<TSharedPtr<FRuleDetailViewData>>& OutData);
	void OnSelectionChanged(TSharedPtr<FRuleDetailViewData> NewSelection, ESelectInfo::Type Arg);
	FReply OnAddGroupClicked();
	void OnAddGroupConfirmed();
	FText GetAddNewGroupErrorMessage() const;
	TSharedRef<SWidget> MakeAddGroupModal();
	FText CandidateNewGroupName;
	FText NewGroupNameErrorMessage;
	TSharedPtr<FStructOnScope> TargetedStructData;
};
