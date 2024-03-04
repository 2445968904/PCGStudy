// Copyright 2022 Greg Shynkar. All Rights Reserved

#include "DialogueBuilderCustomEditor.h"

#define LOCTEXT_NAMESPACE "DialogueBuilderCustomEditor"

// For backwards compatibility we keep names here
const FName FDialogueBuilderCustomEditorModes::DialogueBuilderEditorMode1("DialogueBuilderEditorMode1");	
const FName FDialogueBuilderCustomEditorModes::DialogueBuilderEditorMode2("DialogueBuilderEditorMode2");

/**Constructor */
FDialogueBuilderCustomEditor::FDialogueBuilderCustomEditor()
{
	
}

/** Destructor */
FDialogueBuilderCustomEditor::~FDialogueBuilderCustomEditor()
{

}

void FDialogueBuilderCustomEditor::InitDialogueAssetEditor(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost,TArray<class UDialogueBuilderBlueprint*>& InBlueprints, bool bShouldOpenInDefaultsMode)
{
	check(InBlueprints.Num() == 1 || bShouldOpenInDefaultsMode);
	
	// TRUE if a single Blueprint is being opened and is marked as newly created
	bool bNewlyCreated = InBlueprints.Num() == 1 && InBlueprints[0]->bIsNewlyCreated;
	

	TArray< UObject* > Objects;
	for (UBlueprint* Blueprint : InBlueprints)
	{
		// Flag the blueprint as having been opened
		Blueprint->bIsNewlyCreated = false;

		Objects.Add(Blueprint);
	}

	if (!Toolbar.IsValid())
	{
		Toolbar = MakeShareable(new FBlueprintEditorToolbar(SharedThis(this)));
	}

	GetToolkitCommands()->Append(FPlayWorldCommands::GlobalPlayWorldActions.ToSharedRef());

	CreateDefaultCommands();

	CreateGraphCommandList();

	RegisterMenus();

	// Initialize the asset editor and spawn nothing (dummy layout)
	const TSharedRef<FTabManager::FLayout> DummyLayout = FTabManager::NewLayout("NullLayout")->AddArea(FTabManager::NewPrimaryArea());
	const bool bCreateDefaultStandaloneMenu = true;
	const bool bCreateDefaultToolbar = true;
	const FName BlueprintEditorAppName = FName(TEXT("BlueprintEditorApp"));
	InitAssetEditor(Mode, InitToolkitHost, BlueprintEditorAppName, DummyLayout, bCreateDefaultStandaloneMenu, bCreateDefaultToolbar, Objects);
	
	//Init DocumentManager
	DocumentManager = MakeShareable(new FDocumentTracker);
	DocumentManager->Initialize(SharedThis(this));
	DocumentManager->SetTabManager(TabManager.ToSharedRef());

	TArray<UBlueprint*>& CastedArray = reinterpret_cast<TArray<UBlueprint*>&>(InBlueprints);
	
	CommonInitialization(CastedArray, false);
	
	InitalizeExtenders();

	RegenerateMenusAndToolbars();

	AddApplicationMode(FDialogueBuilderCustomEditorModes::DialogueBuilderEditorMode1, MakeShareable(new FDialogueBuilderApplicationMode(SharedThis(this))));
	SetCurrentMode(FDialogueBuilderCustomEditorModes::DialogueBuilderEditorMode1);

	// Post-layout initialization
	PostLayoutBlueprintEditorInitialization();
	
	Compile();

	// Find and set any instances of this blueprint type if any exists and we are not already editing one
	FBlueprintEditorUtils::FindAndSetDebuggableBlueprintInstances();

	// SetUp DialogueBuilderObject
	DialogueBuilderObject = Cast<UDialogueBuilderObject>(GetDialogueBuilderBlueprint()->GeneratedClass->GetDefaultObject());
	
	if (DialogueBuilderObject)
	{
		// Refresh Data from prev Dialogue Object;
		if (DialogueBuilderObject->IsJustDuplicated)
		{			
			DialogueBuilderBlueprint = Cast<UDialogueBuilderBlueprint>(GetBlueprintObj());
			UDialogueBuilderEdGraph* DialogueBuilderEdGraph = NULL;
			DialogueBuilderEdGraph = Cast<UDialogueBuilderEdGraph>(DialogueBuilderBlueprint->DialogueBuilderGraph);
		
			DialogueBuilderEdGraph->GraphGuid= DialogueBuilderEdGraph->GraphGuid.NewGuid();
			// Set Up ObjectReference to Graph
			DialogueBuilderEdGraph->DialogueBuilderObject = DialogueBuilderObject;
			DialogueBuilderObject->IsJustDuplicated = false;
		}

		if (bNewlyCreated)
		{
			UDialogueBuilderEdGraph* DialogueBuilderEdGraph = NULL;
			DialogueBuilderEdGraph = Cast<UDialogueBuilderEdGraph>(DialogueBuilderBlueprint->DialogueBuilderGraph);
			// Set Up ObjectReference to Graph
			DialogueBuilderEdGraph->DialogueBuilderObject = DialogueBuilderObject;
			AddEntryNodeToDialData();
		}

	}
	
	if (UBlueprint* Blueprint = GetBlueprintObj())
	{
		if (Blueprint->GetClass() == UBlueprint::StaticClass() && Blueprint->BlueprintType == BPTYPE_Normal)
		{
			if (!bShouldOpenInDefaultsMode)
			{
				GetToolkitCommands()->ExecuteAction(FFullBlueprintEditorCommands::Get().EditClassDefaults.ToSharedRef());
			}
		}
	}	
}

FName FDialogueBuilderCustomEditor::GetToolkitFName() const
{
	return FName("DialogueBuilderEditor");
}

FText FDialogueBuilderCustomEditor::GetBaseToolkitName() const
{
	return LOCTEXT("DialogueBuilderEditor", "Dialogue Builder Editor");
}

FString FDialogueBuilderCustomEditor::GetWorldCentricTabPrefix() const
{
	return FString();
}

FLinearColor FDialogueBuilderCustomEditor::GetWorldCentricTabColorScale() const
{
	return FLinearColor();
}

void FDialogueBuilderCustomEditor::CreateEventGraph()
{
	ECreatedDocumentType GraphType = FBlueprintEditor::CGT_NewEventGraph;

	if (!NewDocument_IsVisibleForType(GraphType))
	{
		return;
	}

	FText DocumentNameText = LOCTEXT("NewDocEventGraphName", "EventGraph");
	bool bResetMyBlueprintFilter = true;

	FName DocumentName = FName(*DocumentNameText.ToString());

	check(IsEditingSingleBlueprint());

	const FScopedTransaction Transaction(LOCTEXT("AddNewFunction", "Add New Function"));
	GetBlueprintObj()->Modify();

	EventGraphRef = FBlueprintEditorUtils::CreateNewGraph(GetBlueprintObj(), DocumentName, UEdGraph::StaticClass(), GetDefaultSchemaClass());
	FBlueprintEditorUtils::AddUbergraphPage(GetBlueprintObj(), EventGraphRef);

	OpenDocument(EventGraphRef, FDocumentTracker::OpenNewDocument);
	EventGraphRef->bAllowDeletion = false;
	EventGraphRef->bAllowRenaming = false;

	DialogueBuilderBlueprint->EvenGraphRef = EventGraphRef;

	TArray<FName> EventFuncName;
	EventFuncName.Add("ReceiveBeginPlay");
	EventFuncName.Add("ReceiveTick");

	TArray<FString> EventDescription;
	EventDescription.Add("Event when play begins for this component");
	EventDescription.Add("Event called every frame if tick is enabled.");
	
	UClass* const OverrideFuncClass = GetBlueprintObj()->ParentClass;
	bool bIsOverrideFunc = true;
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	TArray<UK2Node_Event*> NewEventNodes;
	TArray<UFunction*> ParentFunctionRef;

	for (int x=0;x< EventFuncName.Num();x++)
	{
		if (FindUField<UFunction>(GetBlueprintObj()->ParentClass, EventFuncName[x]))
		{
			ParentFunctionRef.Add(FindUField<UFunction>(GetBlueprintObj()->ParentClass, EventFuncName[x]));

			FVector2D SpawnPos = EventGraphRef->GetGoodPlaceForNewNode();

			FName EventName = EventFuncName[x];
			NewEventNodes.Add(FEdGraphSchemaAction_K2NewNode::SpawnNode<UK2Node_Event>(
				EventGraphRef,
				SpawnPos,
				EK2NewNodeFlags::SelectNewNode,
				[EventName, OverrideFuncClass](UK2Node_Event* NewInstance)
			{
				NewInstance->EventReference.SetExternalMember(EventName, OverrideFuncClass);
				NewInstance->bOverrideFunction = true;
			}
			));
			NewEventNodes[x]->NodeComment = EventDescription[x];
			NewEventNodes[x]->bCommentBubbleMakeVisible = true;
			
		}
	}
}

void FDialogueBuilderCustomEditor::CreateGraphCommandList()
{
	FDialogueBuilderGraphCommands::Register();

	GraphEditorCommandsRef = MakeShareable(new FUICommandList);
	{
		GraphEditorCommandsRef->MapAction(FDialogueBuilderGraphCommands::Get().Delete,
			FExecuteAction::CreateSP(this, &FDialogueBuilderCustomEditor::DeleteSelectedNodesDB),
			FCanExecuteAction::CreateSP(this, &FDialogueBuilderCustomEditor::CanDeleteNodesDB)
		);
		
		GraphEditorCommandsRef->MapAction(FGraphEditorCommands::Get().DeleteAndReconnectNodes,
			FExecuteAction::CreateSP(this, &FDialogueBuilderCustomEditor::DeleteSelectedNodesDB),
			FCanExecuteAction::CreateSP(this, &FDialogueBuilderCustomEditor::CanDeleteNodesDB)
		);
		
		GraphEditorCommandsRef->MapAction(FDialogueBuilderGraphCommands::Get().Copy,
			FExecuteAction::CreateSP(this, &FDialogueBuilderCustomEditor::CopySelectedNodesDB),
			FCanExecuteAction::CreateSP(this, &FDialogueBuilderCustomEditor::CanCopyNodesDB)
		);

		GraphEditorCommandsRef->MapAction(FDialogueBuilderGraphCommands::Get().Cut,
			FExecuteAction::CreateSP(this, &FDialogueBuilderCustomEditor::CutSelectedNodes),
			FCanExecuteAction::CreateSP(this, &FDialogueBuilderCustomEditor::CanCutNodesDB)
		);

		GraphEditorCommandsRef->MapAction(FDialogueBuilderGraphCommands::Get().Paste,
			FExecuteAction::CreateSP(this, &FDialogueBuilderCustomEditor::PasteNodesDB),
			FCanExecuteAction::CreateSP(this, &FDialogueBuilderCustomEditor::CanPasteNodes)
		);
		GraphEditorCommandsRef->MapAction(FDialogueBuilderGraphCommands::Get().Duplicate,
			FExecuteAction::CreateSP(this, &FDialogueBuilderCustomEditor::DuplicateNodes),
			FCanExecuteAction::CreateSP(this, &FDialogueBuilderCustomEditor::CanDuplicateNodesDB)
		);
	}
    
}

void FDialogueBuilderCustomEditor::InvokeDialogueBuilderGraphTab()
{
	DialogueBuilderBlueprint = Cast<UDialogueBuilderBlueprint>(GetBlueprintObj());
	UDialogueBuilderEdGraph*DialogueBuilderEdGraph = NULL;

	bool bNewGraph = false;
	if (DialogueBuilderBlueprint->DialogueBuilderGraph == NULL)
	{
		CreateEventGraph();
		bNewGraph = true;
		UEdGraph*NewCreatedGraph =
			FBlueprintEditorUtils::CreateNewGraph(
				GetDialogueBuilderBlueprint(),
			FName("MyDialogueBuilderGraph"),
			UDialogueBuilderEdGraph::StaticClass(),
			UDialogueBuilderGraphSchema::StaticClass());

		DialogueBuilderBlueprint->DialogueBuilderGraph = NewCreatedGraph;
		DialogueBuilderGraph = NewCreatedGraph;

		UClass* const OverrideFuncClass = GetBlueprintObj()->ParentClass;
		FName EventName = "Entry";
		FEdGraphSchemaAction_K2NewNode::SpawnNode<UK2Node_FunctionEntry>(NewCreatedGraph,FVector2D(-10000.f, -10000.f),
			EK2NewNodeFlags::None,
			[EventName, OverrideFuncClass](UK2Node_FunctionEntry* NewInstance)
		{

		}
		);
		
		// Create Entry Node With Creating Graph;
		DialogueBuilderGraph->GetSchema()->CreateDefaultNodesForGraph(*DialogueBuilderGraph);
	}
	
	//SetUp DialogueBuilderEdGraph Pointer;
	if (!DialogueBuilderEdGraph)
	{
		DialogueBuilderEdGraph = Cast<UDialogueBuilderEdGraph>(DialogueBuilderBlueprint->DialogueBuilderGraph);
		DialogueBuilderGraph = DialogueBuilderBlueprint->DialogueBuilderGraph;
	}

	OpenDocument(DialogueBuilderBlueprint->EvenGraphRef, FDocumentTracker::OpenNewDocument);
	EventGraphRef = DialogueBuilderBlueprint->EvenGraphRef;

	TSharedRef<FTabPayload_UObject> Payload = FTabPayload_UObject::Make(DialogueBuilderGraph);

	DocumentManager->OpenDocument(Payload, FDocumentTracker::OpenNewDocument);

	GraphEditor->JumpToNode(DialogueBuilderEdGraph->Entry, false);

}
	
void FDialogueBuilderCustomEditor::PostUndo(bool bSuccess)
{
	if (bSuccess)
	{
		const FTransaction* Transaction = GEditor->Trans->GetTransaction(GEditor->Trans->GetQueueLength() - GEditor->Trans->GetUndoCount());
		HandleUndoTransactionDB(Transaction);
	}
}

void FDialogueBuilderCustomEditor::PostRedo(bool bSuccess)
{
	if (bSuccess)
	{
		const FTransaction* Transaction = GEditor->Trans->GetTransaction(GEditor->Trans->GetQueueLength() - GEditor->Trans->GetUndoCount() - 1);
		HandleUndoTransactionDB(Transaction);
	}
}

void FDialogueBuilderCustomEditor::HandleUndoTransactionDB(const class FTransaction* Transaction)
{
	if (DialogueBuilderGraph)
	{
		//Refresh reference of DialogueBuilderObject;
		UDialogueBuilderEdGraph* DialogueGraph = Cast<UDialogueBuilderEdGraph>(DialogueBuilderGraph);
		DialogueGraph->DialogueBuilderObject= Cast<UDialogueBuilderObject>(GetDialogueBuilderBlueprint()->GeneratedClass->GetDefaultObject());
		DialogueBuilderObject= Cast<UDialogueBuilderObject>(GetDialogueBuilderBlueprint()->GeneratedClass->GetDefaultObject());
		
		// Compare Dialogue Data vs Graph data;
		if (DialogueBuilderObject)
		{
			TArray <int32> NodeIDArrayLoc;
			TArray < UStateNodeDB*> StateArrayLoc;
			int32 StateNodeConnectedToEntry = -1;
			TArray < UEdGraph*> FunctionsArrayRef;

			for (int32 x = 0; x < DialogueBuilderGraph->Nodes.Num(); ++x)
			{
				UStateNodeDB* StateNode = Cast<UStateNodeDB>(DialogueBuilderGraph->Nodes[x]);
				if (StateNode)
				{
					// Add ID to existing State Graphs array;
					NodeIDArrayLoc.Add(StateNode->DialogueNodeId);
					StateArrayLoc.Add(StateNode);

					// Set Currently Existed Next Nodes ID;
					TArray <int32> NextNodeConnectedArray;

					// Check if StateNode Info at data;
					int32 IndexAtData = DialogueBuilderObject->GetListIndexFromNodeID(StateNode->DialogueNodeId);
					if (IndexAtData != -1)
					{
						// Check Data vs Current Connected State Nodes;
						if (StateNode->GetOutputPin()->LinkedTo.Num() != StateNode->NextNodesId.Num())
						{
							// Currently Connected Transition Nodes;
							for (int32 y = 0; y < StateNode->GetOutputPin()->LinkedTo.Num(); ++y)
							{
								UTransitionNodeDB* TransitionConnected = Cast<UTransitionNodeDB>(StateNode->GetOutputPin()->LinkedTo[y]->GetOwningNode());
								if (TransitionConnected)
								{
									UStateNodeDB* NextStateNode = Cast<UStateNodeDB>(TransitionConnected->GetNextState());
									if (NextStateNode)
									{
										NextNodeConnectedArray.Add(NextStateNode->DialogueNodeId);
									}
								}
							}

							// Remove Non existing Next Nodes vs Currently Connected - StateTransitionNameToCheck and PinsIDArray;
							if (NextNodeConnectedArray.Num() > 0)
							{
								TArray<int32> DataToRemoveItems;
								for (int32 y = 0; y < StateNode->NextNodesId.Num(); ++y)
								{
									int32 IndexOfNodesConnected = NextNodeConnectedArray.Find(StateNode->NextNodesId[y]);
									if (IndexOfNodesConnected == -1)
									{
										DataToRemoveItems.Add(StateNode->NextNodesId[y]);

										if (StateNode->StateTransitionNameToCheck.IsValidIndex(y))
										{
											StateNode->StateTransitionNameToCheck.RemoveAt(y);
										}
										if (StateNode->PinsIDArray.IsValidIndex(y))
										{
											StateNode->PinsIDArray.RemoveAt(y);
										}
									}
								}
								// Remove Non existing Next Nodes vs Currently Connected - NextNodesId;
								for (int32 i = 0; i < DataToRemoveItems.Num(); ++i)
								{
									StateNode->NextNodesId.Remove(DataToRemoveItems[i]);
								}
								StateNode->UpdateNodeAtDialData();
							}
							else
							{
								StateNode->NextNodesId.Empty();
								StateNode->StateTransitionNameToCheck.Empty();
								StateNode->PinsIDArray.Empty();

								StateNode->UpdateNodeAtDialData();
							}

						}

						// Need to recheck connection inside data as sometimes previous operations skip some data;
						if (StateNode->NextNodesId.Num() != DialogueBuilderObject->DialoguesData[IndexAtData].NextNodesId.Num())
						{
							StateNode->UpdateNodeAtDialData();
						}
					}
					else
					{
						if (StateNode->GetInputPin()->LinkedTo.Num() > 0)
						{
							for (int32 z = 0; z < StateNode->GetInputPin()->LinkedTo.Num(); ++z)
							{
								UEntryNodeDB* EntryNode = Cast<UEntryNodeDB>(StateNode->GetInputPin()->LinkedTo[z]->GetOwningNode());
								if (EntryNode)
								{
									EntryNode->NextNodesId.Empty();
									EntryNode->NextNodesId.Add(StateNode->DialogueNodeId);
									EntryNode->UpdateNodeAtDialData();
								}
							}
						}
						//No StateNode data at object dtb need to add it;
						StateNode->UpdateNodeAtDialData();
					}
				}

				// Restore Condition Function;
				UTransitionNodeDB* TransitionNode = Cast<UTransitionNodeDB>(DialogueBuilderGraph->Nodes[x]);
				if (TransitionNode)
				{
					if (TransitionNode->IsHasFunction)
					{
						FunctionsArrayRef.Add(TransitionNode->FunctionGraph);
						{
							if (GetDialogueBuilderBlueprint()->FunctionGraphs.Find(TransitionNode->FunctionGraph) == -1)
							{
								GetDialogueBuilderBlueprint()->FunctionGraphs.Add(TransitionNode->FunctionGraph);
							}
						}
					}
				}

				// Set Entry Node for Check connection after loop;
				UEntryNodeDB* EntryNodeRef = Cast<UEntryNodeDB>(DialogueBuilderGraph->Nodes[x]);
				if (EntryNodeRef)
				{
					if (EntryNodeRef->GetOutputPin()->LinkedTo.Num()==0)
					{
						EntryNodeRef->NextNodesId.Empty();
						EntryNodeRef->UpdateNodeAtDialData();
					}
				}
			}

			//Delete Non Existing Data from State Nodes in Current Graph;
			TArray<FDialogueDetailsStruct> DataToDelete;

			// Start Searching from 1 as 0 index is Start node
			for (int32 x = 1; x < DialogueBuilderObject->DialoguesData.Num(); x++)
			{
				if (NodeIDArrayLoc.Find(DialogueBuilderObject->DialoguesData[x].NodeId) == -1)
				{
					DataToDelete.Add(DialogueBuilderObject->DialoguesData[x]);
				}
			}
			for (int32 x = 0; x < DataToDelete.Num(); x++)
			{
				DialogueBuilderObject->DialoguesData.Remove(DataToDelete[x]);
			}

			TArray < UEdGraph*> FunctionsToRemove;
			for (int32 x = 0; x < GetDialogueBuilderBlueprint()->FunctionGraphs.Num(); ++x)
			{
				if (FunctionsArrayRef.Find(GetDialogueBuilderBlueprint()->FunctionGraphs[x]) == -1)
				{
					FunctionsToRemove.Add(GetDialogueBuilderBlueprint()->FunctionGraphs[x]);
				}
			}
			for (int32 y = 0; y < FunctionsToRemove.Num(); ++y)
			{
				CloseDocumentTab(FunctionsToRemove[y]);
				GetDialogueBuilderBlueprint()->FunctionGraphs.Remove(FunctionsToRemove[y]);

				FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprintObj());
			}

		}
	}
	
	UBlueprint* BlueprintObj = GetBlueprintObj();
	if (BlueprintObj && Transaction)
	{
		bool bAffectsBlueprint = false;
		const UPackage* BlueprintOutermost = BlueprintObj->GetOutermost();

	
		// Look at the transaction this function is responding to, see if any object in it has an outermost of the Blueprint
		TArray<UObject*> TransactionObjects;
		Transaction->GetTransactionObjects(TransactionObjects);

		for (UObject* Object : TransactionObjects)
		{
			if (TransactionObjectAffectsBlueprint(Object))
			{
				bAffectsBlueprint = true;
				break;
			}
		}

		// Transaction affects the Blueprint this editor handles, so react as necessary
		if (bAffectsBlueprint)
		{
			// Do not clear the selection on undo for the component tree so that the details panel
			// does not get cleared unnecessarily
			if (CurrentUISelection != SelectionState_Components)
			{
				SetUISelectionState(NAME_None);
			}
			RefreshEditors();

			FSlateApplication::Get().DismissAllMenus();
		}
		if (DialogueBuilderGraph)
		{
			DialogueBuilderGraph->NotifyGraphChanged();
		}
	}
	
}

UDialogueBuilderBlueprint* FDialogueBuilderCustomEditor::GetDialogueBuilderBlueprint()
{
	return DialogueBuilderBlueprint.Get();
}

TSharedPtr<FDocumentTracker> FDialogueBuilderCustomEditor::GetDocumentManager()
{
	return DocumentManager;
}

void FDialogueBuilderCustomEditor::AddEntryNodeToDialData()
{
	TArray<UEntryNodeDB*> EntryNodeRef;
	DialogueBuilderGraph->GetNodesOfClass(EntryNodeRef);

	if (EntryNodeRef.Num() > 0)
	{
		UDialogueBuilderEdGraph*DialogueBuilderEdGraph = NULL;
		DialogueBuilderEdGraph = Cast<UDialogueBuilderEdGraph>(DialogueBuilderBlueprint->DialogueBuilderGraph);
		if (DialogueBuilderEdGraph)
		{
			DialogueBuilderEdGraph->AddEntryNodeData();
		}
	}
}

void FDialogueBuilderCustomEditor::OnSelectedNodesChanged(const TSet<class UObject*>& NewSelection)
{
	TArray<UObject*> SelectedObjects;
	for (UObject* Object : NewSelection)
	{
		SelectedObjects.Add(Object);
		
		UStateNodeDB*StateNodeSelected = Cast<UStateNodeDB>(Object);
		if (StateNodeSelected)
		{
			StateNodeSelected->UpdateNodeAtDialData();
		}
	}

	Inspector->ShowDetailsForObjects(SelectedObjects);
}

void FDialogueBuilderCustomEditor::OnNodeDoubleClickedDB(class UEdGraphNode* Node)
{
	if (UTransitionNodeDB *TransitionNode = Cast<UTransitionNodeDB>(Node))
	{
		if (TransitionNode->IsHasFunction)
		{
			if (GetDialogueBuilderBlueprint()->FunctionGraphs.Find(TransitionNode->FunctionGraph) ==-1)
			{
				GetDialogueBuilderBlueprint()->FunctionGraphs.Add(TransitionNode->FunctionGraph);
			}

			if (TransitionNode->FunctionGraph)
			{
				OpenDocument(TransitionNode->FunctionGraph, FDocumentTracker::OpenNewDocument);
			}
		}
		else
		{
			TransitionNode->IsHasFunction = CreateFunctionForTransition(TransitionNode);
		}
	}
	if (UStateNodeDB *StateNodeRef = Cast<UStateNodeDB>(Node))
	{		
		bool CheckIfFunctionExist = true;
		UFunction* NewFunction;

		//Search Node in Event Graph;
		int32 Index = EventGraphRef->Nodes.Find(StateNodeRef->NodeForJump);

		if (Index == -1)
		{
				NewFunction = StateNodeRef->FindFunction("OnDialoguePlay");
				if (NewFunction)
				{
					FString FunctionName;
					FString locFunctionName;
					locFunctionName = "OnDialoguePlay_NodeID" + FString::FromInt(StateNodeRef->DialogueNodeId);
					FunctionName = locFunctionName;

					FVector2D SpawnPos = EventGraphRef->GetGoodPlaceForNewNode();

					UK2Node_CustomEvent* OnDialoguePlayEvent = UK2Node_CustomEvent::CreateFromFunction(SpawnPos, EventGraphRef, FunctionName, NewFunction, false);
					OnDialoguePlayEvent->bCanRenameNode = false;
					OnDialoguePlayEvent->bIsEditable = false;

					StateNodeRef->NodeForJump = OnDialoguePlayEvent;
					StateNodeRef->OnDialoguePlayFuncName = *FunctionName;
					StateNodeRef->UpdateNodeAtDialData();

					if (OnDialoguePlayEvent)
					{
						JumpToNode(StateNodeRef->NodeForJump, false);
					}
				}
		}
		else
		{
			if (StateNodeRef->NodeForJump)
			{
				if (Index!=-1)
				{
					JumpToNode(StateNodeRef->NodeForJump, false);
				}
			}
		}

	}
	
	// Used For Debug;
	if (UEntryNodeDB*TransitionNodeRef = Cast<UEntryNodeDB>(Node))
	{		
		DialogueBuilderObject = Cast<UDialogueBuilderObject>(GetDialogueBuilderBlueprint()->GeneratedClass->GetDefaultObject());
		DialogueBuilderObject->DebugGetAllIndexes();		
	}
	
}

bool FDialogueBuilderCustomEditor::CreateFunctionForTransition(class UTransitionNodeDB *TransitionNode)
{
	UStateNodeDB* PrevStateNode = Cast<UStateNodeDB>(TransitionNode->GetPreviousState());
	UStateNodeDB* NextStateNode = Cast<UStateNodeDB>(TransitionNode->GetNextState());

	FString FunctionName;
	FString locFunctionName;
	if (PrevStateNode&&NextStateNode)
	{
		locFunctionName = "TransitionRule_NodeID " + FString::FromInt(PrevStateNode->DialogueNodeId) + " NodeID " + FString::FromInt(NextStateNode->DialogueNodeId);
		FunctionName=FBlueprintEditorUtils::FindUniqueKismetName(GetDialogueBuilderBlueprint(), locFunctionName).ToString();
	}
	else
	{
		locFunctionName = "NoName";
		FunctionName = FBlueprintEditorUtils::FindUniqueKismetName(GetDialogueBuilderBlueprint(), locFunctionName).ToString();
		
	}

	UEdGraph*NewGraph = FBlueprintEditorUtils::CreateNewGraph(GetDialogueBuilderBlueprint(), *FunctionName, UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
	
	FBlueprintEditorUtils::AddFunctionGraph<UClass>(GetDialogueBuilderBlueprint(), NewGraph, false, NULL);
	NewGraph->bAllowDeletion = false;
	NewGraph->bAllowRenaming = false;

	TransitionNode->SetFunctionName(*FunctionName);

	TArray<UK2Node_FunctionEntry*> EntryNodes;
	NewGraph->GetNodesOfClass(EntryNodes);
	UK2Node_FunctionEntry* EntryNode = EntryNodes[0];
	EntryNode->AddExtraFlags(FUNC_BlueprintPure);

	UK2Node_FunctionResult* FunctionResult = FBlueprintEditorUtils::FindOrCreateFunctionResultNode(EntryNode);
	if (FunctionResult)
	{
		const UEdGraphSchema_K2* Schema = Cast<UEdGraphSchema_K2>(FunctionResult->GetSchema());

		FEdGraphPinType PinType;
		PinType.bIsReference = false;
		PinType.PinCategory = Schema->PC_Boolean;

		FName NewPinName = FunctionResult->CreateUniquePinName(TEXT("Result"));
		UEdGraphPin* NewPin = FunctionResult->CreateUserDefinedPin(NewPinName, PinType, EGPD_Input, false);
		NewPin->DefaultValue = "true";

		FunctionResult->NodePosX = EntryNode->NodePosX + EntryNode->NodeWidth + 512;
		FunctionResult->NodePosY = EntryNode->NodePosY;
	}

	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	FGraphActionListBuilderBase TempListBuilder;
	TempListBuilder.OwnerOfTemporaries = NewObject<UEdGraph>(GetBlueprintObj());
	TempListBuilder.OwnerOfTemporaries->SetFlags(RF_Transient);

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetDialogueBuilderBlueprint());

	OpenDocument(NewGraph, FDocumentTracker::OpenNewDocument);
	TransitionNode->FunctionGraph = NewGraph;

	return true;
}

bool FDialogueBuilderCustomEditor::CanDeleteNodesDB() const
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();

	bool bCanUserDeleteNode = false;

	if (IsEditable(GetFocusedGraph()) && SelectedNodes.Num() > 0)
	{
		for (UObject* NodeObject : SelectedNodes)
		{
			if (Cast<UEntryNodeDB>(NodeObject))
			{
				bCanUserDeleteNode = false;
				break;
			}

			// If any nodes allow deleting, then do not disable the delete option
			UEdGraphNode* Node = Cast<UEdGraphNode>(NodeObject);
			if (Node->CanUserDeleteNode())
			{
				bCanUserDeleteNode = true;
				break;
			}
		}
	}

	return bCanUserDeleteNode;
}

bool FDialogueBuilderCustomEditor::CanCopyNodesDB() const
{
		// If any of the nodes can be duplicated then we should allow copying
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
		if (Cast<UEntryNodeDB>(Node))
		{
			return false;
		}
		if (Cast<UTransitionNodeDB>(Node))
		{
			return false;
		}
		if ((Node != nullptr) && Node->CanDuplicateNode())
		{
			return true;
		}
	}
	return false;
}

void FDialogueBuilderCustomEditor::CopySelectedNodesDB()
{
	// Export the selected nodes and place the text on the clipboard
	FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	FGraphPanelSelectionSet SelectedNodesForUse;
	FString ExportedText;

	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		if (UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter))
		{
			if (!Cast<UTransitionNodeDB>(Node))
			{
				SelectedNodesForUse.Add(*SelectedIter);
			}
		}
	}

	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(SelectedNodesForUse); SelectedIter; ++SelectedIter)
	{
		if (UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter))
		{	
			Node->PrepareForCopying();
		}
	}

	FEdGraphUtilities::ExportNodesToText(SelectedNodesForUse, /*out*/ ExportedText);
	FPlatformApplicationMisc::ClipboardCopy(*ExportedText);
}

bool FDialogueBuilderCustomEditor::CanCutNodesDB() const
{
	return CanCopyNodesDB() && CanDeleteNodesDB();
}

bool FDialogueBuilderCustomEditor::CanDuplicateNodesDB() const
{
	return CanCopyNodesDB() && IsEditable(GetFocusedGraph());
}

void FDialogueBuilderCustomEditor::PasteNodesDB()
{
	// Find the graph editor with focus
	TSharedPtr<SGraphEditor> FocusedGraphEd = FocusedGraphEdPtr.Pin();
	if (!FocusedGraphEd.IsValid())
	{
		return;
	}

	//PasteNodesHere(FocusedGraphEd->GetCurrentGraph(), FocusedGraphEd->GetPasteLocation());
	
	UEdGraph* DestinationGraph = FocusedGraphEd->GetCurrentGraph();

	const FScopedTransaction Transaction(FGenericCommands::Get().Paste->GetDescription());
	DestinationGraph->Modify();

	// Grab the text to paste from the clipboard.
	FString TextToImport;
	FPlatformApplicationMisc::ClipboardPaste(TextToImport);

	// Import the nodes
	TSet<UEdGraphNode*> PastedNodes;
	FEdGraphUtilities::ImportNodesFromText(DestinationGraph, TextToImport, PastedNodes);

	//Average position of nodes so we can move them while still maintaining relative distances to each other
	FVector2D AvgNodePosition(0.0f, 0.0f);

	for (TSet<UEdGraphNode*>::TIterator It(PastedNodes); It; ++It)
	{
		UEdGraphNode* Node = *It;
		AvgNodePosition.X += Node->NodePosX;
		AvgNodePosition.Y += Node->NodePosY;
	}

	float InvNumNodes = 1.0f / float(PastedNodes.Num());
	AvgNodePosition.X *= InvNumNodes;
	AvgNodePosition.Y *= InvNumNodes;

	for (TSet<UEdGraphNode*>::TIterator It(PastedNodes); It; ++It)
	{
		UEdGraphNode* Node = *It;
		FocusedGraphEd->SetNodeSelection(Node, true);

		Node->NodePosX = (Node->NodePosX - AvgNodePosition.X) + FocusedGraphEd->GetPasteLocation().X;
		Node->NodePosY = (Node->NodePosY - AvgNodePosition.Y) + FocusedGraphEd->GetPasteLocation().Y;

		Node->SnapToGrid(SNodePanel::GetSnapGridSize());

		// Give new node a different Guid from the old one
		Node->CreateNewGuid();

		UK2Node* K2Node = Cast<UK2Node>(Node);
		if ((K2Node != nullptr) && K2Node->NodeCausesStructuralBlueprintChange())
		{
			//bNeedToModifyStructurally = true;
		}

		// For pasted Event nodes, we need to see if there is an already existing node in a ghost state that needs to be cleaned up
		if (UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node))
		{
			// Gather all existing event nodes
			TArray<UK2Node_Event*> ExistingEventNodes;
			FBlueprintEditorUtils::GetAllNodesOfClass<UK2Node_Event>(GetBlueprintObj(), ExistingEventNodes);

			for (UK2Node_Event* ExistingEventNode : ExistingEventNodes)
			{
				check(ExistingEventNode);

				const bool bIdenticalNode = (EventNode != ExistingEventNode) && ExistingEventNode->bOverrideFunction && UK2Node_Event::AreEventNodesIdentical(EventNode, ExistingEventNode);

				// Check if the nodes are identical, if they are we need to delete the original because it is disabled. Identical nodes that are in an enabled state will never make it this far and still be enabled.
				if (bIdenticalNode)
				{
					// Should not have made it to being a pasted node if the pre-existing node wasn't disabled or was otherwise explicitly disabled by the user.
					ensure(ExistingEventNode->IsAutomaticallyPlacedGhostNode());

					// Destroy the pre-existing node, we do not need it.
					ExistingEventNode->DestroyNode();
				}
			}
		}
		// Log new node created to analytics
		AnalyticsTrackNodeEvent(GetBlueprintObj(), Node, false);
	}

}

void FDialogueBuilderCustomEditor::DeleteSelectedNodesDB()
{
	TSharedPtr<SGraphEditor> FocusedGraphEd = FocusedGraphEdPtr.Pin();
	if (!FocusedGraphEd.IsValid())
	{
		return;
	}

	const FScopedTransaction Transaction(FGenericCommands::Get().Delete->GetDescription());
	FocusedGraphEd->GetCurrentGraph()->Modify();

	bool bNeedToModifyStructurally = false;

	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();

	SetUISelectionState(NAME_None);

	if (FocusedGraphEd)
	{
		FocusedGraphEd->ClearSelectionSet();
	}

	// this closes all the document that is outered by this node
	// this is used by AnimBP statemachines/states that can create subgraph
	auto CloseAllDocumentsTab = [this](const UEdGraphNode* InNode)
	{
		TArray<UObject*> NodesToClose;
		GetObjectsWithOuter(InNode, NodesToClose);
		for (UObject* Node : NodesToClose)
		{
			UEdGraph* NodeGraph = Cast<UEdGraph>(Node);
			if (NodeGraph)
			{
				CloseDocumentTab(NodeGraph);
			}
		}
	};

	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		if (UEdGraphNode* Node = Cast<UEdGraphNode>(*NodeIt))
		{
			if (Node->CanUserDeleteNode())
			{
				UTransitionNodeDB* TransitionNode = Cast<UTransitionNodeDB>(Node);
				if (TransitionNode)
				{
					if (TransitionNode->IsHasFunction)
					{

						CloseDocumentTab(TransitionNode->FunctionGraph);
						GetDialogueBuilderBlueprint()->FunctionGraphs.Remove(TransitionNode->FunctionGraph);

						bNeedToModifyStructurally = true;
					}
				}
				
				UStateNodeDB* StateNode = Cast<UStateNodeDB>(Node);
				if (StateNode)
				{
					for (int32 LinkIndex = 0; LinkIndex < StateNode->Pins[0]->LinkedTo.Num(); ++LinkIndex)
					{
						UTransitionNodeDB* TransitionNodeLoc = Cast<UTransitionNodeDB>(StateNode->Pins[0]->LinkedTo[LinkIndex]->GetOwningNode());
						if (TransitionNodeLoc)
						{
							if (TransitionNodeLoc->IsHasFunction)
							{

								CloseDocumentTab(TransitionNodeLoc->FunctionGraph);
								GetDialogueBuilderBlueprint()->FunctionGraphs.Remove(TransitionNodeLoc->FunctionGraph);

								bNeedToModifyStructurally = true;
							}
						}			
					}

					for (int32 LinkIndex = 0; LinkIndex < StateNode->Pins[1]->LinkedTo.Num(); ++LinkIndex)
					{
						UTransitionNodeDB* TransitionNodeLoc = Cast<UTransitionNodeDB>(StateNode->Pins[1]->LinkedTo[LinkIndex]->GetOwningNode());
						if (TransitionNodeLoc)
						{
							if (TransitionNodeLoc->IsHasFunction)
							{

								CloseDocumentTab(TransitionNodeLoc->FunctionGraph);
								GetDialogueBuilderBlueprint()->FunctionGraphs.Remove(TransitionNodeLoc->FunctionGraph);

								bNeedToModifyStructurally = true;
							}
						}
					}
				}
				

				UK2Node* K2Node = Cast<UK2Node>(Node);
				if (K2Node != nullptr && K2Node->NodeCausesStructuralBlueprintChange())
				{
					bNeedToModifyStructurally = true;
				}

				if (UK2Node_Composite* SelectedNode = Cast<UK2Node_Composite>(*NodeIt))
				{
					//Close the tab for the composite if it was open
					if (SelectedNode->BoundGraph)
					{
						DocumentManager->CleanInvalidTabs();
					}
				}
				else if (UK2Node_Timeline* TimelineNode = Cast<UK2Node_Timeline>(*NodeIt))
				{
					DocumentManager->CleanInvalidTabs();
				}

				AnalyticsTrackNodeEvent(GetBlueprintObj(), Node, true);

				// If the user is pressing shift then try and reconnect the pins
				if (FSlateApplication::Get().GetModifierKeys().IsShiftDown())
				{
					ReconnectExecPins(K2Node);
				}

				FBlueprintEditorUtils::RemoveNode(GetBlueprintObj(), Node, true);
			}
		}
	}

	if (bNeedToModifyStructurally)
	{
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprintObj());
	}
	else
	{
		FBlueprintEditorUtils::MarkBlueprintAsModified(GetBlueprintObj());
	}

}

#undef LOCTEXT_NAMESPACE
