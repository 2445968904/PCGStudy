// Copyright 2022 Greg Shynkar. All Rights Reserved


#include "Graphs/Nodes/CommentNodeDB.h"

UCommentNodeDB::UCommentNodeDB(const FObjectInitializer& ObjectInitializer /*= FObjectInitializer::Get()*/)
{
	NodeWidth = 300.0f;
	NodeHeight = 100.0f;
	FontSize = 12;

	bCommentBubblePinned = false;
	bCommentBubbleVisible = false;
	bCanResizeNode = true;
	bCanRenameNode = false;
	CommentDepth = -1;
}


void UCommentNodeDB::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	UDialogueBuilderEdGraph*DialogueGraph = Cast<UDialogueBuilderEdGraph>(GetGraph());
	if (DialogueGraph)
	{
		DialogueGraph->NotifyGraphChanged();
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void UCommentNodeDB::ResizeNode(const FVector2D& NewSize)
{
	if (bCanResizeNode)
	{
		NodeHeight = NewSize.Y;
		NodeWidth = NewSize.X;
	}
}

void UCommentNodeDB::OnRenameNode(const FString& NewName)
{
	NodeComment = NewName;
}
