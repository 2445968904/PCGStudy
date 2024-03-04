// Copyright 2022 Greg Shynkar. All Rights Reserved

#pragma once

#include "DialogueBuilderActions.h"
#include "Graphs/DialogueBuilderGraphFactories.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"


class FDialogueBuilderEditorModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};


