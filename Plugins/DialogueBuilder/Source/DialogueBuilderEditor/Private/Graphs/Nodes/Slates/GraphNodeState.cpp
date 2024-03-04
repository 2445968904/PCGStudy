// Copyright 2022 Greg Shynkar. All Rights Reserved

#include "Graphs/Nodes/Slates/GraphNodeState.h"

#define LOCTEXT_NAMESPACE "DialogueBuilderGraphNodeState"

/////// Start SGraphNodePin
void SGraphNodePin::Construct(const FArguments& InArgs, UEdGraphPin* InPin)
{
	this->SetCursor(EMouseCursor::Default);

	//typedef SGraphNodePin ThisClass;

	bShowLabel = true;

	GraphPinObj = InPin;
	check(GraphPinObj != NULL);

	// Set up a hover for pins that is tinted the color of the pin.
	SBorder::Construct(SBorder::FArguments()
		.BorderImage(this, &SGraphNodePin::GetPinBorder)
		.BorderBackgroundColor(this, &SGraphNodePin::GetPinColor)
		.OnMouseButtonDown(this, &SGraphNodePin::OnPinMouseDown)
		.Cursor(this, &SGraphNodePin::GetPinCursor)
	);
}

TSharedRef<SWidget>	SGraphNodePin::GetDefaultValueWidget()
{
	return SNew(STextBlock);
}

const FSlateBrush* SGraphNodePin::GetPinBorder() const
{
	return (IsHovered())
		? FAppStyle::GetBrush(TEXT("Graph.StateNode.Pin.BackgroundHovered"))
		: FAppStyle::GetBrush(TEXT("Graph.StateNode.Pin.Background"));
}

FSlateColor SGraphNodePin::GetPinColor() const
{
	if (GetPinObj())
	{
		if (GetPinObj()->GetOwningNode())
		{
			UStateNodeDB* StateNodeRef = Cast<UStateNodeDB>(GetPinObj()->GetOwningNode());
			if (StateNodeRef)
			{
				if (StateNodeRef->IsPlayerNode && IsHovered())
				{
					return FSlateColor(FLinearColor(0.007f, 0.2f, 0.25f));
				}
			}
		}
	}
	return FSlateColor(FLinearColor(0.4f, 0.4f, 0.4f));
}
/////// End SGraphNodePin


/////// Start SDialogueNodeIndex
void SDialogueNodeIndex::Construct(const FArguments& InArgs)
{
	const FSlateBrush* IndexBrush = FAppStyle::GetBrush(TEXT("BTEditor.Graph.BTNode.Index"));

	FSlateFontInfo FontRef = FAppStyle::GetFontStyle("BTEditor.Graph.BTNode.IndexText");
	FontRef.Size = 8;

	ChildSlot
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)

		[
			// Add a dummy box here to make sure the widget doesnt get smaller than the brush
			SNew(SBox)
			.WidthOverride(IndexBrush->ImageSize.X)
			.HeightOverride(IndexBrush->ImageSize.Y)
		]
	+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SBorder)
			.BorderImage(IndexBrush)
		.BorderBackgroundColor(this, &SDialogueNodeIndex::GetColor)
		.Padding(FMargin(4.0f, 0.0f, 4.0f, 1.0f))
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		[
			SNew(STextBlock)
			.Text(InArgs._Text)
		.Font(FontRef)
		]
		]
		];
}

FSlateColor SDialogueNodeIndex::GetColor() const
{
	FLinearColor InactiveStateColor(0.08f, 0.08f, 0.08f);
	return InactiveStateColor;
}
/////// End SDialogueNodeIndex


/////// SGraphNodeState
void SGraphNodeState::Construct(const FArguments& InArgs, UStateNodeDB* InNode)
{
	this->GraphNode = InNode;
	StateNodeRef = InNode;
	this->SetCursor(EMouseCursor::CardinalCross);
	this->UpdateGraphNode();
}

FSlateColor SGraphNodeState::GetBorderBackgroundColor() const
{
	FLinearColor InactiveStateColor(0.08f, 0.08f, 0.08f);
	FLinearColor IsSwitchNodeColor(0.572917f, 0.572917f, 0.572917f);
	FLinearColor IsPlayerNodeColor(0.f, 0.06f, 0.1f);

	if (StateNodeRef)
	{
		if (StateNodeRef->IsSwitchNode)
		{
			return IsSwitchNodeColor;
		}

		if (StateNodeRef->IsPlayerNode)
		{
			return IsPlayerNodeColor;
		}
		
	}

	return InactiveStateColor;
}

void SGraphNodeState::UpdateGraphNode()
{
	InputPins.Empty();
	OutputPins.Empty();
	
	// Reset variables that are going to be exposed, in case we are refreshing an already setup node.
	RightNodeBox.Reset();
	LeftNodeBox.Reset();
	
	const FSlateBrush* NodeTypeIcon = GetNameIcon();
	TSharedPtr<SErrorText> ErrorText;

	FLinearColor TitleShadowColor(0.6f, 0.6f, 0.6f);
	TSharedPtr<SNodeTitle> NodeTitle = SNew(SNodeTitle, GraphNode);

	IndexOverlay = SNew(SDialogueNodeIndex)
		.ToolTipText(this, &SGraphNodeState::GetIndexTooltipText)
		.Visibility(EVisibility::Visible)
		.Text(this, &SGraphNodeState::GetIndexText);

	this->ContentScale.Bind(this, &SGraphNode::GetContentScale);
	this->GetOrAddSlot(ENodeZone::Center)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("Graph.StateNode.Body"))
			.Padding(0)
			.BorderBackgroundColor(this, &SGraphNodeState::GetBorderBackgroundColor)
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
				.Padding(10.0f)
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
						.BackgroundColor(this, &SGraphNodeState::GetErrorColor)
						.ToolTipText(this, &SGraphNodeState::GetErrorMsgToolTip)
					]		
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SImage)
					.Image(NodeTypeIcon)
				]
			+ SHorizontalBox::Slot()
				.Padding(FMargin(4.0f, 0.0f, 4.0f, 0.0f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SMultiLineEditableText)
					.TextStyle(FAppStyle::Get(), "Graph.Node.NodeTitle")
					.Text(StateNodeRef->DialogueNodeText)
					.OnTextCommitted(this, &SGraphNodeState::CustOnTextCommitted)
				]	
			]
			]
			]
			]
		];

	// Create comment bubble
	const FSlateColor CommentColor = FLinearColor::White;
	SAssignNew(CommentBubble, SCommentBubble)
		.GraphNode(GraphNode)
		.Text(this, &SGraphNodeState::GetNodeComment)
		.OnTextCommitted(this, &SGraphNodeState::OnCommentTextCommitted)
		.ColorAndOpacity(CommentColor)
		.AllowPinning(false)
		.EnableTitleBarBubble(IsEnabledByText())
		.EnableBubbleCtrls(true)
		.GraphLOD(this, &SGraphNode::GetCurrentLOD)
		.IsGraphNodeHovered(this, &SGraphNodeState::IsHovered);

		GetOrAddSlot(ENodeZone::TopCenter)
		.SlotOffset(TAttribute<FVector2D>(CommentBubble.Get(), &SCommentBubble::GetOffset))
		.SlotSize(TAttribute<FVector2D>(CommentBubble.Get(), &SCommentBubble::GetSize))
		.AllowScaling(TAttribute<bool>(CommentBubble.Get(), &SCommentBubble::IsScalingAllowed))
		.VAlign(VAlign_Top)
		[
			CommentBubble.ToSharedRef()
		];

	// Error Widget;
	ErrorReporting = ErrorText;
	ErrorReporting->SetError(ErrorMsg);
	CreatePinWidgets();
	}

void SGraphNodeState::CreatePinWidgets()
{
	UStateNodeDB* StateNode = CastChecked<UStateNodeDB>(GraphNode);

	UEdGraphPin* CurPin = StateNode->GetOutputPin();
	if (!CurPin->bHidden)
	{
		TSharedPtr<SGraphPin> NewPin = SNew(SGraphNodePin, CurPin);

		AddPin(NewPin.ToSharedRef());
	}
}

void SGraphNodeState::AddPin(const TSharedRef<SGraphPin>& PinToAdd)
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

TArray<FOverlayWidgetInfo> SGraphNodeState::GetOverlayWidgets(bool bSelected, const FVector2D& WidgetSize) const
{
	TArray<FOverlayWidgetInfo> Widgets;

	check(IndexOverlay.IsValid());

	FVector2D Origin(0.0f, -10.0f);

 	FOverlayWidgetInfo Overlay(IndexOverlay);
	Overlay.OverlayOffset = FVector2D(WidgetSize.X - (IndexOverlay->GetDesiredSize().X * 0.3f), Origin.Y);

	Widgets.Add(Overlay);
	
	return Widgets;
}

void SGraphNodeState::CustOnTextCommitted(const FText& InText, ETextCommit::Type CommitInfo)
{
	if (StateNodeRef)
	{
		StateNodeRef->DialogueNodeText = InText;
		StateNodeRef->UpdateNodeAtDialData();
	}
}

const FSlateBrush* SGraphNodeState::GetNameIcon() const
{
	return FAppStyle::GetBrush(TEXT("Graph.StateNode.Icon"));
}


void SGraphNodeState::OnCommentTextCommitted(const FText& NewComment, ETextCommit::Type CommitInfo)
{
	GetNodeObj()->OnUpdateCommentText(NewComment.ToString());
	if (StateNodeRef)
	{
		StateNodeRef->SpeakerName = NewComment;
		StateNodeRef->UpdateNodeAtDialData();
	}
}

bool SGraphNodeState::IsHovered() const
{
	return true;
}

bool SGraphNodeState::IsEnabledByText() const
{
	if (StateNodeRef)
	{
		return  !StateNodeRef->SpeakerName.IsEmpty();
	}

	return false;
}

FString SGraphNodeState::GetNodeComment() const
{
	if (StateNodeRef)
	{
		FString locstring;
		if (StateNodeRef->IsPlayerAnswerOption)
		{
			locstring = "Answer - " + StateNodeRef->SpeakerName.ToString();
			return locstring;
		}
		else
		{
			locstring = StateNodeRef->SpeakerName.ToString();
			return locstring;
		}
	}

	return "";
}

FText SGraphNodeState::GetIndexText() const
{
	int32 Index = 1;
	if (StateNodeRef)
	{
		Index = StateNodeRef->DialogueNodeId;
	}
	return FText::AsNumber(Index);
}

FText SGraphNodeState::GetIndexTooltipText() const
{
	return LOCTEXT("Dialogue Node ID", "Dialogue Node ID");
}

/////// End SGraphNodeState

#undef LOCTEXT_NAMESPACE
