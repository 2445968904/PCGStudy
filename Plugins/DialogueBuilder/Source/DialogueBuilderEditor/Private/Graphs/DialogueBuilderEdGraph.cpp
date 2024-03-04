// Copyright 2022 Greg Shynkar. All Rights Reserved


#include "Graphs/DialogueBuilderEdGraph.h"


void UDialogueBuilderEdGraph::AddEntryNodeData()
{
	if (DialogueBuilderObject->DialoguesData.Num()==0)
	{
		AddIdCount();

		FDialogueDetailsStruct StartStructure;
		StartStructure.DialogueNodeText = FText::FromString("Start Dialogue");
		StartStructure.NodeId = 0;

		AddElementToData(StartStructure);
	}
}

void UDialogueBuilderEdGraph::UpdateElementAtData(FDialogueDetailsStruct Data)
{
	int32 IndexInArray = DialogueBuilderObject->GetListIndexFromNodeID(Data.NodeId);

	if (IndexInArray==-1)
	{
		//If True, Data didn't find element , so need add new element to data;
		DialogueBuilderObject->AddDialogueDataElement(Data);
	}
	else
	{
		DialogueBuilderObject->SetDataElement(IndexInArray, Data);
	}
}

void UDialogueBuilderEdGraph::AddElementToData(FDialogueDetailsStruct Data)
{
	if (DialogueBuilderObject)
	{
		DialogueBuilderObject->AddDialogueDataElement(Data);
	}
}

void UDialogueBuilderEdGraph::RemoveNodeFromData(int32 NodeId)
{
	if (DialogueBuilderObject)
	{
		DialogueBuilderObject->RemoveDataElement(DialogueBuilderObject->GetListIndexFromNodeID(NodeId));
	}
}

int32 UDialogueBuilderEdGraph::GetIdCount()
{
	return NodeIdCount;
}

void UDialogueBuilderEdGraph::AddIdCount()
{
	NodeIdCount++;
}