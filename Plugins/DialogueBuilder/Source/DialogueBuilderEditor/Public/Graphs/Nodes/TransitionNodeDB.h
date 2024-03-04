// Copyright 2022 Greg Shynkar. All Rights Reserved

#pragma once

#include "Graphs/Nodes/StateNodeDB.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "TransitionNodeDB.generated.h"


UCLASS(MinimalAPI)
class UTransitionNodeDB : public UEdGraphNode
{
	GENERATED_BODY()
public:

	//~ Begin UObject Interface
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	//~ End UObject Interface

	//~ Begin UEdGraphNode Interface
	virtual void AllocateDefaultPins() override;
	virtual void PinConnectionListChanged(UEdGraphPin* Pin) override;
	//virtual void NodeConnectionListChanged() override;
	virtual void DestroyNode() override;


	//~ End UEdGraphNode Interface

	virtual UEdGraphPin* GetInputPin() const; // Pins[0];
	virtual UEdGraphPin* GetOutputPin() const; // Pins[1];

	UEdGraphNode* GetPreviousState() const; // Pins[0]->LinkedTo[0]->GetOwningNode()
	UEdGraphNode* GetNextState() const; // Pins[1]->LinkedTo[0]->GetOwningNode()

	// Pins connection part;
	void CreateConnections(class UStateNodeDB* PreviousState, class UStateNodeDB* NextState);
	// Connection of Dialogue Nodes and set up it to DialogueData;
	bool ConnectStateNodesDB(class UStateNodeDB* PreviousState, class UStateNodeDB* NextState);

	// Set FunctionName From DialogueBuilderCustomEditor with Double Click on this node; 
	void SetFunctionName(FName FunctionCheckName);

	UPROPERTY()
	bool Bidirectional;

	// Start Connection Variables Part;
	UPROPERTY()
	bool IsHasFunction;

	UPROPERTY()
	FName ConnectFunctionName;

	UPROPERTY()
	class UEdGraph* FunctionGraph;

	UPROPERTY()
	int32 PreviousNodeID;

	UPROPERTY()
	int32 NextNodeID;
	// End Connection Variables Part;
};