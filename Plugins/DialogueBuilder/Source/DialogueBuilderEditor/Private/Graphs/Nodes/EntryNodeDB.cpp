// Copyright 2022 Greg Shynkar. All Rights Reserved

#include "Graphs/Nodes/EntryNodeDB.h"

#define LOCTEXT_NAMESPACE "DialogueBuilderEntryNode"

void UEntryNodeDB::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	if (GetOutputNode())
	{
		PinConnectionListChanged(GetOutputPin());
	}
}

void UEntryNodeDB::AllocateDefaultPins()
{
	CreatePin(EGPD_Output, FName("EntryNodePin"), FName("Out")); //Pins[0]	
}

FText UEntryNodeDB::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::FromString("Start Dialogue");
}

FText UEntryNodeDB::GetTooltipText() const
{
	return LOCTEXT("DialogueEntryNodeTooltip", "Entry point for Dialogue");
}

void UEntryNodeDB::PinConnectionListChanged(UEdGraphPin* Pin)
{
	UDialogueBuilderEdGraph* DialogueBuilderEdGraph = Cast<UDialogueBuilderEdGraph>(GetGraph());

	if (Pin == GetOutputPin())
	{
		if(Pin->LinkedTo.Num()>0)
		{
			// If Connection change to StateNode;
			UStateNodeDB* StateNode = Cast<UStateNodeDB>(GetOutputNode());

			if (StateNode)
			{
				if (NextNodesId.Num() == 0)
				{
					NextNodesId.Add(StateNode->DialogueNodeId);
				}
				else
				{
					// Conction always to one node;
					NextNodesId[0] = StateNode->DialogueNodeId;
				}
			}

			// Update Dialogue Data;
			if (DialogueBuilderEdGraph)
			{

				FDialogueDetailsStruct DataElement;
				DataElement.NodeId = EntryNodeId;
				DataElement.DialogueNodeText = EntryNodeText;
				DataElement.NextNodesId = NextNodesId;

				DialogueBuilderEdGraph->UpdateElementAtData(DataElement);
			}

		}
		else
		{
			// If connection break;
			if (NextNodesId.Num()>0)
			{
				NextNodesId.Empty();

				FDialogueDetailsStruct DataElement;
				DataElement.NodeId = EntryNodeId;
				DataElement.DialogueNodeText = EntryNodeText;
				DataElement.NextNodesId = NextNodesId;

				DialogueBuilderEdGraph->UpdateElementAtData(DataElement);

			}
		}
	}
}


void UEntryNodeDB::UpdateNodeAtDialData()
{

	UDialogueBuilderEdGraph* DialogueBuilderEdGraph = Cast<UDialogueBuilderEdGraph>(GetGraph());
	if (DialogueBuilderEdGraph)
	{
		FDialogueDetailsStruct DataElement;
		DataElement.NodeId = EntryNodeId;
		DataElement.DialogueNodeText = EntryNodeText;
		DataElement.NextNodesId = NextNodesId;

		DialogueBuilderEdGraph->UpdateElementAtData(DataElement);
	}
}


UEdGraphPin* UEntryNodeDB::GetOutputPin() const
{
	return Pins[0];
}

UEdGraphNode* UEntryNodeDB::GetOutputNode() const
{
	if (Pins.Num() > 0 && Pins[0] != NULL)
	{
		check(Pins[0]->LinkedTo.Num() <= 1);
		if (Pins[0]->LinkedTo.Num() > 0 && Pins[0]->LinkedTo[0]->GetOwningNode() != NULL)
		{
			return Pins[0]->LinkedTo[0]->GetOwningNode();
		}
	}
	return NULL;
}

#undef LOCTEXT_NAMESPACE
