// Copyright 2022 Greg Shynkar. All Rights Reserved

#include "Graphs/Nodes/Slates/GraphNodeCommentDB.h"

void SGraphNodeCommentDB::Construct(const FArguments& InArgs, UCommentNodeDB* InNode)
{
	this->GraphNode = InNode;
	CommentNodeRef = InNode;
	bIsSelected = false;

	// Set up animation
	{
		ZoomCurve = SpawnAnim.AddCurve(0, 0.1f);
		FadeCurve = SpawnAnim.AddCurve(0.15f, 0.15f);
	}

	// Cache these values so they do not force a re-build of the node next tick.
	CachedCommentTitle = GetNodeComment();
	CachedWidth = InNode->NodeWidth;

	this->UpdateGraphNode();

	// Pull out sizes
	UserSize.X = InNode->NodeWidth;
	UserSize.Y = InNode->NodeHeight;

	MouseZone = CRWZ_NotInWindow;
	bUserIsDragging = false;
}

void SGraphNodeCommentDB::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SGraphNode::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	const int32 CurrentWidth = static_cast<int32>(UserSize.X);
	if (CurrentWidth != CachedWidth)
	{
		CachedWidth = CurrentWidth;
	}

	if (CommentNodeRef && CommentNodeRef->CommentText != CachedCommentTitle)
	{
		InlineEditableText->SetText(CommentNodeRef->CommentText);
		CachedCommentTitle = CommentNodeRef->CommentText;
	}

	if (CachedFontSize != CommentNodeRef->FontSize)
	{
		UpdateGraphNode();
	}
}

FReply SGraphNodeCommentDB::OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent)
{
	// If user double-clicked in the title bar area
	if (FindMouseZone(InMyGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition())) == CRWZ_TitleBar && IsEditable.Get())
	{
		// Request a rename
		RequestRename();

		// Set the keyboard focus
		if (!HasKeyboardFocus())
		{
			FSlateApplication::Get().SetKeyboardFocus(SharedThis(this), EFocusCause::SetDirectly);
		}

		return FReply::Handled();
	}
	else
	{
		// Otherwise let the graph handle it, to allow spline interactions to work when they overlap with a comment node
		return FReply::Unhandled();
	}
}

FReply SGraphNodeCommentDB::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if ((MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton) && bUserIsDragging)
	{
		bUserIsDragging = false;

		// Resize the node	
		UserSize.X = FMath::RoundToFloat(UserSize.X);
		UserSize.Y = FMath::RoundToFloat(UserSize.Y);

		GetNodeObj()->ResizeNode(UserSize);

		// End resize transaction
		ResizeTransactionPtr.Reset();

		return FReply::Handled().ReleaseMouseCapture();
	}
	
	return FReply::Unhandled();
}

void SGraphNodeCommentDB::OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{

}

FReply SGraphNodeCommentDB::OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	return FReply::Unhandled();
}

void SGraphNodeCommentDB::UpdateGraphNode()
{
	// No pins in a comment box
	InputPins.Empty();
	OutputPins.Empty();

	// Avoid standard box model too
	RightNodeBox.Reset();
	LeftNodeBox.Reset();

	// Remember if we should be showing the bubble
	CommentNodeRef = CastChecked<UCommentNodeDB>(GraphNode);

	// Setup a tag for this node
	FString TagName;

	// We want the name of the blueprint as our name - we can find the node from the GUID
	UObject* Package = GraphNode->GetOutermost();
	UObject* LastOuter = GraphNode->GetOuter();
	while (LastOuter->GetOuter() != Package)
	{
		LastOuter = LastOuter->GetOuter();
	}
	TagName = FString::Printf(TEXT("GraphNode,%s,%s"), *LastOuter->GetFullName(), *GraphNode->NodeGuid.ToString());

	SetupErrorReporting();

	// Setup a meta tag for this node
	FGraphNodeMetaData TagMeta(TEXT("Graphnode"));
	PopulateMetaTag(&TagMeta);

	CommentStyle = FAppStyle::Get().GetWidgetStyle<FInlineEditableTextBlockStyle>("Graph.CommentBlock.TitleInlineEditableText");
	CommentStyle.EditableTextBoxStyle.TextStyle.Font.Size = CommentNodeRef->FontSize;
	CommentStyle.TextStyle.Font.Size = CommentNodeRef->FontSize;
	CachedFontSize = CommentNodeRef->FontSize;
	
	this->ContentScale.Bind(this, &SGraphNode::GetContentScale);
	this->GetOrAddSlot(ENodeZone::Center)
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("Kismet.Comment.Background"))
		.ColorAndOpacity(FLinearColor::White)
		.BorderBackgroundColor(this, &SGraphNodeCommentDB::GetCommentBodyColor)
		.Padding(FMargin(3.0f))
		.AddMetaData<FGraphNodeMetaData>(TagMeta)
		[
			SNew(SVerticalBox)
			.ToolTipText(this, &SGraphNode::GetNodeTooltip)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Top)
		[
			SAssignNew(TitleBar, SBorder)
			.BorderImage(FAppStyle::GetBrush("Graph.Node.TitleBackground"))
		.BorderBackgroundColor(this, &SGraphNodeCommentDB::GetCommentTitleBarColor)
		.Padding(FMargin(10, 5, 5, 3))
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Center)
		[
			SAssignNew(InlineEditableText, SInlineEditableTextBlock)
			.Style(&CommentStyle)
		.Text(FText::FromString(CommentNodeRef->CommentText))
		.OnTextCommitted(this, &SGraphNodeCommentDB::CustOnTextCommitted)
		.IsSelected(this, &SGraphNodeCommentDB::IsSelectedExclusively)
		]
		]
	+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(1.0f)
		[
			ErrorReporting->AsWidget()
		]
	+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			// NODE CONTENT AREA
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("NoBorder"))
		]
		]
		];
}

FString SGraphNodeCommentDB::GetNodeComment() const
{
	UCommentNodeDB* CommentNodeDB = Cast<UCommentNodeDB>(GraphNode);
	if (CommentNodeDB)
	{
		return CommentNodeDB->CommentText;
	}
	
	const FString Title = GetEditableNodeTitle();
	return Title;
}

void SGraphNodeCommentDB::GetOverlayBrushes(bool bSelected, const FVector2D WidgetSize, TArray<FOverlayBrushInfo>& Brushes) const
{
	const float Fudge = 3.0f;

	FOverlayBrushInfo HandleBrush = FAppStyle::GetBrush(TEXT("Graph.Node.Comment.Handle"));

	HandleBrush.OverlayOffset.X = WidgetSize.X - HandleBrush.Brush->ImageSize.X - Fudge;
	HandleBrush.OverlayOffset.Y = WidgetSize.Y - HandleBrush.Brush->ImageSize.Y - Fudge;

	Brushes.Add(HandleBrush);
	return SGraphNode::GetOverlayBrushes(bSelected, WidgetSize, Brushes);
}

int32 SGraphNodeCommentDB::GetSortDepth() const
{
	UCommentNodeDB* CommentNodeDB = Cast<UCommentNodeDB>(GraphNode);
	return CommentNodeDB ? CommentNodeDB->CommentDepth : -1;
}

FVector2D SGraphNodeCommentDB::ComputeDesiredSize(float) const
{
	return UserSize;
}

float SGraphNodeCommentDB::GetTitleBarHeight() const
{
	return TitleBar.IsValid() ? TitleBar->GetDesiredSize().Y : 0.0f;
}

FVector2D SGraphNodeCommentDB::GetNodeMaximumSize() const
{
	return FVector2D(UserSize.X + 500, UserSize.Y + 500);

}

void SGraphNodeCommentDB::CustOnTextCommitted(const FText& InText, ETextCommit::Type CommitInfo)
{
	if (CommentNodeRef)
	{
		CommentNodeRef->CommentText = InText.ToString();
	}
}


FSlateColor SGraphNodeCommentDB::GetCommentBodyColor() const
{
	UCommentNodeDB* CommentNodeDB = Cast<UCommentNodeDB>(GraphNode);

	if (CommentNodeDB)
	{
		return CommentNodeDB->CommentColor;
	}
	else
	{
		return FLinearColor::White;
	}
}

FSlateColor SGraphNodeCommentDB::GetCommentTitleBarColor() const
{
	UCommentNodeDB* CommentNodeDB = Cast<UCommentNodeDB>(GraphNode);
	if (CommentNodeDB)
	{
		const FLinearColor Color = CommentNodeDB->CommentColor * 0.6f;
		return FLinearColor(Color.R, Color.G, Color.B);
	}
	else
	{
		const FLinearColor Color = FLinearColor::White * 0.6f;
		return FLinearColor(Color.R, Color.G, Color.B);
	}
}

FSlateColor SGraphNodeCommentDB::GetCommentBubbleColor() const
{
	UCommentNodeDB* CommentNodeDB = Cast<UCommentNodeDB>(GraphNode);
	if (CommentNodeDB)
	{
		const FLinearColor Color = FLinearColor::White * 0.6f;
		return FLinearColor(Color.R, Color.G, Color.B);
	}
	else
	{
		const FLinearColor Color = FLinearColor::White * 0.6f;
		return FLinearColor(Color.R, Color.G, Color.B);
	}
}


float SGraphNodeCommentDB::GetWrapAt() const
{
	return (float)(CachedWidth - 32.0f);
}
