// Copyright 2020-2024 Maksym Maisak. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Delegates/Delegate.h"
#include "GraphEditor.h"
#include "HAL/Platform.h"
#include "IAssetTypeActions.h"
#include "Input/Reply.h"
#include "Misc/EngineVersionComparison.h"
#include "Types/SlateEnums.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

class FUICommandList;
class ITableRow;
class SBorder;
class STableViewBase;
class SWidget;
class UEdGraph;
class UEdGraphPin;
struct FDiffSingleResult;
template <typename ItemType> class SListView;

class SHTNDiff : public SCompoundWidget
{
public:

	// Delegate for default Diff tool
	DECLARE_DELEGATE_TwoParams(FOpenInDefaultDiffTool, class UHTN*, class UHTN*);

	SLATE_BEGIN_ARGS(SHTNDiff) {}
		SLATE_ARGUMENT(class UHTN*, HTNOld)
		SLATE_ARGUMENT(class UHTN*, HTNNew)
		SLATE_ARGUMENT(struct FRevisionInfo, OldRevision)
		SLATE_ARGUMENT(struct FRevisionInfo, NewRevision)
		SLATE_ARGUMENT(bool, ShowAssetNames)
		SLATE_EVENT(FOpenInDefaultDiffTool, OpenInDefaultDiffTool)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:

	// A panel used to display the HTN
	struct FHTNDiffPanel
	{
		/** Constructor */
		FHTNDiffPanel();

		/** 
		 * Generate a panel that displays the HTN and reflects the items in the DiffResults
		 */
		void GeneratePanel(
#if UE_VERSION_OLDER_THAN(5, 1, 0)
			UEdGraph* const GraphToDiff);
#else
			TSharedPtr<TArray<FDiffSingleResult>> DiffResults);
#endif

		/**
		 * Returns a text describing the revision this diff panel is showing
		 */
		FText GetRevisionDescription() const;

		/** 
		 * Called when user hits keyboard shortcut to copy nodes
		 */
		void CopySelectedNodes();

		/**
		 * Called When graph node gains focus
		 */
		void OnSelectionChanged( const FGraphPanelSelectionSet& Selection );

		/**
		 * Delegate to say if a node property should be editable
		 */
		bool IsPropertyEditable();

		/**
		 * Gets whatever nodes are selected in the Graph Editor
		 * @return Gets the selected nodes in the graph
		 */
		FGraphPanelSelectionSet GetSelectedNodes() const;

		/**
		 * Can user copy any of the selected nodes?
		 * @return True if can copy
		 */
		bool CanCopyNodes() const;

		// The HTN that owns the graph we are showing
		class UHTN* HTN;

		// Revision information for this HTN
		FRevisionInfo RevisionInfo;

		// The border around the graph editor, used to change the content when new graphs are set
		TSharedPtr<SBorder> GraphEditorBorder;

		// The graph editor which does the work of displaying the graph
		TWeakPtr<class SGraphEditor> GraphEditor;

		// If we should show a name identifying which asset this panel is displaying
		bool bShowAssetName;
	
		// Command list for this diff panel
		TSharedPtr<FUICommandList> GraphEditorCommands;

		// Property View 
		TSharedPtr<class IDetailsView> DetailsView;
	};

	using FSharedDiffOnGraph = TSharedPtr<struct FHTNDiffResultItem>;

	void OnOpenInDefaultDiffTool();

	TSharedRef<SWidget> GenerateDiffListWidget();
	TSharedRef<SWidget> GenerateDiffPanels();

	void BuildDiffSourceArray();

	void NextDiff();
	void PrevDiff();

	/**
	 * Get current Index into the diff array
	 * @return The Index
	 */
	int32 GetCurrentDiffIndex();

	/** 
	 * Called when difference selection is changed
	 * @param Item The item that was selected
	 * @param SelectionType The way it was selected
	 */
	void OnSelectionChanged(FSharedDiffOnGraph Item, ESelectInfo::Type SelectionType);

	/**
	 * Called when a new row is being generated
	 * @param Item The item being generated
	 * @param OwnerTable The table its going into
	 * @return The Row that was inserted
	 */
	TSharedRef<ITableRow> OnGenerateRow(FSharedDiffOnGraph Item, const TSharedRef<STableViewBase>& OwnerTable);

	/** 
	 * Get the Slate graph editor of the supplied Graph
	 * @param Graph The graph we want the Editor from
	 * @return The Graph Editor
	 */
	SGraphEditor* GetGraphEditorForGraph(UEdGraph* Graph) const;

	/** Get the image to show for the toggle lock option*/
	FSlateIcon GetLockViewImage() const;

	/** User toggles the option to lock the views between the two graphs */
	void OnToggleLockView();

	void ResetGraphEditors();

	// Delegate to call when user wishes to view the defaults
	FOpenInDefaultDiffTool	OpenInDefaultDiffTool;

	// The 2 panels we will be comparing
	FHTNDiffPanel PanelOld;
	FHTNDiffPanel PanelNew;

	// If the two views should be locked
	bool bLockViews = true;

	// Source for list view 
	TArray<FSharedDiffOnGraph> DiffListSource;

	// result from FGraphDiffControl::DiffGraphs
	TSharedPtr<TArray<FDiffSingleResult>> FoundDiffs;

	// Key commands processed by this widget
	TSharedPtr<FUICommandList> KeyCommands;

	// ListView of differences
	TSharedPtr<SListView<FSharedDiffOnGraph>> DiffList;

	// The last pin the user clicked on
	UEdGraphPin* LastPinTarget = nullptr;

	// The last other pin the user clicked on
	UEdGraphPin* LastOtherPinTarget = nullptr;
};
