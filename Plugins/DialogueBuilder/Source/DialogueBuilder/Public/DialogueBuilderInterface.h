// Copyright 2023 Greg Shynkar. All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "DialogueBuilderInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UDialogueBuilderInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class DIALOGUEBUILDER_API IDialogueBuilderInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintImplementableEvent,BlueprintCallable, Category="DialogueInterfaceFunction")
	void BreakDialogueNodeDelay();

};
