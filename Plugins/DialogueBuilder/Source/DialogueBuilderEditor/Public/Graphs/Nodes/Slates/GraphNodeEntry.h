// Copyright 2022 Greg Shynkar. All Rights Reserved

#pragma once

#include "SGraphPin.h"
#include "SGraphNode.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "Graphs/Nodes/EntryNodeDB.h"


class SGraphEntryPin : public SGraphPin
{
public:
	SLATE_BEGIN_ARGS(SGraphEntryPin) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdGraphPin* InPin);
protected:
	// Begin SGraphPin interface
	virtual TSharedRef<SWidget>	GetDefaultValueWidget() override;
	virtual FSlateColor GetPinColor() const override;
	// End SGraphPin interface

	const FSlateBrush* GetPinBorder() const;
};


class SGraphNodeEntry : public SGraphNode
{
public:
	SLATE_BEGIN_ARGS(SGraphNodeEntry) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEntryNodeDB* InNode);

	// SGraphNode interface
	virtual void UpdateGraphNode() override;
	virtual void AddPin(const TSharedRef<SGraphPin>& PinToAdd) override;
	virtual void CreatePinWidgets() override;
	// End of SGraphNode interface

protected:
	FSlateColor GetBorderBackgroundColor() const;
	FText GetPreviewCornerText() const;
};
