// Copyright 2020-2024 Maksym Maisak. All Rights Reserved.

#include "HTNNode.h"

#include "AIController.h"
#include "Tasks/AITask.h"
#include "GameplayTasksComponent.h"
#include "VisualLogger/VisualLogger.h"

UHTNNode::UHTNNode(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer),
#if WITH_EDITORONLY_DATA
	NodeIndexInGraph(INDEX_NONE),
#endif
	bCreateNodeInstance(false),
	bOwnsGameplayTasks(false),
	bNotifyOnPlanExecutionStarted(false),
	bNotifyOnPlanExecutionFinished(false),
	bForceUsingPlanningWorldState(false),
	HTNAsset(nullptr),
	OwnerComponent(nullptr)
{}

#if WITH_ENGINE

UWorld* UHTNNode::GetWorld() const
{
	if (OwnerComponent)
	{
		return OwnerComponent->GetWorld();
	}
	
	if (GetOuter())
	{
		// Special case for htn nodes in the editor
		if (Cast<UPackage>(GetOuter()))
		{
			// GetOuter should return a UPackage and its Outer is a UWorld
			return Cast<UWorld>(GetOuter()->GetOuter());
		}

		return GetOuter()->GetWorld();
	}

	return nullptr;
}

#endif

void UHTNNode::InitializeFromAsset(UHTN& Asset)
{
	HTNAsset = &Asset;
}

void UHTNNode::InitializeSpecialMemory(UHTNComponent& OwnerComp, uint8* NodeMemory, const FHTNPlan& Plan, const FHTNPlanStepID& StepID) const
{
	if (bCreateNodeInstance)
	{
		new(GetSpecialNodeMemory<FHTNNodeSpecialMemory>(NodeMemory)) FHTNNodeSpecialMemory;
	}
}

void UHTNNode::InitializeInPlan(UHTNComponent& OwnerComp, uint8* NodeMemory, 
	const FHTNPlan& Plan, const FHTNPlanStepID& StepID, 
	TArray<UHTNNode*>& OutNodeInstances) const
{
	FHTNNodeSpecialMemory* const SpecialMemory = GetSpecialNodeMemory<FHTNNodeSpecialMemory>(NodeMemory);
	if (SpecialMemory)
	{
		SpecialMemory->NodeInstanceIndex = INDEX_NONE;
	}

	if (!bCreateNodeInstance)
	{
		InitializeSpecialMemory(OwnerComp, NodeMemory, Plan, StepID);
		InitializeMemory(OwnerComp, NodeMemory, Plan, StepID);
	}
	else
	{
		SCOPE_CYCLE_COUNTER(STAT_AI_HTN_NodeInstantiation);
		UHTNNode* const NodeInstance = DuplicateObject(this, &OwnerComp);
		INC_DWORD_STAT(STAT_AI_HTN_NumNodeInstances);

		check(HTNAsset);
		check(SpecialMemory);
		
		NodeInstance->TemplateNode = const_cast<UHTNNode*>(this);
		NodeInstance->InitializeFromAsset(*HTNAsset);
		NodeInstance->SetOwnerComponent(&OwnerComp);
		NodeInstance->InitializeSpecialMemory(OwnerComp, NodeMemory, Plan, StepID);
		NodeInstance->InitializeMemory(OwnerComp, NodeMemory, Plan, StepID);

		SpecialMemory->NodeInstanceIndex = OutNodeInstances.Add(NodeInstance);
	}
}

void UHTNNode::CleanupInPlan(UHTNComponent& OwnerComp, uint8* NodeMemory) const
{
	UHTNNode* const Node = GetNodeFromMemory(OwnerComp, NodeMemory);
	if (!ensure(Node))
	{
		return;
	}

	Node->CleanupMemory(OwnerComp, NodeMemory);
	Node->CleanupSpecialMemory(OwnerComp, NodeMemory);

	// If the Node is an instance created just for this plan, explicitly mark it as pending kill. 
	// This makes IsValid() checks fail for the Node. 
	// 
	// This is necessary because at this point there may be outstanding latent actions for this node.
	// For example, if the node started a latent action OnPlanExecutionStart but the plan got aborted before the node begins execution,
	// so OnTaskFinished/OnExecutionFinish never got called to cancel the latent actions. 
	// Or the node did try to cancel its latent actions but that didn't work because 
	// FLatentActionManager::RemoveActionsForObject doesn't guarantee removal of the actions 
	// if we're in the middle of processing latent actions (e.g., the Tick of the HTNComponent).
	// 
	// Killing the node instance completely is a more reliable way of ensuring that 
	// FLatentActionManager won't execute any of that instance's pending latent actions, 
	// compared to FLatentActionManager::RemoveActionsForObject.
	if (Node->IsInstance())
	{
#if ENGINE_MAJOR_VERSION < 5
		Node->MarkPendingKill();
#else
		Node->MarkAsGarbage();
#endif
	}
}

UHTNNode* UHTNNode::GetNodeFromMemory(const UHTNComponent& OwnerComp, const uint8* NodeMemory) const
{
	return OwnerComp.GetNodeFromMemory(this, NodeMemory);
}

void UHTNNode::WrappedOnPlanExecutionStarted(UHTNComponent& OwnerComp, uint8* NodeMemory) const
{
	check(!IsInstance());
	if (bNotifyOnPlanExecutionStarted)
	{
		UHTNNode* const Node = GetNodeFromMemory(OwnerComp, NodeMemory);
		if (ensure(Node))
		{
			UE_VLOG(&OwnerComp, LogHTN, VeryVerbose, TEXT("on plan execution started: '%s'"), *Node->GetShortDescription());
			Node->OnPlanExecutionStarted(OwnerComp, NodeMemory);
		}
	}
}

void UHTNNode::WrappedOnPlanExecutionFinished(UHTNComponent& OwnerComp, uint8* NodeMemory, EHTNPlanExecutionFinishedResult Result) const
{
	check(!IsInstance());
	if (bNotifyOnPlanExecutionFinished)
	{
		UHTNNode* const Node = GetNodeFromMemory(OwnerComp, NodeMemory);
		if (ensure(Node))
		{
			UE_VLOG(&OwnerComp, LogHTN, VeryVerbose, TEXT("on plan execution finished: '%s'"), *Node->GetShortDescription());
			Node->OnPlanExecutionFinished(OwnerComp, NodeMemory, Result);
		}
	}
}

FString UHTNNode::GetStaticDescription() const
{
	if (GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint))
	{
		return GetClass()->GetName().LeftChop(2);
	}

	return GetSubStringAfterUnderscore(GetClass()->GetName());
}

FString UHTNNode::GetNodeName() const
{
	if (NodeName.Len())
	{
		return NodeName;
	}
	
	const FString ClassName = GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint) ? 
		GetClass()->GetName().LeftChop(2) :
		GetClass()->GetName();

	return GetSubStringAfterUnderscore(ClassName);
}

FString UHTNNode::GetShortDescription() const
{
	return FString::Printf(TEXT("%s (of %s)"), *GetNodeName(), *GetNameSafe(GetSourceHTNAsset()));
}

UHTN* UHTNNode::GetSourceHTNAsset() const
{
	if (const UHTNNode* const NodeInAsset = GetTemplateNode())
	{
		UHTN* const SourceHTNAsset = NodeInAsset->GetTypedOuter<UHTN>();
		ensureMsgf(SourceHTNAsset, TEXT("Could not find source HTN asset of HTN node %s"), *GetNodeName());
		return SourceHTNAsset;
	}

	return nullptr;
}

UGameplayTasksComponent* UHTNNode::GetGameplayTasksComponent(const UGameplayTask& Task) const
{
	if (const UAITask* const AITask = Cast<UAITask>(&Task))
	{
		if (AAIController* const AIController = AITask->GetAIController())
		{
			return AIController->GetGameplayTasksComponent(Task);
		}
	}

	if (OwnerComponent)
	{
		return OwnerComponent->GetGameplayTasksComponent(Task);
	}

	return Task.GetGameplayTasksComponent();
}

AActor* UHTNNode::GetGameplayTaskOwner(const UGameplayTask* Task) const
{
	if (!Task)
	{
		if (/*IsInstanced()*/true)
		{
			const UHTNComponent* const HTNComponent = Cast<const UHTNComponent>(GetOuter());
			check(HTNComponent);
			return HTNComponent->GetAIOwner();
		}
		else
		{
			UE_LOG(LogHTN, Warning, TEXT("%s: Unable to determine default GameplayTaskOwner!"), *GetName());
			return nullptr;
		}
	}

	if (const UAITask* AITask = Cast<const UAITask>(Task))
	{
		return AITask->GetAIController();
	}

	if (const UGameplayTasksComponent* const TasksComponent = Task->GetGameplayTasksComponent())
	{
		return TasksComponent->GetGameplayTaskOwner(Task);
	}
	
	return nullptr;
}

AActor* UHTNNode::GetGameplayTaskAvatar(const UGameplayTask* Task) const
{
	if (!Task)
	{
		if (/*IsInstanced()*/true)
		{
			const UHTNComponent* const HTNComponent = Cast<const UHTNComponent>(GetOuter());
			check(HTNComponent);
			return HTNComponent->GetAIOwner();
		}
		else
		{
			UE_LOG(LogHTN, Warning, TEXT("%s: Unable to determine default GameplayTaskAvatar!"), *GetName());
			return nullptr;
		}
	}

	if (const UAITask* AITask = Cast<const UAITask>(Task))
	{
		return AITask->GetAIController() ? AITask->GetAIController()->GetPawn() : nullptr;
	}

	if (const UGameplayTasksComponent* const TasksComponent = Task->GetGameplayTasksComponent())
	{
		return TasksComponent->GetGameplayTaskAvatar(Task);
	}

	return nullptr;
}

uint8 UHTNNode::GetGameplayTaskDefaultPriority() const { return StaticCast<uint8>(EAITaskPriority::AutonomousAI); }

void UHTNNode::OnGameplayTaskInitialized(UGameplayTask& Task)
{
	if (const UAITask* const AITask = Cast<const UAITask>(&Task))
	{
		if (!AITask->GetAIController())
		{
			// this means that the AI task was either
			// created without specifying UAITask::OwnerController's value (like via BP's Construct Object node)
			// or it was created in C++ with an inappropriate function.
			UE_LOG(LogHTN, Error, TEXT("Missing AIController in AITask %s"), *AITask->GetName());
		}
	}
}

UHTNComponent* UHTNNode::GetHTNComponentByTask(UGameplayTask& Task) const
{
	if (UAITask* const AITask = Cast<UAITask>(&Task))
	{
		if (AAIController* const AIController = AITask->GetAIController())
		{
			return Cast<UHTNComponent>(AIController->BrainComponent);
		}
	}

	return nullptr;
}

FString UHTNNode::GetSubStringAfterUnderscore(const FString& Input)
{
	const int32 Index = Input.Find(TEXT("_"));
	if (Index != INDEX_NONE)
	{
		return Input.Mid(Index + 1);
	}

	return Input;
}

void UHTNNode::Replan(UHTNComponent& OwnerComp, uint8* NodeMemory, const FHTNReplanParameters& Params) const
{
	if (!IsValid(this))
	{
		UE_VLOG_UELOG(&OwnerComp, LogHTN, Warning, 
			TEXT("'%s' called Replan while no longer valid. Ignoring."),
			*GetShortDescription());
		return;
	}

	UE_VLOG(&OwnerComp, LogHTN, Log, 
		TEXT("'%s' called Replan %s"), 
		*GetShortDescription(), 
		*Params.ToCompactString());

	if (Params.bReplanOutermostPlanInstance)
	{
		OwnerComp.GetRootPlanInstance().Replan(Params);
	}
	else
	{
		// Fallback in case user code did not provide NodeMemory.
		// Find NodeMemory belonging to the node. 
		// This may return the wrong memory if the node is present in the plan multiple times,
		// so providing NodeMemory is preferred.
		if (!NodeMemory)
		{
			const FHTNNodeInPlanInfo Info = OwnerComp.FindActiveNodeInfo(this);
			NodeMemory = Info.NodeMemory;

			UE_CVLOG(Info.IsValid(), &OwnerComp, LogHTN, Log,
				TEXT("'%s' called Replan without providing NodeMemory, found NodeMemory in plan instance %s"),
				*GetShortDescription(),
				*Info.PlanInstance->GetLogPrefix());

			UE_CVLOG_UELOG(!Info.IsValid(), &OwnerComp, LogHTN, Error,
				TEXT("'%s' called Replan without providing NodeMemory, could not find NodeMemory"),
				*GetShortDescription());
		}

		if (NodeMemory)
		{
			if (UHTNPlanInstance* const PlanInstance = OwnerComp.FindPlanInstanceByNodeMemory(NodeMemory))
			{
				return PlanInstance->Replan(Params);
			}
			else
			{
				UE_VLOG_UELOG(&OwnerComp, LogHTN, Error,
					TEXT("'%s' Replan could not find an active plan instance to which the node belongs."),
					*GetShortDescription());
			}
		}
	}
}

UHTNNode::FGuardOwnerComponent::FGuardOwnerComponent(const UHTNNode& Node, UHTNComponent* NewOwner) : 
	Node(Node), 
	OldOwner(Node.GetOwnerComponent())
{
	Node.SetOwnerComponent(NewOwner);
}

UHTNNode::FGuardOwnerComponent::~FGuardOwnerComponent()
{
	Node.SetOwnerComponent(OldOwner);
}
