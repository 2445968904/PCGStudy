// Copyright 2022 Greg Shynkar. All Rights Reserved

#pragma once

#include "HAL/PlatformApplicationMisc.h"
#include "CoreMinimal.h"
#include "Containers/ArrayView.h"
#include "DialogueBuilderBlueprint.h"
#include "DialogueBuilderObject.h"
#include "K2Node_Composite.h"
#include "K2Node_Timeline.h"
#include "SBlueprintEditorToolbar.h"
#include "Kismet2/DebuggerCommands.h"
#include "SNodePanel.h"
#include "SKismetInspector.h"
#include "K2Node_CustomEvent.h"
#include "EdGraphSchema_K2_Actions.h"
#include "DialogueBuilderGraphCommands.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_FunctionResult.h"
#include "EdGraphUtilities.h"
#include "DialogueBuilderApplicationMode.h"
#include "WorkflowOrientedApp/WorkflowCentricApplication.h"
#include "AIGraphEditor.h"
#include "WorkflowOrientedApp/WorkflowTabManager.h"
#include "GraphEditor.h"
#include "Editor/Transactor.h"
#include "Graphs/DialogueBuilderGraphSchema.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "BlueprintEditor.h"

class IDetailsView;
class SDockableTab;

#define LOCTEXT_NAMESPACE "DialogueBuilderCustomEditor"

// Create Struct to set it up to DialogueBuilderApplicationMode;
struct FDialogueBuilderCustomEditorModes
{
	// Mode constants
	static const FName DialogueBuilderEditorMode1;
	static const FName DialogueBuilderEditorMode2;

	static FText GetLocalizedMode(const FName InMode)
	{
		static TMap< FName, FText > LocModes;

		if (LocModes.Num() == 0)
		{
			LocModes.Add(DialogueBuilderEditorMode1, NSLOCTEXT("DialogueBuilderEditorMode1", "DialogueBuilderEditororMode1", "Dialogue Builder Editor Mode1"));
			LocModes.Add(DialogueBuilderEditorMode2, NSLOCTEXT("DialogueBuilderEditororMode2", "DialogueBuilderEditororMode2", "Dialogue Builder Editor Mode2"));
		}

		check(InMode != NAME_None);
		const FText* OutDesc = LocModes.Find(InMode);
		check(OutDesc);
		return *OutDesc;
	}
private:
	FDialogueBuilderCustomEditorModes() {}
};
#undef LOCTEXT_NAMESPACE


class FDialogueBuilderCustomEditor : public FBlueprintEditor
{
public:

	/**Constructor */
	FDialogueBuilderCustomEditor();

	/** Destructor */
	virtual ~FDialogueBuilderCustomEditor();

	// Initialize the editor function
	void InitDialogueAssetEditor(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, TArray<class UDialogueBuilderBlueprint*>& InBlueprints, bool bShouldOpenInDefaultsMode =false);

	/** Begin IToolkit interface */
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;
	/** End IToolkit interface */

	// Create EventGraph and start functions;
	void CreateEventGraph();

	// Create and assign commands for graph tab;
	void CreateGraphCommandList();

	// Open Tabs while open dialogue asset;
	virtual void InvokeDialogueBuilderGraphTab(); 

	//~ Begin FEditorUndoClient Interface
	virtual void	PostUndo(bool bSuccess) override;
	virtual void	PostRedo(bool bSuccess) override;
	// End of FEditorUndoClient

	// ExecuteAction and CanExecuteAction for GraphEditorCommands 
	bool CanDeleteNodesDB() const;
	bool CanCopyNodesDB() const;
	void CopySelectedNodesDB();
	bool CanCutNodesDB() const;
	bool CanDuplicateNodesDB() const;
	void PasteNodesDB();
	void DeleteSelectedNodesDB();

	/** Returns a pointer to the Blueprint object we are currently editing, as long as we are editing exactly one */
	UDialogueBuilderBlueprint* GetDialogueBuilderBlueprint();

	// Copy of HandleUndoTransaction from FBlueprintEditor with check of transition functions from TransitionNodeDB 
	void HandleUndoTransactionDB(const class FTransaction* Transaction);

	// Create transition function with double click on TransitionNodeDB;
	bool CreateFunctionForTransition(class UTransitionNodeDB *TransitionNode);

	// Editor Events for Graph Tab // CreateSP at DialogueBuilderEdTabFactories
	void OnSelectedNodesChanged(const TSet<class UObject*>& NewSelection);
	void OnNodeDoubleClickedDB(class UEdGraphNode* Node);

	// With creating Graph Tab we create Entry Node that can`t be deleted or changed
	void AddEntryNodeToDialData();

	TSharedPtr<FDocumentTracker> GetDocumentManager();

	TSharedPtr<SGraphEditor> GraphEditor;
	TSharedPtr<FUICommandList> GraphEditorCommandsRef;
	TSharedPtr<IDetailsView> DetailsViewTab;

private:

	TWeakObjectPtr<UDialogueBuilderBlueprint> DialogueBuilderBlueprint;
	UDialogueBuilderObject* DialogueBuilderObject;
	TSharedPtr<FDocumentTracker> DocumentManager;
	UEdGraph* DialogueBuilderGraph;
	UEdGraph* EventGraphRef;

};

