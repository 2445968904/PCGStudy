// Copyright 2022 Greg Shynkar. All Rights Reserved

#pragma once

#include "Graphs/DialogueBuilderEdGraph.h"
#include "DialogueBuilderCustomEditor.h"
#include "WorkflowOrientedApp/WorkflowTabFactory.h"
#include "WorkflowOrientedApp/WorkflowUObjectDocuments.h"


// Generate the details panel of the class 
struct FDialogueBuilderDetailsTabFactory : public FWorkflowTabFactory
{
public:
	FDialogueBuilderDetailsTabFactory(TSharedPtr<class FDialogueBuilderCustomEditor> InDialogueBuilderEditor);
	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const override;
	virtual FText GetTabToolTipText(const FWorkflowTabSpawnInfo& Info) const override;

protected:
	TWeakPtr<FDialogueBuilderCustomEditor> DialogueBuilderEditor;
};


// This class is a Tab factory class, used to generate GraphTab
struct FDialogueBuilderGraphTabFactory : public FDocumentTabFactoryForObjects<UDialogueBuilderEdGraph>
{
public:
	FDialogueBuilderGraphTabFactory(TSharedPtr<FDialogueBuilderCustomEditor> InDialogueBuilderEditor);
	virtual void OnTabActivated(TSharedPtr<SDockTab> Tab) const override;
	virtual void OnTabRefreshed(TSharedPtr<SDockTab> Tab) const override;

protected:
	virtual TAttribute<FText> ConstructTabNameForObject(UDialogueBuilderEdGraph* DocumentID) const;
	virtual TSharedRef<SWidget> CreateTabBodyForObject(const FWorkflowTabSpawnInfo& Info, UDialogueBuilderEdGraph* DocumentID) const;	
	virtual const FSlateBrush* GetTabIconForObject(const FWorkflowTabSpawnInfo& Info, UDialogueBuilderEdGraph* DocumentID) const;

private:
	FDialogueBuilderCustomEditor* CurDialogueEditorRef;
	TWeakPtr<FDialogueBuilderCustomEditor> DialogueBuilderEditor;
};
