// Copyright 2022 Greg Shynkar. All Rights Reserved

#pragma once

#include "DialogueBuilderObject.h"
#include "DialogueBuilderCustomEditor.h"
#include "Toolkits/IToolkitHost.h"
#include "AssetTypeActions_Base.h"

class FDialogueBuilderActions : public FAssetTypeActions_Base
{
public:
	FDialogueBuilderActions(EAssetTypeCategories::Type AsstCategory);
	~FDialogueBuilderActions();

	//IAssetTypeActions Implementation;
	virtual FText GetName() const override;
	virtual FColor GetTypeColor() const override;
	virtual UClass* GetSupportedClass() const override;
	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>()) override;
	virtual uint32 GetCategories() override;
	virtual UThumbnailInfo* GetThumbnailInfo(UObject* Asset) const  override;

private:
	EAssetTypeCategories::Type DialogueAssetCategory;
};
