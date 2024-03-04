// Copyright 2020-2024 Maksym Maisak. All Rights Reserved.

#include "Nodes/HTNNode_SubNetworkDynamic.h"
#include "AITask_MakeHTNPlan.h"
#include "Misc/StringBuilder.h"
#include "VisualLogger/VisualLogger.h"

UHTNNode_SubNetworkDynamic::UHTNNode_SubNetworkDynamic(const FObjectInitializer& Initializer) : Super(Initializer),
	DefaultHTN(nullptr),
	bTakeHTNFromWorldstateKey(false)
{
	TakeHTNFromWorldstateKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, TakeHTNFromWorldstateKey), UHTN::StaticClass());
	TakeHTNFromWorldstateKey.AllowNoneAsValue(true);
}

void UHTNNode_SubNetworkDynamic::InitializeFromAsset(UHTN& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (const UBlackboardData* const BBAsset = GetBlackboardAsset())
	{
		TakeHTNFromWorldstateKey.ResolveSelectedKey(*BBAsset);
	}
	else
	{
		UE_LOG(LogHTN, Warning, TEXT("Can't initialize task: %s, make sure that the HTN specifies a Blackboard asset!"), *GetShortDescription());
	}
}

void UHTNNode_SubNetworkDynamic::MakePlanExpansions(FHTNPlanningContext& Context)
{	
	FHTNPlanStep* AddedStep = nullptr;
	FHTNPlanStepID AddedStepID;
	const TSharedRef<FHTNPlan> NewPlan = Context.MakePlanCopyWithAddedStep(AddedStep, AddedStepID);

	// If the subnetwork is valid, add a sublevel. Otherwise just skip this node and move to the next.
	const UHTNComponent& OwnerComp = *Context.PlanningTask->GetOwnerComponent();
	UHTN* const HTN = GetHTN(OwnerComp, *Context.WorldStateAfterEnteringDecorators);
	if (IsHTNValid(OwnerComp, HTN))
	{
		AddedStep->SubLevelIndex = Context.AddLevel(*NewPlan, HTN, AddedStepID);
	}
	
	Context.SubmitCandidatePlan(NewPlan);
}

void UHTNNode_SubNetworkDynamic::GetNextPrimitiveSteps(FHTNGetNextStepsContext& Context, const FHTNPlanStepID& ThisStepID)
{
	const FHTNPlanStep& Step = Context.Plan.GetStep(ThisStepID);
	Context.AddFirstPrimitiveStepsInLevel(Step.SubLevelIndex);
}

FString UHTNNode_SubNetworkDynamic::GetNodeName() const
{
	if (NodeName.Len())
	{
		return Super::GetNodeName();
	}

	if (bTakeHTNFromWorldstateKey)
	{
		return FString::Printf(TEXT("SubNetwork from key: %s"), *TakeHTNFromWorldstateKey.SelectedKeyName.ToString());
	}
	else if (IsValid(DefaultHTN))
	{
		return GetSubStringAfterUnderscore(DefaultHTN->GetName());
	}

	return Super::GetNodeName();
}

FString UHTNNode_SubNetworkDynamic::GetStaticDescription() const
{
	TStringBuilder<1024> SB;

	SB << Super::GetStaticDescription() << TEXT(":");

	if (bTakeHTNFromWorldstateKey)
	{
		SB << TEXT("\nTakes HTN from worldstate key: ") << TakeHTNFromWorldstateKey.SelectedKeyName.ToString();
	}
	else
	{
		SB << TEXT("\nInjection tag: ") << InjectTag.ToString();
	}

	SB << TEXT("\nDefault: ") << GetNameSafe(DefaultHTN);

	return *SB;
}

#if WITH_EDITOR

FName UHTNNode_SubNetworkDynamic::GetNodeIconName() const
{
	return FName(TEXT("BTEditor.Graph.BTNode.Task.RunBehavior.Icon"));
}

UObject* UHTNNode_SubNetworkDynamic::GetAssetToOpenOnDoubleClick(const UHTNComponent* DebuggedHTNComponent) const
{
	UHTN* DynamicSubHTN = DefaultHTN;
	if (DebuggedHTNComponent)
	{
		const UBlackboardComponent* const BlackboardComponent = DebuggedHTNComponent->GetBlackboardComponent();
		const FBlackboardWorldState Worldstate = BlackboardComponent ? 
			FBlackboardWorldState(*const_cast<UBlackboardComponent*>(BlackboardComponent)) : 
			FBlackboardWorldState();
		DynamicSubHTN = GetHTN(*DebuggedHTNComponent, Worldstate);
	}

	return DynamicSubHTN;
}

#endif

UHTN* UHTNNode_SubNetworkDynamic::GetHTN(const UHTNComponent& OwnerComp, const FBlackboardWorldState& WorldState) const
{
	if (bTakeHTNFromWorldstateKey)
	{
		if (UHTN* const HTNFromWorldstate = Cast<UHTN>(WorldState.GetValue<UBlackboardKeyType_Object>(TakeHTNFromWorldstateKey.GetSelectedKeyID())))
		{
			return HTNFromWorldstate;
		}
	}
	else if (const UHTNExtension_SubNetworkDynamic* const Extension = OwnerComp.FindExtension<UHTNExtension_SubNetworkDynamic>())
	{
		if (UHTN* const InjectedHTN = Extension->GetDynamicHTN(InjectTag))
		{
			return InjectedHTN;
		}
	}

	return DefaultHTN;
}

bool UHTNNode_SubNetworkDynamic::IsHTNValid(const UHTNComponent& OwnerComp, const UHTN* HTN) const
{
	if (IsValid(HTN) && IsValid(HTN->BlackboardAsset) && HTN->StartNodes.Num())
	{
		if (const UBlackboardComponent* const BlackboardComponent = OwnerComp.GetBlackboardComponent())
		{
			return BlackboardComponent->IsCompatibleWith(HTN->BlackboardAsset);
		}
	}

	return false;
}

#if ENABLE_VISUAL_LOG

void UHTNExtension_SubNetworkDynamic::DescribeSelfToVisLog(FVisualLogEntry* Snapshot) const
{
	FVisualLogStatusCategory Category(TEXT("HTN SubNetwork Dynamic"));
	for (const TPair<FGameplayTag, UHTN*>& Pair : GameplayTagToDynamicHTN)
	{
		Category.Add(*Pair.Key.ToString(), *GetNameSafe(Pair.Value));
	}
	Snapshot->Status.Add(MoveTemp(Category));
}

#endif

UHTN* UHTNExtension_SubNetworkDynamic::GetDynamicHTN(FGameplayTag InjectTag) const
{
	return GameplayTagToDynamicHTN.FindRef(InjectTag);
}

bool UHTNExtension_SubNetworkDynamic::SetDynamicHTN(FGameplayTag InjectTag, UHTN* HTN, 
	bool bForceReplanIfChangedNodesInCurrentPlan, bool bForceReplanIfChangedDuringPlanning, bool bForceAbortCurrentPlanIfForcingReplan)
{
	UHTN* const PreviousHTN = GameplayTagToDynamicHTN.FindRef(InjectTag);
	if (PreviousHTN == HTN)
	{
		return false;
	}

	if (HTN)
	{
		GameplayTagToDynamicHTN.Add(InjectTag, HTN);
	}
	else
	{
		GameplayTagToDynamicHTN.Remove(InjectTag);
	}

	FHTNReplanParameters ReplanParams;
	ReplanParams.bForceAbortPlan = bForceAbortCurrentPlanIfForcingReplan;
	ReplanParams.bForceRestartActivePlanning = true;
	ReplanParams.DebugReason = TEXT("SetDynamicHTN");

	bool bCalledForceReplan = false;

	// Make a new plan if this change affects any SubNetworkDynamic nodes in the current plan, or if there is an ongoing planning process.
	if (UHTNComponent* const HTNComponent = GetHTNComponent())
	{
		if (bForceReplanIfChangedNodesInCurrentPlan)
		{
			HTNComponent->ForEachPlanInstance([&](UHTNPlanInstance& PlanInstance)
			{
				if (const TSharedPtr<const FHTNPlan> CurrentPlan = PlanInstance.GetCurrentPlan())
				{
					for (const TSharedPtr<FHTNPlanLevel>& Level : CurrentPlan->Levels)
					{
						for (const FHTNPlanStep& Step : Level->Steps)
						{
							if (UHTNNode_SubNetworkDynamic* const DynamicSubNetworkNode = Cast<UHTNNode_SubNetworkDynamic>(Step.Node.Get()))
							{
								// Do an check on the InjectTag, because tag hierarchy is not supported for this.
								if (!DynamicSubNetworkNode->bTakeHTNFromWorldstateKey && DynamicSubNetworkNode->InjectTag.MatchesTagExact(InjectTag))
								{
									const UHTN* const PreviousHTNForNode = PreviousHTN ? PreviousHTN : DynamicSubNetworkNode->DefaultHTN;
									const UHTN* const NewHTNForNode = HTN ? HTN : DynamicSubNetworkNode->DefaultHTN;
									if (PreviousHTNForNode != NewHTNForNode)
									{
										UE_VLOG_UELOG(HTNComponent, LogHTN, Log, 
											TEXT("Triggering Replan because the Dynamic HTN (tag %s) in the current plan was changed from %s to %s"),
											*InjectTag.ToString(), *GetNameSafe(PreviousHTNForNode), *GetNameSafe(NewHTNForNode));
										PlanInstance.Replan(ReplanParams);
										bCalledForceReplan = true;
										return;
									}
								}
							}
						}
					}
				}
			});
		}
	
		if (bForceReplanIfChangedDuringPlanning)
		{
			HTNComponent->ForEachPlanInstance([&](UHTNPlanInstance& PlanInstance)
			{
				if (UAITask_MakeHTNPlan* const PlanningTask = PlanInstance.GetCurrentPlanningTask())
				{
					UE_VLOG_UELOG(HTNComponent, LogHTN, Log, TEXT("Restarting planning process because the Dynamic HTN (tag %s) was changed from %s to %s"),
						*InjectTag.ToString(), *GetNameSafe(PreviousHTN), *GetNameSafe(HTN));
					PlanInstance.Replan(ReplanParams);
					bCalledForceReplan = true;
				}
			});
		}
	}

	return bCalledForceReplan;
}

void UHTNExtension_SubNetworkDynamic::ResetDynamicHTNs()
{
	GameplayTagToDynamicHTN.Reset();
}
