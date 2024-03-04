// Copyright 2020-2024 Maksym Maisak. All Rights Reserved.

#pragma once

#include "HTNStandaloneNode.h"
#include "HTNNode_Optional.generated.h"

// Everything after this node is optional. 
// The planner will try to make a plan that includes tasks after this. 
// If that fails, it will exclude those tasks from the final plan instead of failing the whole planning.
// Identical to having a Prefer node that only uses the top branch.
UCLASS()
class HTN_API UHTNNode_Optional : public UHTNStandaloneNode
{
	GENERATED_BODY()
	
public:
	UHTNNode_Optional(const FObjectInitializer& Initializer);
	virtual void MakePlanExpansions(FHTNPlanningContext& Context) override;
	virtual void GetNextPrimitiveSteps(FHTNGetNextStepsContext& Context, const FHTNPlanStepID& ThisStepID) override;
};
