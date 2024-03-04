// Copyright 2022 Greg Shynkar. All Rights Reserved

#pragma once

#include "Graphs/DialogueBuilderEdGraph.h"
#include "Graphs/Nodes/StateNodeDB.h"
#include "K2Node_FunctionEntry.h"
#include "EdGraph/EdGraphNode.h"
#include "EntryNodeDB.generated.h"


UCLASS(MinimalAPI)
class UEntryNodeDB : public UEdGraphNode
{
	GENERATED_BODY()

public:
	//~ Begin UObject Interface
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	//~ End UObject Interface

	//~ Begin UEdGraphNode Interface
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;
	void PinConnectionListChanged(UEdGraphPin* Pin) override;
	//~ End UEdGraphNode Interface
	
	//Pointer Pins[0];
	virtual UEdGraphPin* GetOutputPin() const;

	//Pointer to Pins[0]->LinkedTo[0]->GetOwningNode() ;
	UEdGraphNode* GetOutputNode() const;
	
	// SetUp Dialogue Info to collect in DialogueData at UDialogueObject;
	void UpdateNodeAtDialData();


	//Start Dialogue Data Part;
	UPROPERTY()
	int32 EntryNodeId = 0;
	UPROPERTY()
	FText EntryNodeText= FText::FromString("Start Dialogue");
	UPROPERTY()
	TArray <int32> NextNodesId;
	//End Dialogue Data Part;
};
