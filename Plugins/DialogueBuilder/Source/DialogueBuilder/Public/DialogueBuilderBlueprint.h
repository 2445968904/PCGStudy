// Copyright 2022 Greg Shynkar. All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Engine/Blueprint.h"
#include "UObject/ObjectMacros.h"
#include "DialogueBuilderBlueprint.generated.h"


UCLASS()
class DIALOGUEBUILDER_API UDialogueBuilderBlueprint: public UBlueprint
{
	GENERATED_BODY()

public:
	// Pointers of UEdGraphs to check that blueprint is created;

	UPROPERTY()
	UEdGraph* DialogueBuilderGraph;
	
	UPROPERTY()
	UEdGraph* EvenGraphRef;

};


