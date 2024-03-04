// Copyright 2022 Greg Shynkar. All Rights Reserved

#include "Graphs/DialogueBuilderGraphFactories.h"


TSharedPtr<class SGraphNode> FDialogueBuilderGraphNodeFactory::CreateNode(class UEdGraphNode* InNode) const
{
	if (UStateNodeDB* StateNode = Cast<UStateNodeDB>(InNode))
	{
		return SNew(SGraphNodeState, StateNode);
	}
	
	else if (UEntryNodeDB* EntryNode = Cast<UEntryNodeDB>(InNode))
	{
		return SNew(SGraphNodeEntry, EntryNode);
	}
	
	else if (UTransitionNodeDB* TransitionNode = Cast<UTransitionNodeDB>(InNode))
	{
		return SNew(SGraphNodeTransition, TransitionNode);
	}

	else if (UCommentNodeDB* CommentNodeDB = Cast<UCommentNodeDB>(InNode))
	{
		return SNew(SGraphNodeCommentDB, CommentNodeDB);
	}

	return nullptr;
}

class FConnectionDrawingPolicy* FDialogueBuilderGraphPinConnectionFactory::CreateConnectionPolicy(const class UEdGraphSchema* Schema, int32 InBackLayerID, int32 InFrontLayerID, float ZoomFactor, const class FSlateRect& InClippingRect, class FSlateWindowElementList& InDrawElements, class UEdGraph* InGraphObj) const
{
	if (Schema->IsA(UDialogueBuilderGraphSchema::StaticClass()))
	{
		return new FDialogueBGraphConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, ZoomFactor, InClippingRect, InDrawElements, InGraphObj);
	}
	return nullptr;
}
