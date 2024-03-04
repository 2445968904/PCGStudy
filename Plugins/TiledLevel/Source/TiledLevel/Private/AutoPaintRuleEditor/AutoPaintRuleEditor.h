// Copyright 2022 PufStudio. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "AutoPaintRule.h"


class TILEDLEVEL_API FAutoPaintRuleEditor : public FAssetEditorToolkit, public FGCObject
{
public:
    FAutoPaintRuleEditor();
    ~FAutoPaintRuleEditor();
    UAutoPaintRule* GetRuleBeingEdited() const { return RuleBeingEdited; }
    void InitRuleEditor(const EToolkitMode::Type Mode, const TSharedPtr<class IToolkitHost>& InitToolkitHost,
                        UAutoPaintRule* InRule);
    void SetContentFromEdMode(class FTiledLevelEdMode* InEdMode);
    class UTiledLevelAsset* GetTempAsset();

protected:
    // IToolkit interface
    virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
    virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
    // End of IToolkit interface
    // FAssetEditorToolkit
    virtual FName GetToolkitFName() const override;
    virtual FText GetBaseToolkitName() const override;
    virtual FText GetToolkitName() const override;
    virtual FText GetToolkitToolTipText() const override;
    virtual FLinearColor GetWorldCentricTabColorScale() const override;
    virtual FString GetWorldCentricTabPrefix() const override;
    virtual void OnToolkitHostingStarted(const TSharedRef<IToolkit>& Toolkit) override;
	virtual void OnToolkitHostingFinished(const TSharedRef<IToolkit>& Toolkit) override;
    // End of FAssetToolkit

    // GC interface
    virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
    virtual FString GetReferencerName() const override { return "FTiledItemSetEditor"; }
    // end of GC interface

private:
	class FTiledLevelEdMode* EdMode = nullptr;
    TSharedPtr<class SBorder> ToolboxPtr;
    UAutoPaintRule* RuleBeingEdited = nullptr;
    TSharedPtr<class SAutoPaintRuleEditorViewport> ViewportPtr;
    TSharedPtr<class SAutoPaintItemPalette> PalettePtr;
	TSharedPtr<class SAutoPaintRuleDetailsEditor> DetailsEditorPtr;
	TSharedPtr<FUICommandList> CommandList;
	
	void MakeTransactionForAsset(FText TransactionMessage);
    void BindCommands();
    void ExtendMenu();
    void OnConfirmCreateOrEditItem(FAutoPaintItem* EditingItem = nullptr);
    void AddNewItem();
	FText CandidateNewItemName;
	FLinearColor CandidateNewItemColor;
	FText CandidateNewGroupName;
	TSharedRef<SWidget> MakeDialogContent_CreateOrEditItem(bool IsCreate = true);
    TSharedRef<SWidget> MakeDialogContent_CreateOrEditGroup(bool IsCreate = true);
    void OnConfirmCreateOrEditGroup(FAutoPaintRuleGroup* EditingGroup = nullptr);
    void AddNewRuleGroup();
    void DuplicateRule() const;
    bool CanEditSomething() const;
    void OnEditSelectedItemOrGroup();
    void OnRemoveSomething();
    bool CanRemoveSomething() const;
    void CopyRule();
	UAutoPaintItemRule* CopiedItemRule = nullptr;
    void PasteRule() const;
    bool CanPasteRule() const;
    void FillToolbar(FToolBarBuilder& ToolbarBuilder);
    void ExtendToolbar();
    void UpdateMatchedRuleHint(UAutoPaintItemRule* TargetItemRule) const;
	
	// auto paint item registry
    TSharedRef<SDockTab> SpawnTab_Toolbox(const FSpawnTabArgs& SpawnTabArgs);
	// rule configuration
    TSharedRef<SDockTab> SpawnTab_Rules(const FSpawnTabArgs& Args);
    TSharedRef<SDockTab> SpawnTab_Preview(const FSpawnTabArgs& Args);
    TSharedRef<SDockTab> SpawnTab_PreviewSettings(const FSpawnTabArgs& Args);

};
