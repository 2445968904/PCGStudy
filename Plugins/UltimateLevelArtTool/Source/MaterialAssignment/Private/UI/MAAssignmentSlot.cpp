// Copyright 2023 Leartes Studios. All Rights Reserved.

#include "UI/MAAssignmentSlot.h"
#include "AssetToolsModule.h"
#include "Editor.h"
#include "MADebug.h"
#include "MAMaterialParamObject.h"
#include "MAToolSubsystem.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/SinglePropertyView.h"
#include "Components/PropertyViewBase.h"
#include "Components/TextBlock.h"
#include "Interfaces/MAToolWindowInterface.h"
#include "Materials/MaterialInterface.h"

class FAssetRegistryModule;

void UMAAssignmentSlot::NativePreConstruct()
{
	Super::NativePreConstruct();

#if WITH_EDITOR
	
	ApplyBtn->SetIsEnabled(false);
	MultipleChoiceBox->SetToolTipText(FText::FromName(TEXT("The static meshes with this slot contain different materials.")));
	CounterText->SetToolTipText(FText::FromName(TEXT("This value indicates the number of static meshes with this slot.")));
	SlotRenameText->SetToolTipText(FText::FromName(TEXT("Click to change the name of this slot in all meshes that have this slot.")));
	MultipleChoiceBox->SetVisibility(ESlateVisibility::Hidden);
#endif
	
}

void UMAAssignmentSlot::NativeConstruct()
{
	Super::NativeConstruct();

	if (!ApplyBtn->OnClicked.IsBound())
	{
		ApplyBtn->OnClicked.AddDynamic(this, &UMAAssignmentSlot::ApplyBtnPressed);
	}

	if(!SlotRenameText->OnTextCommitted.IsBound())
	{
		SlotRenameText->OnTextCommitted.AddDynamic(this,&UMAAssignmentSlot::OnSlotNameTextCommitted);
	}
}

void UMAAssignmentSlot::ApplyBtnPressed()
{
	if(const auto ToolSubsystem = GEditor->GetEditorSubsystem<UMAToolSubsystem>())
	{
		const auto ToolWindowRef = ToolSubsystem->GetMAMainScreen();
		if(ToolWindowRef.IsValid())
		{
			if(const auto IToolWindow = Cast<IMAToolWindowInterface>(ToolWindowRef.Get()))
			{
				if(MaterialParamObject && MaterialParamObject->GetMaterialParam())
				{
					DefaultMaterialName = MaterialParamObject->GetMaterialParam()->GetName();
					IToolWindow->ApplySingleMaterialChange(SlotName,MaterialParamObject->GetMaterialParam());
					IToolWindow->MaterialPropertyChanged(false);
				}
			}
		}
	}
	bMaterialParamChangedOnce = false;
	DisableApplyButton();
}

void UMAAssignmentSlot::OnSlotNameTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	if(CommitMethod == ETextCommit::Type::OnEnter)
	{
		if(const auto ToolSubsystem = GEditor->GetEditorSubsystem<UMAToolSubsystem>())
		{
			const auto ToolWindowRef = ToolSubsystem->GetMAMainScreen();
			if(ToolWindowRef.IsValid())
			{
				if(const auto IToolWindow = Cast<IMAToolWindowInterface>(ToolWindowRef.Get()))
				{
					if(!Text.IsEmpty() && !Text.ToString().Equals(SlotName.ToString()))
					{
						if(IToolWindow->RenameMaterialSlot(SlotName,*Text.ToString()))
						{
							MADebug::ShowNotifyInfo(FString(TEXT("Material Slots Renamed Succesfully")));
							SlotName = *Text.ToString();
							SlotRenameText->SetText(FText::FromName(SlotName));
							return;
						}
					}
				}
			}
		}
	}
	SlotRenameText->SetText(FText::FromName(SlotName));
}

void UMAAssignmentSlot::RefreshTheSMNameList()
{
	if(MeshNames.Num() == 0){return;}

	FString LocalString;

	const int32 Num = MeshNames.Num();
	for(int32 Index = 0 ; Index < Num ; Index++)
	{
		LocalString.Append(MeshNames[Index]);
		if(Index == Num - 1){break;}
		LocalString.Append(TEXT("\n"));
	}
	MeshNameList->SetText(FText::FromString(LocalString));
}


void UMAAssignmentSlot::InitializeTheSlot(const FName& InSlotName, UMaterialInterface* InExistingMaterial,const FString& InFirstMeshName)
{
	SlotName = InSlotName;
	MeshNames.Add(InFirstMeshName);
	SlotRenameText->SetText(FText::FromName(InSlotName));
	if(IsValid(InExistingMaterial))
	{
		DefaultMaterialName = InExistingMaterial->GetName();
	}

	if((MaterialParamObject = Cast<UMAMaterialParamObject>(NewObject<UMAMaterialParamObject>(this, TEXT("MaterialParamObject")))))
	{
		MaterialPropertyView->SetPropertyName(FName(TEXT("NewMaterial")));
		MaterialPropertyView->SetObject(MaterialParamObject);
		if(IsValid(InExistingMaterial))
		{
			MaterialParamObject->SetMaterialParam(InExistingMaterial);
		}
		MaterialParamObject->OnMaterialParamChanged.BindUObject(this, &UMAAssignmentSlot::OnMaterialParamChanged);
	}
	RefreshTheSMNameList();
}

void UMAAssignmentSlot::AddNewObjectToCounter(const FString& InMeshName)
{
	MeshNames.Add(InMeshName);
	CounterText->SetText(FText::FromString(FString::FromInt(MeshNames.Num())));
	RefreshTheSMNameList();
}

void UMAAssignmentSlot::CheckForMaterialDifferences(const FString& InMaterialName) const
{
	if(InMaterialName.Equals(DefaultMaterialName)){return;}

	MultipleChoiceBox->SetVisibility(ESlateVisibility::Visible);
	
}

TObjectPtr<UMaterialInterface> UMAAssignmentSlot::GetNewMaterial() const
{
	if(MaterialParamObject){return MaterialParamObject->GetMaterialParam();}
	return nullptr;
}

void UMAAssignmentSlot::DisableApplyButton()
{
	ApplyBtn->SetIsEnabled(false);
}

void UMAAssignmentSlot::AllMaterialsApplied()
{
	DefaultMaterialName = MaterialParamObject->GetMaterialParam()->GetName();
	bMaterialParamChangedOnce = false;
	DisableApplyButton();
}

void UMAAssignmentSlot::OnMaterialParamChanged()
{
	if(!MaterialParamObject){return;}

	//If returned to default one then disable apply button
	if(DefaultMaterialName.Equals(MaterialParamObject->GetMaterialParam()->GetName()))
	{
		DisableApplyButton();
			
		if(bMaterialParamChangedOnce)
		{
			if(const auto ToolSubsystem = GEditor->GetEditorSubsystem<UMAToolSubsystem>())
			{
				const auto ToolWindowRef = ToolSubsystem->GetMAMainScreen();
				if(ToolWindowRef.IsValid())
				{
					if(const auto IToolWindow = Cast<IMAToolWindowInterface>(ToolWindowRef.Get()))
					{
						IToolWindow->MaterialPropertyChanged(false);
						bMaterialParamChangedOnce = false;
					}
				}
			}
		}
		return;
	}
	
	//Notify if did not before
	if(!bMaterialParamChangedOnce)
	{
		if(const auto ToolSubsystem = GEditor->GetEditorSubsystem<UMAToolSubsystem>())
		{
			const auto ToolWindowRef = ToolSubsystem->GetMAMainScreen();
			if(ToolWindowRef.IsValid())
			{
				if(const auto IToolWindow = Cast<IMAToolWindowInterface>(ToolWindowRef.Get()))
				{
					IToolWindow->MaterialPropertyChanged(true);
					bMaterialParamChangedOnce = true;
					ApplyBtn->SetIsEnabled(true);
				}
			}
		}
	}
}

void UMAAssignmentSlot::NativeDestruct()
{
	if(ApplyBtn){ApplyBtn->OnClicked.RemoveAll(this);}
	if(SlotRenameText){SlotRenameText->OnTextCommitted.RemoveAll(this);}
	Super::NativeDestruct();
}
