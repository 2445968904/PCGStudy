// Copyright 2023 Leartes Studios. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"


class FObjectDistributionModule : public IModuleInterface
{
public:

	/* IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;


#if WITH_EDITOR
	
private:
	static void RegisterCustomDetailClasses();
	static void UnregisterCustomDetailClasses();

#endif

};
