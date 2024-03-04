// Copyright 2022 Greg Shynkar. All Rights Reserved

#include "DialogueBuilder.h"


static const FName DialogueBuilderTabName("DialogueBuilder");

#define LOCTEXT_NAMESPACE "FDialogueBuilderModule"

void FDialogueBuilderModule::StartupModule()
{
}

void FDialogueBuilderModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FDialogueBuilderModule, DialogueBuilder)