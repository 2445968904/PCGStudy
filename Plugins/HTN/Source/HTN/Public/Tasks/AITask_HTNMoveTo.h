// Copyright 2020-2024 Maksym Maisak. All Rights Reserved.

#pragma once

#include "Tasks/AITask_MoveTo.h"
#include "HTNPlanStep.h"

#include "AITask_HTNMoveTo.generated.h"

UCLASS()
class UAITask_HTNMoveTo : public UAITask_MoveTo
{
	GENERATED_BODY()

public:
	const uint8* NodeMemory = nullptr;
};