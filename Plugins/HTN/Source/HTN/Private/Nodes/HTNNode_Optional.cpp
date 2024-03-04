// Copyright 2020-2024 Maksym Maisak. All Rights Reserved.

#include "Nodes/HTNNode_Optional.h"
#include "AITask_MakeHTNPlan.h"

UHTNNode_Optional::UHTNNode_Optional(const FObjectInitializer& Initializer) : Super(Initializer)
{
	bPlanNextNodesAfterThis = false;
}

void UHTNNode_Optional::MakePlanExpansions(FHTNPlanningContext& Context)
{
	const auto MakeNewPlan = [&](bool bWithContent)
	{
		FHTNPlanStep* AddedStep = nullptr;
		FHTNPlanStepID AddedStepID;
		const TSharedRef<FHTNPlan> NewPlan = Context.MakePlanCopyWithAddedStep(AddedStep, AddedStepID);
		
		if (bWithContent)
		{
			AddedStep->SubLevelIndex = NextNodes.Num() ? Context.AddInlineLevel(*NewPlan, AddedStepID) : INDEX_NONE;
		}

		Context.SubmitCandidatePlan(NewPlan, bWithContent ? TEXT("with content") : TEXT("empty"), /*bOnlyUseIfPreviousFails=*/true);
	};

	MakeNewPlan(true);
	MakeNewPlan(false);
}

void UHTNNode_Optional::GetNextPrimitiveSteps(FHTNGetNextStepsContext& Context, const FHTNPlanStepID& ThisStepID)
{
	const FHTNPlanStep& Step = Context.Plan.GetStep(ThisStepID);
	if (Step.SubLevelIndex != INDEX_NONE)
	{
		Context.AddFirstPrimitiveStepsInLevel(Step.SubLevelIndex);
	}
}
