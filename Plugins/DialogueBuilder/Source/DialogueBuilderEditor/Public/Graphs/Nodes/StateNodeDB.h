// Copyright 2022 Greg Shynkar. All Rights Reserved

#pragma once

#include "Graphs/DialogueBuilderEdGraph.h"
#include "Animation/AnimSequence.h"
#include "K2Node_CustomEvent.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "StateNodeDB.generated.h"


UCLASS(MinimalAPI)
class UStateNodeDB : public UEdGraphNode
{
	GENERATED_BODY()

public:
	//~ Begin UObject Interface
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	//~ End UObject Interface

	//~ Begin UEdGraphNode Interface 
	virtual void AllocateDefaultPins() override;
	virtual void AutowireNewNode(UEdGraphPin* FromPin) override;
	void PinConnectionListChanged(UEdGraphPin* Pin) override;
//	virtual void NodeConnectionListChanged() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;
	virtual void DestroyNode() override;
	//~ End UEdGraphNode Interface 

	virtual UEdGraphPin* GetInputPin() const; // Pins[0]
	virtual UEdGraphPin* GetOutputPin() const; // Pins[1]

	// Set this DialogueNodeText from FDialogueBGSchemaGraphNodeActions while performing action;
	void SetName(FText NewName);

	// Get Index of connected DialogueNode at NextNodesId array;
	int32 GetFunctionListindex(int32 DialogueNodeID);

	// SetUp Dialogue ID number;
	void SetStartValues();

	// SetUp Dialogue Info to collect in DialogueData at UDialogueObject;
	void UpdateNodeAtDialData();

	// Destroy related TransitionNodes when StateNodeDB deleted; 
	void DestroyTransitionNodes();

	// Break Connection in DialogueData. Goes When StateNodeDB deleted first.
	void PostBreakConnectionWithNextState(UEdGraphPin* Pin);

	// Break Connection in DialogueData. Goes When TransitionNode deleted first.
	void BreakConnectionWithNextState(int32 ConnectNodeID);


	/** Visible part at Details Tab; */

	UPROPERTY(VisibleDefaultsOnly, Category = "Dialogue Node | Default")
	int32 DialogueNodeId = -1;

	UPROPERTY(EditAnywhere, Category = "Dialogue Node | Default")
	FText DialogueNodeText;

	UPROPERTY(EditAnywhere, Category = "Dialogue Node | Default")
	bool IsPlayerNode;

	UPROPERTY(EditAnywhere, Category = "Dialogue Node | Default")
	bool IsPlayerAnswerOption;

	UPROPERTY(EditAnywhere, Category = "Dialogue Node | Default")
	FText SpeakerName;
	
	UPROPERTY(EditAnywhere, Category = "Dialogue Node | Default")
	UTexture2D* SpeakerTexture = nullptr;

	UPROPERTY(EditAnywhere, Category = "Dialogue Node | Default")
	bool IsSwitchNode;

	UPROPERTY(EditAnywhere, Category = "Dialogue Node | Default")
	float DelayToMoveNextNodes = 0.2;

	UPROPERTY(EditAnywhere, Category = "Dialogue Node | Default")
	class USoundBase* DialogueAudio = nullptr;

	UPROPERTY(EditAnywhere, Category = "Dialogue Node | Default")
	class UDialogueWave* DialogueWave = nullptr;

	UPROPERTY(EditAnywhere, Category = "Dialogue Node | Default")
	float BeginPlayDelay = 0;

	UPROPERTY(EditAnywhere, Category = "Dialogue Node | Animation")
	UAnimSequence* AnimToUpdate = nullptr;

	UPROPERTY(EditAnywhere, Category = "Dialogue Node | Animation")
	UAnimSequence* AnimToUpdateHead = nullptr;

	UPROPERTY(EditAnywhere, Category = "Dialogue Node | Animation")
	bool IsItEndNode;

	UPROPERTY(EditAnywhere, Category = "Dialogue Node | Type Writer Text Effect")
	bool WriteEffect;

	UPROPERTY(EditAnywhere, Category = "Dialogue Node | Type Writer Text Effect")
	float WriteEffectSpeed;

	UPROPERTY(EditAnywhere, Category = "Dialogue Node | Timed Decision")
	bool IsTimedDecisions;
	
	UPROPERTY(EditAnywhere, Category = "Dialogue Node | Timed Decision")
	float TimedDecisionsTime;


	/** Non-Visible / Connection part at Details Tab */

	// Array of Dialogues Node ID numbers connected to this Node;
	UPROPERTY()
	TArray <int32> NextNodesId; 
	
	// Array of Transition Function Names for this Node. Order Proportional to NextNodesId ;
	UPROPERTY()
	TArray <FName> StateTransitionNameToCheck; 
	 
	// Array of PinID connected to this Node;
	UPROPERTY()
	TArray<FGuid> PinsIDArray;

	/** Data related to OnPlayDialogue Function*/
	// Sample for Transition function creation;
	UFUNCTION()
	void OnDialoguePlay(float PlayDelay);

	UPROPERTY()
	FName OnDialoguePlayFuncName;
	
	UPROPERTY()
	UK2Node_CustomEvent* NodeForJump;

	UPROPERTY()
	FGuid GraphID;
};