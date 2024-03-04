// Copyright 2022 Greg Shynkar. All Rights Reserved

#pragma once

#include "InputCoreTypes.h"
#include "Input/Events.h"
#include "SGraphPanel.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "TutorialMetaData.h"
#include "Graphs/Nodes/CommentNodeDB.h"
#include "SGraphNodeResizable.h"
#include "SCommentBubble.h"

class SCommentBubble;

class SGraphNodeCommentDB : public SGraphNodeResizable
{
public:
	SLATE_BEGIN_ARGS(SGraphNodeCommentDB) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UCommentNodeDB* InNode);

	//~ Begin SWidget Interface
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	virtual FReply OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
	virtual FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;


protected:
	//~ Begin SGraphNode Interface
	virtual void UpdateGraphNode() override;

	//~ Begin SNodePanel::SNode Interface
	virtual FString GetNodeComment() const override;
	virtual void GetOverlayBrushes(bool bSelected, const FVector2D WidgetSize, TArray<FOverlayBrushInfo>& Brushes) const override;
	virtual int32 GetSortDepth() const override;

	//~ Begin SPanel Interface
	virtual FVector2D ComputeDesiredSize(float) const override;
	//~ End SPanel Interface

	//~ Begin SGraphNodeResizable Interface
	virtual float GetTitleBarHeight() const override;
	virtual FVector2D GetNodeMaximumSize() const override;

	/** Set Dialogue Text and Update it to Dialogue Data */
	void CustOnTextCommitted(const FText& InText, ETextCommit::Type CommitInfo);
	
private:

	/** @return the color to tint the comment body */
	FSlateColor GetCommentBodyColor() const;

	/** @return the color to tint the title bar */
	FSlateColor GetCommentTitleBarColor() const;

	/** @return the color to tint the comment bubble */
	FSlateColor GetCommentBubbleColor() const;

	/** Returns the width to wrap the text of the comment at */
	float GetWrapAt() const;

	/** The comment bubble widget (used when zoomed out) */
	TSharedPtr<SCommentBubble> CommentBubble;

	/** Was the bubble desired to be visible last frame? */
	 bool bCachedBubbleVisibility;

	/** the title bar, needed to obtain it's height */
	TSharedPtr<SBorder> TitleBar;

protected:
	/** The current selection state of the comment */
	bool bIsSelected;

	/** cached comment title */
	FString CachedCommentTitle;

private:
	/** cached comment title */
	int32 CachedWidth;

	/** cached font size */
	int32 CachedFontSize;

	/** Local copy of the comment style */
	FInlineEditableTextBlockStyle CommentStyle;

	// Dialogue Node Pointer to update data;
	UCommentNodeDB* CommentNodeRef = nullptr;
};