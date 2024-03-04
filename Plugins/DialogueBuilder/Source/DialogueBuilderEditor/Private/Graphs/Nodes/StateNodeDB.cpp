// Copyright 2022 Greg Shynkar. All Rights Reserved


#include "Graphs/Nodes/StateNodeDB.h"


#define LOCTEXT_NAMESPACE "DialogueStateNode"


void UStateNodeDB::AllocateDefaultPins()
{
	CreatePin(EGPD_Input, FName("StateNodePin"), FName("In"));
	CreatePin(EGPD_Output, FName("StateNodePin"), FName("Out"));
}

void UStateNodeDB::AutowireNewNode(UEdGraphPin* FromPin)
{
	// First Event When node is created;
	if (DialogueNodeId == -1)
	{
		SetStartValues();
	}

	if (FromPin)
	{
		if (GetSchema()->TryCreateConnection(FromPin, GetInputPin()))
		{
			FromPin->GetOwningNode()->NodeConnectionListChanged();
		}
	}
	UDialogueBuilderEdGraph* DialogueBuilderEdGraph = Cast<UDialogueBuilderEdGraph>(GetGraph());
	if (DialogueBuilderEdGraph)
	{
		GraphID = DialogueBuilderEdGraph->GraphGuid;
		DialogueBuilderEdGraph->Modify();
	}

	Modify();

	UpdateNodeAtDialData();

	Super::AutowireNewNode(FromPin);
}

void UStateNodeDB::SetStartValues()
{
	UDialogueBuilderEdGraph* DialogueBuilderEdGraph = Cast<UDialogueBuilderEdGraph>(GetGraph());

	DialogueBuilderEdGraph->AddIdCount();
	DialogueNodeId = DialogueBuilderEdGraph->GetIdCount();
}

FText UStateNodeDB::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return DialogueNodeText;
}

FText UStateNodeDB::GetTooltipText() const
{
	return LOCTEXT("DialogueStateNode_Tooltip", "Dialogue Node");
}

UEdGraphPin* UStateNodeDB::GetInputPin() const
{
	return Pins[0];
}

UEdGraphPin* UStateNodeDB::GetOutputPin() const
{
	return Pins[1];
}

void UStateNodeDB::PinConnectionListChanged(UEdGraphPin* Pin)
{

	// Output Part Pin Connection List Changed;
	if (Pin==GetOutputPin())
	{		
		// Check if Number of attached Nodes more than data containers; 
		if (Pin->LinkedTo.Num() > PinsIDArray.Num())
		{
			PinsIDArray.Add(Pin->LinkedTo.Last()->PinId);
		}
		//If less need to find empty connection and delete it from data containers;
		if (Pin->LinkedTo.Num() < PinsIDArray.Num())
		{
			PostBreakConnectionWithNextState(Pin);
		}
		
	}
	// Input Part Pin Connection List Changed;
	if (Pin == GetInputPin())
	{

	}
}

void UStateNodeDB::SetName(FText NewName)
{
	DialogueNodeText = NewName;
}

void UStateNodeDB::UpdateNodeAtDialData()
{	
	// IsPlayerNode can't be different from IsPlayerAnswerOption;
	if (IsPlayerNode == false && IsPlayerAnswerOption == true  )
	{
		IsPlayerAnswerOption = false;
	}

	UDialogueBuilderEdGraph* DialogueGraph = Cast<UDialogueBuilderEdGraph>(GetGraph());

	if (DialogueGraph)
	{
		if (DialogueNodeId == -1)
		{
			DialogueGraph->AddIdCount();
			DialogueNodeId = DialogueGraph->GetIdCount();

			NextNodesId.Empty();
			StateTransitionNameToCheck.Empty();
			PinsIDArray.Empty();
			OnDialoguePlayFuncName = "";
			NodeForJump = nullptr;
			
		}

		FDialogueDetailsStruct DataElement;
		// Visible Data Parts in Dialogue Editor;
		DataElement.NodeId = DialogueNodeId;
		DataElement.DialogueNodeText = DialogueNodeText;
		DataElement.IsPlayerNode = IsPlayerNode;
		DataElement.IsPlayerAnswerOption = IsPlayerAnswerOption;
		DataElement.SpeakerName = SpeakerName;
		DataElement.SpeakerTexture = SpeakerTexture;
		DataElement.IsSwitchNode = IsSwitchNode;
		DataElement.DelayToMoveNextNodes = DelayToMoveNextNodes;
		DataElement.DialogueAudio = DialogueAudio;
		DataElement.DialogueWave = DialogueWave;
		DataElement.BeginPlayDelay = BeginPlayDelay;
		DataElement.AnimToUpdate = AnimToUpdate;
		DataElement.AnimToUpdateHead = AnimToUpdateHead;
		DataElement.IsItEndNode = IsItEndNode;
		DataElement.WriteEffect = WriteEffect;
		DataElement.WriteEffectSpeed = WriteEffectSpeed;
		DataElement.IsTimedDecisions = IsTimedDecisions;
		DataElement.TimedDecisionsTime = TimedDecisionsTime;

		// Working Data Parts in Dialogue Editor;
		DataElement.NextNodesId = NextNodesId;
		DataElement.NodePosition = FVector2D(float(NodePosX), float(NodePosY));
		DataElement.TransitionNameToCheck = StateTransitionNameToCheck;
		DataElement.OnDialoguePlayFuncName = OnDialoguePlayFuncName;

		DialogueGraph->UpdateElementAtData(DataElement);
	}
}


void UStateNodeDB::OnDialoguePlay(float PlayDelay)
{
	// Sample for Transition function creation;
}

void UStateNodeDB::DestroyTransitionNodes()
{

	for (int32 LinkIndex = 0; LinkIndex < Pins[1]->LinkedTo.Num(); ++LinkIndex)
	{
		UEdGraphNode* TargetNode = Pins[1]->LinkedTo[LinkIndex]->GetOwningNode();
		TargetNode->DestroyNode();
	}

	// Bidirectional transitions where we are the 'backwards' link
	for (int32 LinkIndex = 0; LinkIndex < Pins[0]->LinkedTo.Num(); ++LinkIndex)
	{
		UEdGraphNode* TargetNode = Pins[0]->LinkedTo[LinkIndex]->GetOwningNode();
	}
}

int32 UStateNodeDB::GetFunctionListindex(int32 DialogueNodeID)
{
	return NextNodesId.Find(DialogueNodeID);
}

void UStateNodeDB::BreakConnectionWithNextState(int32 ConnectNodeID)
{
	int32 IndexToRemove = NextNodesId.Find(ConnectNodeID);
	
	if (IndexToRemove != -1)
	{
		PinsIDArray.RemoveAt(IndexToRemove);
		NextNodesId.RemoveAt(IndexToRemove);
		StateTransitionNameToCheck.RemoveAt(IndexToRemove);
	}
	
	UpdateNodeAtDialData();
}

void UStateNodeDB::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	UDialogueBuilderEdGraph*DialogueGraph = Cast<UDialogueBuilderEdGraph>(GetGraph());
	
	if (DialogueGraph)
	{
		// If Node copied from other Dialogue;
		if (GraphID != DialogueGraph->GraphGuid)
		{
			DialogueNodeId = -1;
			GraphID = DialogueGraph->GraphGuid;
		}

		DialogueGraph->NotifyGraphChanged();
		
		// Remove Duplicate ID Number check;
		for (int32 x = 0; x < DialogueGraph->Nodes.Num(); ++x)
		{
			UStateNodeDB* StateCast = Cast<UStateNodeDB>(DialogueGraph->Nodes[x]);
			if (StateCast)
			{
				if (StateCast->DialogueNodeId == DialogueNodeId && StateCast != this)
				{
					DialogueGraph->AddIdCount();
					DialogueNodeId = DialogueGraph->GetIdCount();
					NextNodesId.Empty();
					StateTransitionNameToCheck.Empty();
					PinsIDArray.Empty();
					OnDialoguePlayFuncName = "";
					NodeForJump = nullptr;
				}
			}
		}
		
	}

	UpdateNodeAtDialData();

	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void UStateNodeDB::PostBreakConnectionWithNextState(UEdGraphPin* Pin)
{

	int32 IndexToRemove = -1;
	TArray<FGuid> LocalPinIDArray;

	// Collect all PinId connected to Pin pointer;
	for (int32 y = 0; y < Pin->LinkedTo.Num(); ++y)
	{
		LocalPinIDArray.Add(Pin->LinkedTo[y]->PinId);
	}

	// Find Index of Not Connected Pin by its ID;
	for (int32 x = 0; x < PinsIDArray.Num(); ++x)
	{
		IndexToRemove = x;

		if (LocalPinIDArray.Find(PinsIDArray[IndexToRemove]) == -1)
		{
			break;
		}
	}

	if (IndexToRemove != -1)
	{
		PinsIDArray.RemoveAt(IndexToRemove);
		NextNodesId.RemoveAt(IndexToRemove);
		StateTransitionNameToCheck.RemoveAt(IndexToRemove);
	}

	UpdateNodeAtDialData();
}

void UStateNodeDB::DestroyNode()
{
	// Destroy related TransitionNodes with node deletion;
	DestroyTransitionNodes();

	// Remove this Dialogue info from DialogueData Array;
	UDialogueBuilderEdGraph* DialogueGraph = Cast<UDialogueBuilderEdGraph>(GetGraph());
	if (DialogueGraph)
	{
		DialogueGraph->RemoveNodeFromData(DialogueNodeId);
	}
	
	Super::DestroyNode();
}

#undef LOCTEXT_NAMESPACE