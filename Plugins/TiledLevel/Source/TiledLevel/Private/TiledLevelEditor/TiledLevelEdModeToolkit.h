// Copyright 2022 PufStudio. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "Misc/TextFilter.h"
#include "Toolkits/BaseToolkit.h"
#include "TiledPaletteItem.h"

class UAutoPaintRule;
class UTiledItemSet;

class FTiledLevelEdModeToolkit : public FModeToolkit
{
public:

	FTiledLevelEdModeToolkit(class FTiledLevelEdMode* InOwningMode);

	/** FModeToolkit interface */
	virtual void Init(const TSharedPtr<IToolkitHost>& InitToolkitHost) override;

	/** IToolkit interface */
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual class FEdMode* GetEditorMode() const override;
	virtual TSharedPtr<class SWidget> GetInlineContent() const override;
	
	// virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager) override;
	// virtual void UnregisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager) override;
	
	UObject* GetCurrentItemSet() const;
	UObject* GetCurrentRule() const;
	void LoadItemSet(UTiledItemSet* NewSet);
	void LoadAutoPaintRule(UAutoPaintRule* NewRule);
	
	// palette widget
	TSharedPtr<class SWidgetSwitcher> PaletteSwitcher;
	TSharedPtr<class STiledPalette> ItemPalette;
	TSharedPtr<class SAutoPaintItemPalette> AutoItemPalette;
	TSharedPtr<SWidgetSwitcher> AssetChooserSwitcher;
	
private:
	FTiledLevelEdMode* EdMode = nullptr;
	TWeakObjectPtr<UTiledItemSet> CurrentItemSetPtr;
	TWeakObjectPtr<UAutoPaintRule> CurrentAutoPaintRulePtr;

	
	EVisibility GetErrorMessageVisibility() const;

	// toolbar widget
	TSharedRef<SWidget> BuildToolbar();
	TSharedRef<SWidget> BuildAssetChooser();
    TOptional<int> GetFloorsNum() const;
    TSharedRef<SWidget> MakeSelectionOptions();
    TOptional<int> GetSelectionSizeValue() const;
    void OnSelectionSizeChanged(int NewSelectionSize) const;
    void OnSelectionSizeCommitted(int NewSelectionSize, ETextCommit::Type CommitType) const;
    void SetSelectionForAll() const;
    bool GetSelectionForAll() const;
	TSharedRef<SWidget> MakeEraserOptions();
	void SetEraserSnapTarget(EPlacedType SnapTarget);
	bool CanSetEraserSnapTarget(EPlacedType SnapTarget);
	bool GetEraserSnapTargetChecked(EPlacedType SnapTarget);
	EVisibility GetEraserExtentInputVisibility(EAxis::Type Axis) const;
	TOptional<int> GetEraserExtentValue(EAxis::Type Axis) const;
	void OnEraserExtentChanged(int InValue, EAxis::Type Axis);
	void OnEraserExtentCommitted(int InValue, ETextCommit::Type CommitType, EAxis::Type Axis);
	bool GetOrphanDataExistence() const;
	void ClearOrphanData();
	
	TSharedRef<SWidget> MakeFillOptions();
	// either for tile or edge, so I just use boolean to separate
	void SetFillMode(bool IsTile) const;
	bool GetFillMode(bool IsTile) const;
	void SetBoundaryTarget(bool IsTile) const;
	bool GetBoundaryTarget(bool IsTile) const;
	bool CanSetBoundaryTarget() const;
	void SetNeedGround();
	bool GetNeedGround() const;

	TSharedRef<SWidget> MakeStepOptions();
	TOptional<int> GetStepSizeValue() const;
	void OnStepSizeChanged(int InValue);
	void OnStepSizeCommitted(int InValue, ETextCommit::Type CommitType);
	TOptional<int> GetMaxStartPoint() const;
	TOptional<int> GetStartPoint(bool IsX = true) const;
	void OnStartPointChanged(int InValue, bool IsX = true);
	void OnStartPointCommitted(int InValue, ETextCommit::Type CommitType, bool IsX = true);

	TSharedRef<SWidget> MakeAutoPaintOptions();
	float GetAutoPaintHintOpacity() const;
	void SetAutoPaintHintOpacity(float NewOpacity);
	void ConvertAutoPaintToNormal() const;
	
	// Asset reference widget
	void OnChangeItemSet(UObject* NewAsset);
	bool OnShouldFilterAsset(const struct FAssetData& AssetData);
	void OnChangeRule(UObject* Object);
	bool OnShouldFilterRuleAsset(const FAssetData& AssetData) const;
	
};
