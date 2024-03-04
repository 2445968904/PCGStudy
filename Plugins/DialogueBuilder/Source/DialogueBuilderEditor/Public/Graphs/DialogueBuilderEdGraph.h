// Copyright 2022 Greg Shynkar. All Rights Reserved

#pragma once

#include "DialogueBuilderObject.h"
#include "EdGraph/EdGraph.h"
#include "GraphEditAction.h"
#include "DialogueBuilderEdGraph.generated.h"


UCLASS()
class DIALOGUEBUILDEREDITOR_API UDialogueBuilderEdGraph : public UEdGraph
{
	GENERATED_BODY()

public:

	//Start Set up data to DialogueData at DialogueBuilderObject;
	void UpdateElementAtData(FDialogueDetailsStruct Data);
	void AddEntryNodeData();
	void AddElementToData(FDialogueDetailsStruct Data);
	void RemoveNodeFromData(int32 NodeId);
	//End Set up data to DialogueData at DialogueBuilderObject;

	// Related Functions to make ID for every node;
	int32 GetIdCount();
	void AddIdCount();

	UPROPERTY()
	UEdGraphNode* Entry;
	UPROPERTY()
	UDialogueBuilderObject* DialogueBuilderObject;
	UPROPERTY()
	int32 NodeIdCount = -1;

};
