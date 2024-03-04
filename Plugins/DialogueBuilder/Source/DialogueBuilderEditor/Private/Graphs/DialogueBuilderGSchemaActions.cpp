// Copyright 2022 Greg Shynkar. All Rights Reserved

#include "Graphs/DialogueBuilderGSchemaActions.h"

#define LOCTEXT_NAMESPACE "DialogueBuilderGraphSchemaActions"

UEdGraphNode* FDialogueBGSchemaGraphNodeActions::PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode /*= true*/)
{
	if (ParentGraph == NULL)
	{
		return NULL;
	}

	// Create a system default node temporarily
	UStateNodeDB* ResultNode = NewObject<UStateNodeDB>(ParentGraph);
	ParentGraph->Modify();
	//ResultNode->SetFlags(RF_Transactional);
	//ResultNode->Rename(NULL, ParentGraph, REN_NonTransactional);
	ParentGraph->bEditable = true;
	ParentGraph->bAllowRenaming = true;
	ParentGraph->AddNode(ResultNode, bSelectNewNode);
	ResultNode->CreateNewGuid();
	ResultNode->NodePosX = Location.X;
	ResultNode->NodePosY = Location.Y;

	// Setup node start name;
	FString NodeName = "Dialogue Node Text";
	ResultNode->SetName(FText::FromString(NodeName));

	ResultNode->AllocateDefaultPins();
	ResultNode->AutowireNewNode(FromPin);
	
	ParentGraph->NotifyGraphChanged();

	return ResultNode;
}

UEdGraphNode* FDialogueBGSchemaGraphCommentActions::PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode /*= true*/)
{
	if (ParentGraph == NULL)
	{
		return NULL;
	}

	// Create a system default node temporarily
 	UCommentNodeDB* ResultNode = NewObject<UCommentNodeDB>(ParentGraph);
	//UEdGraphNode_Comment* ResultNode = NewObject<UEdGraphNode_Comment>(ParentGraph);

	ParentGraph->Modify();
	//ResultNode->SetFlags(RF_Transactional);
	//ResultNode->bCanResizeNode = true;
	ParentGraph->AddNode(ResultNode, bSelectNewNode);

	ResultNode->CreateNewGuid();
	//ResultNode->Rename(NULL, ParentGraph, REN_None);
	//ResultNode->MakeNameValidator();
	//FString NodeName = "Comment";
	//ResultNode->OnRenameNode(NodeName);

	ResultNode->NodePosX = Location.X;
	ResultNode->NodePosY = Location.Y;
	// Setup node start name;
	ParentGraph->NotifyGraphChanged();

	return ResultNode;
}

#undef LOCTEXT_NAMESPACE

