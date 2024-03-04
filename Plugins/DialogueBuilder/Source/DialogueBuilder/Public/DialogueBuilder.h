// Copyright 2022 Greg Shynkar. All Rights Reserved

#pragma once

#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"


class FDialogueBuilderModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
};
