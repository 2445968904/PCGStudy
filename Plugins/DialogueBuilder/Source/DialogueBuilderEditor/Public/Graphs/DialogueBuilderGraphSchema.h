// Copyright 2022 Greg Shynkar. All Rights Reserved

#pragma once

#include "DialogueBuilderGSchemaActions.h"
#include "Graphs/Nodes/EntryNodeDB.h"
#include "Graphs/Nodes/TransitionNodeDB.h"
#include "EdGraph/EdGraphSchema.h"
#include "ToolMenu.h"
#include "DialogueBuilderGraphCommands.h"
#include "GraphEditorActions.h"

#include "DialogueBuilderGraphSchema.generated.h"


UCLASS(MinimalAPI)
class UDialogueBuilderGraphSchema : public UEdGraphSchema
{
	GENERATED_BODY()

public:
	// Get all actions that can be performed when right clicking on a graph or drag-releasing on a graph from a pin
	virtual void GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const override;

	// Node's menu
	virtual void GetContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const override;

	// Populate new graph with any default nodes
	virtual void CreateDefaultNodesForGraph(UEdGraph& Graph) const override;
	
	// Try to make a connection between two pins
	virtual bool TryCreateConnection(UEdGraphPin* PinA, UEdGraphPin* PinB) const override;

	// Determine if a connection can be created between two pins
	virtual const FPinConnectionResponse CanCreateConnection(const UEdGraphPin* PinA, const UEdGraphPin* PinB) const override;

	// Auto creation Transition Node between StateNodes
	virtual bool CreateAutomaticConversionNodeAndConnections(UEdGraphPin* PinA, UEdGraphPin* PinB) const override;
};