// Copyright 2022 Greg Shynkar. All Rights Reserved

#include "DialogueBuilderActions.h"

FDialogueBuilderActions::FDialogueBuilderActions(EAssetTypeCategories::Type AsstCategory)
{
	DialogueAssetCategory = AsstCategory;
}

FDialogueBuilderActions::~FDialogueBuilderActions()
{

}

FText FDialogueBuilderActions::GetName() const
{
	return NSLOCTEXT("DialogueBuilderActions", "DialogueBuilderActions", "DialogueBuilder");
}

FColor FDialogueBuilderActions::GetTypeColor() const
{
	//return FColor(50, 110, 140);
	 return FColor::Emerald;
}

UClass* FDialogueBuilderActions::GetSupportedClass() const
{
	return UDialogueBuilderBlueprint::StaticClass();
}

void FDialogueBuilderActions::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor /*= TSharedPtr<IToolkitHost>()*/)
{
	EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;
	
	TArray<class UDialogueBuilderBlueprint*> InBlueprints;

	for (auto Object : InObjects)
	{
		if (UDialogueBuilderBlueprint* DialogueNewAsset = Cast<UDialogueBuilderBlueprint>(Object))
		{
			InBlueprints.Add(DialogueNewAsset);
		}
	}

	if (InBlueprints.Num() > 0)
	{
		TSharedRef< FDialogueBuilderCustomEditor > NewEditor(new FDialogueBuilderCustomEditor());
		NewEditor->InitDialogueAssetEditor(Mode, EditWithinLevelEditor, InBlueprints);
	}
}

uint32 FDialogueBuilderActions::GetCategories()
{
	return DialogueAssetCategory;
}

UThumbnailInfo* FDialogueBuilderActions::GetThumbnailInfo(UObject* Asset) const
{
	return NULL;
}
