// Copyright 2022 Greg Shynkar. All Rights Reserved

#include "Graphs/Nodes/Slates/GraphNodeEntry.h"


/** Start SGraphEntryPin Part */

void SGraphEntryPin::Construct(const FArguments& InArgs, UEdGraphPin* InPin)
{
	this->SetCursor(EMouseCursor::Default);

	bShowLabel = true;

	GraphPinObj = InPin;
	check(GraphPinObj != NULL);

	// Set up a hover for pins that is tinted the color of the pin.
	SBorder::Construct(SBorder::FArguments()
		.BorderImage(this, &SGraphEntryPin::GetPinBorder)
		.BorderBackgroundColor(this, &SGraphEntryPin::GetPinColor)
		.OnMouseButtonDown(this, &SGraphEntryPin::OnPinMouseDown)
		.Cursor(this, &SGraphEntryPin::GetPinCursor)
	);
}

TSharedRef<SWidget> SGraphEntryPin::GetDefaultValueWidget()
{
	return SNew(STextBlock);
}

const FSlateBrush* SGraphEntryPin::GetPinBorder() const
{
	return (IsHovered())
		? FAppStyle::GetBrush(TEXT("Graph.StateNode.Pin.BackgroundHovered"))
		: FAppStyle::GetBrush(TEXT("Graph.StateNode.Pin.Background"));
}
	
FSlateColor SGraphEntryPin::GetPinColor() const
{
	return FSlateColor(FLinearColor(1.0f, 1.0f, 0.15f));
}
/** End SGraphEntryPin Part */


/** Start SGraphNodeEntry Part */

void SGraphNodeEntry::Construct(const FArguments& InArgs, UEntryNodeDB* InNode)
{
	this->GraphNode = InNode;
	this->SetCursor(EMouseCursor::CardinalCross);
	this->UpdateGraphNode();
}

void SGraphNodeEntry::UpdateGraphNode()
{
	InputPins.Empty();
	OutputPins.Empty();

	// Reset variables that are going to be exposed, in case we are refreshing an already setup node.
	RightNodeBox.Reset();
	LeftNodeBox.Reset();

	const FSlateBrush* NodeTypeIcon = FAppStyle::GetBrush(TEXT("Graph.StateNode.Icon"));

	FLinearColor TitleShadowColor(0.20f, 0.20f, 0.20f);

	TSharedPtr<SErrorText> ErrorText;
	TSharedPtr<SNodeTitle> NodeTitle = SNew(SNodeTitle, GraphNode);

	this->ContentScale.Bind(this, &SGraphNode::GetContentScale);
	this->GetOrAddSlot(ENodeZone::Center)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("Graph.StateNode.Body"))
		.Padding(0)
		.BorderBackgroundColor(this, &SGraphNodeEntry::GetBorderBackgroundColor)
		[
			SNew(SOverlay)
			// PIN AREA
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SAssignNew(RightNodeBox, SVerticalBox)
		]

	// STATE NAME AREA
	+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Padding(8.0f)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("Graph.StateNode.ColorSpill"))
		.BorderBackgroundColor(TitleShadowColor)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Visibility(EVisibility::SelfHitTestInvisible)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()

		.AutoWidth()
		[
			// POPUP ERROR MESSAGE
			SAssignNew(ErrorText, SErrorText)
			.BackgroundColor(this, &SGraphNodeEntry::GetErrorColor)
		.ToolTipText(this, &SGraphNodeEntry::GetErrorMsgToolTip)
		]
	+ SHorizontalBox::Slot()
		.Padding(FMargin(8.0f, 0.0f, 6.0f, 0.0f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SAssignNew(InlineEditableText, SInlineEditableTextBlock)
			.Style(FAppStyle::Get(), "Graph.StateNode.NodeTitleInlineEditableText")
		.Text(NodeTitle.Get(), &SNodeTitle::GetHeadTitle)
		.OnVerifyTextChanged(this, &SGraphNodeEntry::OnVerifyNameTextChanged)
		.OnTextCommitted(this, &SGraphNodeEntry::OnNameTextCommited)
		.IsReadOnly(this, &SGraphNodeEntry::IsNameReadOnly)
		.IsSelected(this, &SGraphNodeEntry::IsSelectedExclusively)
		]
	+ SVerticalBox::Slot()
		.AutoHeight()
		[
			NodeTitle.ToSharedRef()
		]
		]
		]
		]
		]
		];

	ErrorReporting = ErrorText;
	ErrorReporting->SetError(ErrorMsg);
	CreatePinWidgets();
}

void SGraphNodeEntry::AddPin(const TSharedRef<SGraphPin>& PinToAdd)
{
	PinToAdd->SetOwner(SharedThis(this));
	RightNodeBox->AddSlot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		.FillHeight(1.0f)
		[
			PinToAdd
		];
	OutputPins.Add(PinToAdd);
}

void SGraphNodeEntry::CreatePinWidgets()
{
	UEntryNodeDB* EntryNode = CastChecked<UEntryNodeDB>(GraphNode);
	if (EntryNode)
	{
		UEdGraphPin* CurPin = EntryNode->GetOutputPin();
		if (!CurPin->bHidden)
		{
			TSharedPtr<SGraphPin> NewPin = SNew(SGraphEntryPin, CurPin);
			AddPin(NewPin.ToSharedRef());
		}
	}
}

FSlateColor SGraphNodeEntry::GetBorderBackgroundColor() const
{
	FLinearColor StateColor(0.7f, 0.50f, 0.06f);
	return StateColor;
}

FText SGraphNodeEntry::GetPreviewCornerText() const
{
	return NSLOCTEXT("SGraphNodeEntry", "TextDescription", "Entry point for Dialogue");
}

/** End SGraphNodeEntry Part */
