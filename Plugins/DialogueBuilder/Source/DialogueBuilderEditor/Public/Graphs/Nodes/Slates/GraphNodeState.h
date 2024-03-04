// Copyright 2022 Greg Shynkar. All Rights Reserved

#pragma once

#include "SGraphPin.h"
#include "SGraphNode.h"
#include "SCommentBubble.h"
#include "Widgets/Text/SMultiLineEditableText.h"
#include "Graphs/Nodes/StateNodeDB.h"


/////// SGraphNodePin
class SGraphNodePin : public SGraphPin
{
public:
	SLATE_BEGIN_ARGS(SGraphNodePin) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdGraphPin* InPin);
protected:
	// Begin SGraphPin interface
	virtual TSharedRef<SWidget>	GetDefaultValueWidget() override;
	virtual FSlateColor GetPinColor() const override;
	// End SGraphPin interface

	const FSlateBrush* GetPinBorder() const;
};

/////// SDialogueNodeIndex
class SDialogueNodeIndex : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDialogueNodeIndex) {}
	SLATE_ATTRIBUTE(FText, Text)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	FSlateColor GetColor() const;
};

/////// SGraphNodeState
class SGraphNodeState : public SGraphNode
{
public:
	SLATE_BEGIN_ARGS(SGraphNodeState) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UStateNodeDB* InNode);

	// Start SGraphNode interface
	virtual void UpdateGraphNode() override;
	virtual void CreatePinWidgets() override;
	virtual void AddPin(const TSharedRef<SGraphPin>& PinToAdd) override;
	// End of SGraphNode interface

	// Start SNodePanel interface
	virtual TArray<FOverlayWidgetInfo> GetOverlayWidgets(bool bSelected, const FVector2D& WidgetSize) const override;
	// End SNodePanel interface

protected:
	/** Get the text to display in the index overlay */
	FText GetIndexText() const;

	/** Get the tooltip for the index overlay */
	FText GetIndexTooltipText() const;

	/** Set Dialogue Text and Update it to Dialogue Data */
	void CustOnTextCommitted(const FText& InText, ETextCommit::Type CommitInfo);
	
	/** Set SpeakerName and Update it to Dialogue Data */
	void OnCommentTextCommitted(const FText& NewComment, ETextCommit::Type CommitInfo);
	
	/** Get SpeakerName from Dialogue Node */
	FString GetNodeComment() const;

	/** SpeakerName become visible after editing it inside details panel */
	bool IsEnabledByText() const;

	/** Start Appearance part */
	FSlateColor GetBorderBackgroundColor() const;
	virtual const FSlateBrush* GetNameIcon() const;
	bool IsHovered() const;
	/** End Appearance part */

	/** The widget we use to display the index of the node */
	TSharedPtr<SWidget> IndexOverlay;

	/** The widget we use to display the speaker name */
	TSharedPtr<SCommentBubble> CommentBubble;

	// Dialogue Node Pointer to update data;
	UStateNodeDB* StateNodeRef = nullptr;
};
