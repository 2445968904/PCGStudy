// Copyright 2022 Greg Shynkar. All Rights Reserved

#include "DialogueBuilderFactory.h"

UDialogueBuilderFactory::UDialogueBuilderFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;

	//Configure the class that this factory creates
	SupportedClass = UDialogueBuilderBlueprint::StaticClass();
	ParentClass = UActorComponent::StaticClass();
}

bool UDialogueBuilderFactory::ConfigureProperties()
{
	return true;
}

UObject* UDialogueBuilderFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags InFlags, UObject* InContext, FFeedbackContext* InWarn, FName InCallingContext)
{
	return FactoryCreateNew(InClass, InParent, InName, InFlags, InContext, InWarn);
}

UObject* UDialogueBuilderFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	check(InClass->IsChildOf(UDialogueBuilderBlueprint::StaticClass()));

	UDialogueBuilderBlueprint* NewBP = NewObject<UDialogueBuilderBlueprint>(InParent, InClass, InName, Flags);

	NewBP->ParentClass = UDialogueBuilderObject::StaticClass();
	NewBP->BlueprintType = BPTYPE_Normal;
	NewBP->bIsNewlyCreated = true;
  	NewBP->bLegacyNeedToPurgeSkelRefs = false;
  	NewBP->GenerateNewGuid();

	return  NewBP;
}
