// Copyright 2022 Greg Shynkar. All Rights Reserved

#include "Graphs/Nodes/TransitionNodeDB.h"


void UTransitionNodeDB::AllocateDefaultPins()
{
	UEdGraphPin* Inputs = CreatePin(EGPD_Input, TEXT("Transition"), TEXT("In"));
	Inputs->bHidden = true;
	UEdGraphPin* Outputs = CreatePin(EGPD_Output, TEXT("Transition"), TEXT("Out"));
	Outputs->bHidden = true;
}

UEdGraphPin* UTransitionNodeDB::GetInputPin() const
{
	return Pins[0];
}

UEdGraphPin* UTransitionNodeDB::GetOutputPin() const
{
	return Pins[1];
}

void UTransitionNodeDB::DestroyNode()
{	
	Super::DestroyNode();
}

UEdGraphNode* UTransitionNodeDB::GetPreviousState() const
{
	if (Pins.Num()>0) 
	{
		if (Pins[0]->LinkedTo.Num() > 0)
		{
			// Cast safety as connection always to UStateNodeDB;
			return Cast<UStateNodeDB>(Pins[0]->LinkedTo[0]->GetOwningNode());
		}
		else
		{
			return NULL;
		}
	}
	else
	{
		return NULL;
	}
}

UEdGraphNode* UTransitionNodeDB::GetNextState() const
{
	if (Pins.Num() > 0) 
	{
		if (Pins[1]->LinkedTo.Num() > 0)
		{
			// Cast safety as connection always to UStateNodeDB;
			return Cast<UStateNodeDB>(Pins[1]->LinkedTo[0]->GetOwningNode());
		}
		else
		{
			return NULL;
		}
	}
	else
		{
			return NULL;
		}
}

void UTransitionNodeDB::CreateConnections(class UStateNodeDB* PreviousState, class UStateNodeDB* NextState)
{

	// Previous to this
	Pins[0]->Modify();
	Pins[0]->LinkedTo.Empty();

	PreviousState->GetOutputPin()->Modify();
	Pins[0]->MakeLinkTo(PreviousState->GetOutputPin());

	// This to next
	Pins[1]->Modify();
	Pins[1]->LinkedTo.Empty();

	NextState->GetInputPin()->Modify();
	Pins[1]->MakeLinkTo(NextState->GetInputPin());

	ConnectStateNodesDB(PreviousState, NextState);
}

bool UTransitionNodeDB::ConnectStateNodesDB(class UStateNodeDB* PreviousState, class UStateNodeDB* NextState)
{	
	if (PreviousState!=NULL && NextState != NULL)
	{
		if (NextState->DialogueNodeId==-1)
		{
			NextState->SetStartValues();
		}

		PreviousNodeID = PreviousState->DialogueNodeId;
		NextNodeID = NextState->DialogueNodeId;

		// Added links between nodes;
		PreviousState->NextNodesId.Add(NextState->DialogueNodeId);
		ConnectFunctionName = "True";
		PreviousState->StateTransitionNameToCheck.Add("True");

		PreviousState->UpdateNodeAtDialData();

		return true;
	}

	return false;
}

void UTransitionNodeDB::PinConnectionListChanged(UEdGraphPin* Pin)
{
	UStateNodeDB* PreviousState = Cast<UStateNodeDB>(GetPreviousState());
	UStateNodeDB* NextState = Cast<UStateNodeDB>(GetNextState());

	if (Pin->LinkedTo.Num() == 0)
	{
		if (PreviousState)
		{
			PreviousState->BreakConnectionWithNextState(NextNodeID);
		}

		DestroyNode();
	}
}


void UTransitionNodeDB::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	UStateNodeDB* PreviousState = Cast<UStateNodeDB>(GetPreviousState());
	UStateNodeDB* NextState = Cast<UStateNodeDB>(GetNextState());

	if (PreviousState)
	{
		int32 IndexOfNextStateInsidePrevious = PreviousState->NextNodesId.Find(NextState->DialogueNodeId);

		if (IndexOfNextStateInsidePrevious == -1)
		{
			PreviousState->NextNodesId.Add(NextState->DialogueNodeId);
			PreviousState->StateTransitionNameToCheck.Add(ConnectFunctionName);
			PreviousState->PinsIDArray.Add(GetInputPin()->PinId);
			PreviousState->UpdateNodeAtDialData();
		}
		else
		{
			PreviousState->StateTransitionNameToCheck[IndexOfNextStateInsidePrevious] = ConnectFunctionName;
			PreviousState->UpdateNodeAtDialData();
		}
	}

	if (!PreviousState || !NextState)
	{
		DestroyNode();
	}
}

void UTransitionNodeDB::SetFunctionName(FName FunctionCheckName)
{

	UStateNodeDB* PreviousState = Cast<UStateNodeDB>(GetPreviousState());
	UStateNodeDB* NextState = Cast<UStateNodeDB>(GetNextState());

	if (PreviousState)
	{
		ConnectFunctionName = FunctionCheckName;
		PreviousState->StateTransitionNameToCheck[PreviousState->GetFunctionListindex(NextState->DialogueNodeId)] = FunctionCheckName;

		PreviousState->UpdateNodeAtDialData();
	}
}

