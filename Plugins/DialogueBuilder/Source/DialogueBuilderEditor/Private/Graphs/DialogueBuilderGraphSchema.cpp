// Copyright 2022 Greg Shynkar. All Rights Reserved

#include "Graphs/DialogueBuilderGraphSchema.h"

#define LOCTEXT_NAMESPACE "DialogueBuilderGraphSchema"

void UDialogueBuilderGraphSchema::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	// Add Action To Right Mouse - Add New Dialogue Node;
	FText Category = LOCTEXT("DialogueBuilder", "Dialogue Builder");
	FText MenuDescGraph = LOCTEXT("MenuDesc", "New Dialogue Node");
	FText ToolTipGraph = LOCTEXT("DialogueToolTip", "Add New Dialogue Node to Graph");
	TSharedPtr<FDialogueBGSchemaGraphNodeActions> NewDialogueNode(new FDialogueBGSchemaGraphNodeActions(Category, MenuDescGraph, ToolTipGraph, 0));
	ContextMenuBuilder.AddAction(NewDialogueNode);

	// Add Action To Right Mouse - Add New Comment Node;
	FText MenuDescGraph1 = LOCTEXT("K2Node", "Add Comment");
	FText ToolTipGraph1 = LOCTEXT("DialogueToolTip1", "Add New Comment to Graph");
	TSharedPtr<FDialogueBGSchemaGraphCommentActions> NewDialogueNode1(new FDialogueBGSchemaGraphCommentActions(Category, MenuDescGraph1, ToolTipGraph1, 0));
	ContextMenuBuilder.AddAction(NewDialogueNode1);
}

void UDialogueBuilderGraphSchema::GetContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const
{
	check(Context && Context->Graph);
	if (Context->Node)
	{
		FToolMenuSection& Section = Menu->AddSection("StateMachineNodeActions", LOCTEXT("NodeActionsMenuHeader", "Node Actions"));

		if (!Context->bIsDebugging)
		{
			// Node contextual actions
			Section.AddMenuEntry(FDialogueBuilderGraphCommands::Get().Delete);
			Section.AddMenuEntry(FDialogueBuilderGraphCommands::Get().Cut);
			Section.AddMenuEntry(FDialogueBuilderGraphCommands::Get().Copy);
			Section.AddMenuEntry(FDialogueBuilderGraphCommands::Get().Duplicate);
		}
	}
	
	Super::GetContextMenuActions(Menu, Context);
}

void UDialogueBuilderGraphSchema::CreateDefaultNodesForGraph(UEdGraph& Graph) const
{
	if (UDialogueBuilderEdGraph* CurrentGraph = CastChecked<UDialogueBuilderEdGraph>(&Graph))
	{
		if (CurrentGraph->Entry==NULL)
		{
			// Create Entrynode;
			FGraphNodeCreator<UEntryNodeDB> NodeCreator(Graph);
			UEntryNodeDB* EntryNode = NodeCreator.CreateNode();
			NodeCreator.Finalize();
			SetNodeMetaData(EntryNode, FNodeMetadata::DefaultGraphNode);
			CurrentGraph->Entry = EntryNode;
		}
	}
}

bool UDialogueBuilderGraphSchema::TryCreateConnection(UEdGraphPin* PinA, UEdGraphPin* PinB) const
{
	if (PinB->Direction == PinA->Direction)
	{
		if (UStateNodeDB* Node = Cast<UStateNodeDB>(PinB->GetOwningNode()))
		{
			PinB = Node->GetOutputPin();
			
			if (PinA->Direction == EGPD_Input)
			{
				PinB = Node->GetOutputPin();
			}
			else
			{
				PinB = Node->GetInputPin();
			}
		}
	}

	return  UEdGraphSchema::TryCreateConnection(PinA, PinB);
}

const FPinConnectionResponse UDialogueBuilderGraphSchema::CanCreateConnection(const UEdGraphPin* PinA, const UEdGraphPin* PinB) const
{
	// Make sure the pins are not on the same node
	if (PinA->GetOwningNode() == PinB->GetOwningNode())
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Both are on the same node"));
	}

	// Connect entry node to a state is OK
	const bool bPinAIsEntry = PinA->GetOwningNode()->IsA(UEntryNodeDB::StaticClass());
	const bool bPinBIsEntry = PinB->GetOwningNode()->IsA(UEntryNodeDB::StaticClass());
	const bool bPinAIsStateNode = PinA->GetOwningNode()->IsA(UStateNodeDB::StaticClass());
	const bool bPinBIsStateNode = PinB->GetOwningNode()->IsA(UStateNodeDB::StaticClass());

	if (bPinAIsEntry || bPinBIsEntry)
	{
		if (bPinAIsEntry && bPinBIsStateNode)
		{
			return FPinConnectionResponse(CONNECT_RESPONSE_BREAK_OTHERS_A, TEXT("Connect1"));
		}

		if (bPinBIsEntry && bPinAIsStateNode)
		{
			return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Can`t Connect"));
		}

		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Entry must connect to a state node"));
	}


	const bool bPinAIsTransition = PinA->GetOwningNode()->IsA(UTransitionNodeDB::StaticClass());
	const bool bPinBIsTransition = PinB->GetOwningNode()->IsA(UTransitionNodeDB::StaticClass());

	if (bPinAIsTransition && bPinBIsTransition)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Cannot wire a transition to a transition"));
	}

	// Compare the directions
	bool bDirectionsOK = false;

	// Transitions are exclusive (both input and output), but states are not
	if (bPinAIsTransition)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_BREAK_OTHERS_A, TEXT(""));
	}
	else if (bPinBIsTransition)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_BREAK_OTHERS_B, TEXT(""));
	}
	else if (!bPinAIsTransition && !bPinBIsTransition)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_MAKE_WITH_CONVERSION_NODE, TEXT("Create a transition"));
	}
	else
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, TEXT(""));
	}
	
	return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, TEXT("NotConnect"));
}

bool UDialogueBuilderGraphSchema::CreateAutomaticConversionNodeAndConnections(UEdGraphPin* PinA, UEdGraphPin* PinB) const
{
	UStateNodeDB* NodeA = Cast<UStateNodeDB>(PinA->GetOwningNode());
	UStateNodeDB* NodeB = Cast<UStateNodeDB>(PinB->GetOwningNode());

	if ((NodeA != NULL) && (NodeB != NULL)
		&& (NodeA->GetInputPin() != NULL) && (NodeA->GetOutputPin() != NULL)
		&& (NodeB->GetInputPin() != NULL) && (NodeB->GetOutputPin() != NULL))
	{
		UTransitionNodeDB* TransitionNode = FEdGraphSchemaAction_NewNode::SpawnNodeFromTemplate<UTransitionNodeDB>(NodeA->GetGraph(), NewObject<UTransitionNodeDB>(), FVector2D(0.0f, 0.0f), false);

		if (PinA->Direction == EGPD_Output)
		{
			TransitionNode->CreateConnections(NodeA, NodeB);
		}
		else
		{
			TransitionNode->CreateConnections(NodeB, NodeA);
		}

		return true;
	}
	
	return false;
}


#undef LOCTEXT_NAMESPACE