// Copyright 2022 PufStudio. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"

class UAutoPaintRule;

class TILEDLEVEL_API FAutoPaintRuleTypeActions : public FAssetTypeActions_Base
{
public:
	FAutoPaintRuleTypeActions(EAssetTypeCategories::Type InAssetCategory);

	// IAssetTypeActions interface
	virtual FText GetName() const override;
	virtual UClass* GetSupportedClass() const override;
	virtual FColor GetTypeColor() const override;
	virtual uint32 GetCategories() override;
	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>()) override;
	virtual bool HasActions(const TArray<UObject*>& InObjects) const override { return true; }
	virtual void GetActions(const TArray<UObject*>& InObjects, FToolMenuSection& Section) override;
	// End of IAssetTypeActions interface

private:
	EAssetTypeCategories::Type MyAssetCategory;
	
	void ExecuteCreateTiledLevel(TWeakObjectPtr<UAutoPaintRule> AutoPaintRulePtr);
};
