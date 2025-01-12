// Copyright 2023 Leartes Studios. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

class FAutoSplineModule : public IModuleInterface
{
public:

	/* IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
};
