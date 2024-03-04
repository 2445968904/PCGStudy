// Copyright 2022 PufStudio. All Rights Reserved.

#include "AutoPaintRuleFactory.h"

#include "AutoPaintRule.h"
#include "TiledItemSet.h"
#include "Dialogs/Dialogs.h"
#include "WorkflowOrientedApp/SContentReference.h"

UAutoPaintRuleFactory::UAutoPaintRuleFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UAutoPaintRule::StaticClass();
}

bool UAutoPaintRuleFactory::ConfigureProperties()
{
	if (InitialItemSetPtr.IsValid()) return true;
	bSuccessInitItemSet = false;
	SGenericDialogWidget::FArguments Args;
	Args.OnOkPressed_Lambda([=]()
	{
		if (InitialItemSetPtr.IsValid())
			bSuccessInitItemSet = true;
	});
	SGenericDialogWidget::OpenDialog(FText::FromString("Create new auto paint rule"), CreateDialogContent(), Args, true);
	return bSuccessInitItemSet;
}

UObject* UAutoPaintRuleFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags,
                                                 UObject* Context, FFeedbackContext* Warn)
{
	UAutoPaintRule* NewRule = NewObject<UAutoPaintRule>(InParent, InClass, InName, Flags | RF_Transactional);
	NewRule->SourceItemSet = InitialItemSetPtr.Get();
	return NewRule;
}

void UAutoPaintRuleFactory::SetSourceItemSet(const TWeakObjectPtr<UTiledItemSet>& InItemSetPtr)
{
	InitialItemSetPtr = InItemSetPtr;
}

TSharedRef<SWidget> UAutoPaintRuleFactory::CreateDialogContent()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		[
			SNew(STextBlock)
			.Text(FText::FromString("Choose source item set:"))
		]
		+ SVerticalBox::Slot()
		.Padding(24, 12, 0, 0)
		[
			SNew(SContentReference)
			.WidthOverride(140.0f)
			.AssetReference_UObject(this, &UAutoPaintRuleFactory::GetItemSet)
			.OnSetReference_UObject(this, &UAutoPaintRuleFactory::OnSetItemSet)
			.AllowedClass(UTiledItemSet::StaticClass())
			.ShowFindInBrowserButton(false)
			.AllowSelectingNewAsset(true)
		]
	;
}

UObject* UAutoPaintRuleFactory::GetItemSet() const
{
	return InitialItemSetPtr.Get();
}

void UAutoPaintRuleFactory::OnSetItemSet(UObject* NewAsset)
{
	InitialItemSetPtr = Cast<UTiledItemSet>(NewAsset);
}
