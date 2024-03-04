// Copyright 2022 Greg Shynkar. All Rights Reserved

#pragma once

#include "DialogueBuilderObject.h"
#include "DialogueBuilderBlueprint.h"
#include "Factories/BlueprintFactory.h"
#include "DialogueBuilderFactory.generated.h"

UCLASS()
class DIALOGUEBUILDEREDITOR_API UDialogueBuilderFactory : public UFactory
{
	GENERATED_BODY()

public: 
	// UFactory interface
	UDialogueBuilderFactory();
	virtual bool ConfigureProperties() override;
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags InFlags, UObject* InContext, FFeedbackContext* InWarn, FName InCallingContext) override;
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	// End of UFactory interface

	UClass* ParentClass;
};
