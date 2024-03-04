// Copyright 2022 Greg Shynkar. All Rights Reserved

#pragma once

#include "Graphs/Nodes/CommentNodeDB.h"
#include "Graphs/Nodes/TransitionNodeDB.h"
#include "Graphs/Nodes/EntryNodeDB.h"
#include "Graphs/Nodes/StateNodeDB.h"
#include "Graphs/Nodes/Slates/GraphNodeCommentDB.h"
#include "KismetPins/SGraphPinExec.h"
#include "DialogueBuilderGraphSchema.h"
#include "DialogueBGraphConnectionDrawingPolicy.h"
#include "EdGraphUtilities.h"

struct  FDialogueBuilderGraphNodeFactory : public FGraphPanelNodeFactory
{
	//Creating a corresponding nodes for Dialogue;
	virtual TSharedPtr<class SGraphNode> CreateNode(class UEdGraphNode* InNode) const override;
};

struct  FDialogueBuilderGraphPinConnectionFactory : public FGraphPanelPinConnectionFactory
{
	//Creating a corresponding Draw Policy for Dialogue;
	virtual class FConnectionDrawingPolicy* CreateConnectionPolicy(const class UEdGraphSchema* Schema, int32 InBackLayerID, int32 InFrontLayerID, float ZoomFactor, const class FSlateRect& InClippingRect, class FSlateWindowElementList& InDrawElements, class UEdGraph* InGraphObj) const override;
};
