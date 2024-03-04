// Copyright 2022 Greg Shynkar. All Rights Reserved

#pragma once

//#include "EdGraphNode_Comment.h"
#include "AIGraphSchema.h"
#include "Graphs/Nodes/StateNodeDB.h"
#include "Graphs/Nodes/CommentNodeDB.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphPin.h"
#include "DialogueBuilderGSchemaActions.generated.h"

// This class represents what will happen with creating a nodes
USTRUCT()
struct DIALOGUEBUILDEREDITOR_API FDialogueBGSchemaGraphNodeActions : public FEdGraphSchemaAction
{
	GENERATED_BODY()
public:
	
	FDialogueBGSchemaGraphNodeActions() {};

	FDialogueBGSchemaGraphNodeActions(FText InNodeCategory, FText InMenuDesc, FText InToolTip, const int32 InGrouping)
		: FEdGraphSchemaAction(MoveTemp(InNodeCategory), MoveTemp(InMenuDesc), MoveTemp(InToolTip), InGrouping){}
	
	// Click the Action call, a node will be created;
	virtual UEdGraphNode* PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) override;
};

USTRUCT()
struct DIALOGUEBUILDEREDITOR_API FDialogueBGSchemaGraphCommentActions : public FEdGraphSchemaAction
{
	GENERATED_BODY()
public:

	FDialogueBGSchemaGraphCommentActions() {};

	FDialogueBGSchemaGraphCommentActions(FText InNodeCategory, FText InMenuDesc, FText InToolTip, const int32 InGrouping)
		: FEdGraphSchemaAction(MoveTemp(InNodeCategory), MoveTemp(InMenuDesc), MoveTemp(InToolTip), InGrouping){}

	// Click the Action call, a node will be created;
	virtual UEdGraphNode* PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) override;
};