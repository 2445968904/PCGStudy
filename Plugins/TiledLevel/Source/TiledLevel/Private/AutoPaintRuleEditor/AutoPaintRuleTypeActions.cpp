// Copyright 2022 PufStudio. All Rights Reserved.

#include "AutoPaintRuleTypeActions.h"

#include "AutoPaintRule.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "TiledItemSet.h"
#include "AutoPaintRuleEditor.h"
#include "AutoPaintRuleFactory.h"
#include "TiledLevelAsset.h"
#include "TiledLevelStyle.h"
#include "ToolMenuSection.h"
#include "TiledLevelEditor/TiledLevelFactory.h"

#define LOCTEXT_NAMESPACE "TiledItemSetTypeActions"

FAutoPaintRuleTypeActions::FAutoPaintRuleTypeActions(EAssetTypeCategories::Type InAssetCategory)
	:MyAssetCategory(InAssetCategory)
{
}

FText FAutoPaintRuleTypeActions::GetName() const
{
	return LOCTEXT("FAutoPaintRuleAssetTypeActionsName", "Auto Paint Rule");
}

UClass* FAutoPaintRuleTypeActions::GetSupportedClass() const
{
	return UAutoPaintRule::StaticClass();
}

FColor FAutoPaintRuleTypeActions::GetTypeColor() const
{
	return FColorList::Firebrick;
}

uint32 FAutoPaintRuleTypeActions::GetCategories()
{
	return MyAssetCategory;
}

void FAutoPaintRuleTypeActions::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;
	for (UObject* Obj : InObjects)
 	{
 		if (UAutoPaintRule* Rule = Cast<UAutoPaintRule>(Obj))
 		{
 			TSharedRef<FAutoPaintRuleEditor> NewRuleEditor(new FAutoPaintRuleEditor());
 			NewRuleEditor->InitRuleEditor(Mode, EditWithinLevelEditor, Rule);
 		}		
 	}	
}

void FAutoPaintRuleTypeActions::GetActions(const TArray<UObject*>& InObjects, FToolMenuSection& Section)
{
	TArray<TWeakObjectPtr<UAutoPaintRule>> AutoPaintRules = GetTypedWeakObjectPtrs<UAutoPaintRule>(InObjects);

	if (AutoPaintRules.Num() == 1)
	{
		Section.AddMenuEntry(
			"TiledItemSet_CreateTiledLevel",
			LOCTEXT("AutoPaintRule_CreateTiledLevel", "Create Tiled Level"),
			LOCTEXT("AutoPaintRule_CreateTiledLevelTooltip", "Creates a tiled level using the selected auto paint rule"),
			FSlateIcon(FTiledLevelStyle::GetStyleSetName(), "ClassIcon.TiledLevelAsset"),
			FUIAction(FExecuteAction::CreateSP(this, &FAutoPaintRuleTypeActions::ExecuteCreateTiledLevel, AutoPaintRules[0]))
			);
	}
}

void FAutoPaintRuleTypeActions::ExecuteCreateTiledLevel(TWeakObjectPtr<UAutoPaintRule> AutoPaintRulePtr)
{
	FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	const FString SuffixToIgnore(TEXT("Set"));
	const FString TileMapSuffix(TEXT("Level"));

	if (UAutoPaintRule* Rule = AutoPaintRulePtr.Get())
	{
		// Figure out what to call the new tiled level
		FString EffectiveRuleName = Rule->GetName();
		EffectiveRuleName.RemoveFromEnd(SuffixToIgnore);

		const FString RulePathName = Rule->GetOutermost()->GetPathName();
		const FString LongPackagePath = FPackageName::GetLongPackagePath(RulePathName);

		const FString NewTiledLevelDefaultPath = LongPackagePath / EffectiveRuleName;

		// Make sure the name is unique
		FString AssetName;
		FString PackageName;
		AssetToolsModule.Get().CreateUniqueAssetName(NewTiledLevelDefaultPath, TileMapSuffix, /*out*/ PackageName, /*out*/ AssetName);
		const FString PackagePath = FPackageName::GetLongPackagePath(PackageName);

		// Create the new tiled level
		UTiledLevelFactory* TiledLevelFactory = NewObject<UTiledLevelFactory>();
		TiledLevelFactory->InitialItemSet = Rule->SourceItemSet;
		TiledLevelFactory->InitialRule = Rule;
		ContentBrowserModule.Get().CreateNewAsset(AssetName, PackagePath, UTiledLevelAsset::StaticClass(), TiledLevelFactory);
	}
}

#undef LOCTEXT_NAMESPACE