//$ Copyright 2015-23, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"

class IDetailsView;
class UMGPatternGraph;

DECLARE_DELEGATE_RetVal_OneParam(FReply, FPatternRuleGraphDelegate, TWeakObjectPtr<UMGPatternGraph>);
DECLARE_DELEGATE_OneParam(FPatternRuleGraphChangedDelegate, TWeakObjectPtr<UMGPatternGraph>);
DECLARE_DELEGATE_OneParam(FPatternRuleGraphNodeDelegate, UEdGraphNode*);
DECLARE_DELEGATE_OneParam(FPatternRuleGraphSelectedNodeChangedDelegate, const TSet<class UObject*>&);

class FMGPatternGraphHandler : public TSharedFromThis<FMGPatternGraphHandler> {
public:
    ~FMGPatternGraphHandler();
    void Bind();
    void SetGraphEditor(TSharedPtr<SGraphEditor> InGraphEditor);
    void SetPropertyEditor(TWeakPtr<IDetailsView> InPropertyEditor);

private:

    /** Select every node in the graph */
    void SelectAllNodes();
    /** Whether we can select every node */
    bool CanSelectAllNodes() const;

    /** Deletes all the selected nodes */
    void DeleteSelectedNodes();

    bool CanDeleteNode(class UEdGraphNode* Node);

    /** Delete an array of Material Graph Nodes and their corresponding expressions/comments */
    void DeleteNodes(const TArray<class UEdGraphNode*>& NodesToDelete);

    /** Delete only the currently selected nodes that can be duplicated */
    void DeleteSelectedDuplicatableNodes();

    /** Whether we are able to delete the currently selected nodes */
    bool CanDeleteNodes() const;

    /** Copy the currently selected nodes */
    void CopySelectedNodes();
    /** Whether we are able to copy the currently selected nodes */
    bool CanCopyNodes() const;

    /** Paste the contents of the clipboard */
    void PasteNodes();
    bool CanPasteNodes() const;
    void PasteNodesHere(const FVector2D& Location);

    /** Cut the currently selected nodes */
    void CutSelectedNodes();
    /** Whether we are able to cut the currently selected nodes */
    bool CanCutNodes() const;

    /** Duplicate the currently selected nodes */
    void DuplicateNodes();
    /** Whether we are able to duplicate the currently selected nodes */
    bool CanDuplicateNodes() const;

    /** Called when the selection changes in the GraphEditor */
    void HandleSelectedNodesChanged(const TSet<class UObject*>& NewSelection) const;

    /** Called when a node is double clicked */
    void HandleNodeDoubleClicked(class UEdGraphNode* Node) const;

public:
    SGraphEditor::FGraphEditorEvents GraphEvents;
    TSharedPtr<FUICommandList> GraphEditorCommands;

    FPatternRuleGraphNodeDelegate OnNodeDoubleClicked;
    FPatternRuleGraphSelectedNodeChangedDelegate OnNodeSelectionChanged;

private:
    TSharedPtr<SGraphEditor> GraphEditor;
    TWeakPtr<IDetailsView> PropertyEditor;
};
