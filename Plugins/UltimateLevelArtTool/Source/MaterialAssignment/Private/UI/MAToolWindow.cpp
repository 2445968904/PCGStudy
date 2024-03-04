// Copyright 2023 Leartes Studios. All Rights Reserved.

#include "UI/MAToolWindow.h"
#include "AssetToolsModule.h"
#include "ContentBrowserModule.h"
#include "Editor.h"
#include "EditorAssetLibrary.h"
#include "EditorUtilityLibrary.h"
#include "MAAssignmentSlot.h"
#include "MADebug.h"
#include "MAToolAssetData.h"
#include "MAToolSubsystem.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"


void UMAToolWindow::NativePreConstruct()
{
	Super::NativePreConstruct();

	ApplyAllBox->SetVisibility(ESlateVisibility::Collapsed);
	NoMeshText->SetVisibility(ESlateVisibility::Hidden);
}

void UMAToolWindow::NativeConstruct()
{
	Super::NativeConstruct();

	if (!ApplyAllBtn->OnClicked.IsBound())
	{
		ApplyAllBtn->OnClicked.AddDynamic(this, &UMAToolWindow::ApplyAllBtnPressed);
	}

	if(const auto ToolSettingsSubsystem = GEditor->GetEditorSubsystem<UMAToolSubsystem>())
	{
		ToolSettingsSubsystem->SetMAToolMainScreen(this);
	}

	FContentBrowserModule& ContentBrowserModule = FModuleManager::GetModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
	ContentBrowserModule.GetOnAssetSelectionChanged().AddUObject(this, &UMAToolWindow::HandleOnAssetSelectionChanged);
	ContentBrowserModule.GetOnAssetPathChanged().AddUObject(this, &UMAToolWindow::HandleOnAssetPathChanged);

	const FAssetRegistryModule& AssetRegistryModule = FModuleManager::Get().GetModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	AssetRegistry.OnAssetRemoved().AddUObject(this, &UMAToolWindow::HandleOnAssetRemoved);
	AssetRegistry.OnAssetRenamed().AddUObject(this, &UMAToolWindow::HandleOnAssetRenamed);
	
	UpdateSelectedAssets();
	
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Emplace("/Game");
	Filter.ClassPaths.Emplace(UObjectRedirector::StaticClass()->GetClassPathName());
}


void UMAToolWindow::HandleOnAssetSelectionChanged(const TArray<FAssetData>& NewSelectedAssets, bool bIsPrimaryBrowser)
{
	UpdateSelectedAssets();
}

void UMAToolWindow::UpdateSelectedAssets()
{
	UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
	if(!EditorWorld){return;}
	
	const auto LocalSelectedAssets = UEditorUtilityLibrary::GetSelectedAssets();
	
	if(LocalSelectedAssets.IsEmpty())
	{
		NoMeshText->SetVisibility(ESlateVisibility::Visible);
		
		ResetAllVariables();
		return;
	}
	
	const TArray<UStaticMesh*> NewAssetList = FilterStaticMeshAssets(LocalSelectedAssets);
	
	if(NewAssetList.IsEmpty())
	{
		NoMeshText->SetVisibility(ESlateVisibility::Visible);
		
		ResetAllVariables();
		return;
	}

	NoMeshText->SetVisibility(ESlateVisibility::Hidden);

	//For per material In the static mesh
	for(auto CurrentAsset : NewAssetList)
	{
		if(SelectedStaticMeshes.Contains(CurrentAsset)){continue;}

		SelectedStaticMeshes.Add(CurrentAsset);

		for(auto CurrentMaterial : CurrentAsset->GetStaticMaterials())
		{
			//If contains then find and add existing slot
			if(SlotNames.Contains(CurrentMaterial.MaterialSlotName))
			{
				for(const auto CurrentSlot : MaterialSlotWidgets)
				{
					if(CurrentSlot->GetSlotName().IsEqual(CurrentMaterial.MaterialSlotName))
					{
						CurrentSlot->AddNewObjectToCounter(CurrentAsset->GetName());
						CurrentSlot->CheckForMaterialDifferences(CurrentMaterial.MaterialInterface->GetName());
					}
				}
			}
			else // Create New Slot & Add In It
			{
				const TSoftClassPtr<UUserWidget> WidgetClassPtr(MAToolAssetData::MaterialAssignmentSlot);
				if(const auto ClassRef = WidgetClassPtr.LoadSynchronous())
				{
					if (UMAAssignmentSlot* CreatedAssignmentSlot = Cast<UMAAssignmentSlot>(CreateWidget(EditorWorld, ClassRef)))
					{
						CreatedAssignmentSlot->InitializeTheSlot(CurrentMaterial.MaterialSlotName,CurrentMaterial.MaterialInterface,CurrentAsset->GetName());
						MaterialSlotWidgets.Add(CreatedAssignmentSlot);
						SlotBox->AddChild(CreatedAssignmentSlot);
						SlotNames.Add(CurrentMaterial.MaterialSlotName);
					}
				}
			}
		}
	}

	//Remove static meshes that are not in the new list
	bool bListDecreased = false;
	const int32 Num = SelectedStaticMeshes.Num();

	if(Num > 0)
	{
		for(int32 Index = 0 ; Index < Num ; Index++)
		{
			if (!NewAssetList.Contains(SelectedStaticMeshes[Num - Index - 1]))
			{
				SelectedStaticMeshes.RemoveAt(Num - Index - 1);
				bListDecreased = true;
			}
		}
	}
	if(bListDecreased)
	{
		ResetAllVariables();
		UpdateSelectedAssets();
	}
	RecalculateApplyAllBtnStatus();
}

void UMAToolWindow::ResetAllVariables()
{
	SelectedStaticMeshes.Empty();
	SlotBox->ClearChildren();
	MaterialSlotWidgets.Empty();
	SlotNames.Empty();
	ApplyAllBox->SetVisibility(ESlateVisibility::Collapsed);
}

TArray<UStaticMesh*> UMAToolWindow::FilterStaticMeshAssets(const TArray<UObject*>& Assets) const
{
	TArray<UStaticMesh*> FilteredAssets;

	for (UObject* Asset : Assets)
	{
		if (UStaticMesh* StaticMesh = Cast<UStaticMesh>(Asset))
		{
			FilteredAssets.Add(StaticMesh);
		}
	}
	return FilteredAssets;
}
void UMAToolWindow::ApplyAllBtnPressed()
{
	if(MaterialSlotWidgets.IsEmpty()){return;}
	
	for(const auto MaterialSlot : MaterialSlotWidgets)
	{
		if(MaterialSlot->IsMaterialParamChanged())
		{
			MaterialSlot->AllMaterialsApplied();
		
			auto SlotName = MaterialSlot->GetSlotName();
			auto MaterialToApply = MaterialSlot->GetNewMaterial();
		
			if(!MaterialToApply){continue;}

			for(const auto CurrentMesh : SelectedStaticMeshes)
			{
				if(!CurrentMesh){continue;}
			
				bool bIsMeshChanged = false;
			
				auto MeshStaticMaterials = CurrentMesh->GetStaticMaterials();
			
				for(int32 Index = 0 ; Index < MeshStaticMaterials.Num() ; Index++)
				{
					if(MeshStaticMaterials[Index].MaterialSlotName.IsEqual(SlotName))
					{
						CurrentMesh->SetMaterial(Index,MaterialToApply);
						bIsMeshChanged = true;
						break;
					}
				}
				if(bIsMeshChanged)
				{
					UEditorAssetLibrary::SaveLoadedAsset(CurrentMesh,false);
				}
			}
		}
	}

	
	ApplyAllBox->SetVisibility(ESlateVisibility::Collapsed);
	AlteredSlotCounter = false;

	FixUpDirectories();
}
void UMAToolWindow::RecalculateApplyAllBtnStatus()
{
	if(MaterialSlotWidgets.IsEmpty())
	{
		ApplyAllBox->SetVisibility(ESlateVisibility::Collapsed);
		AlteredSlotCounter = 0;
		return;
	}

	for(const auto MaterialSlotWidget : MaterialSlotWidgets)
	{
		if(MaterialSlotWidget->IsMaterialParamChanged())
		{
			++AlteredSlotCounter;
		}
	}
	AlteredSlotCounter > 1 ? ApplyAllBox->SetVisibility(ESlateVisibility::Visible) : ApplyAllBox->SetVisibility(ESlateVisibility::Collapsed);
}

void UMAToolWindow::MaterialPropertyChanged(bool bIncreaseCounter)
{
	if(bIncreaseCounter)
	{
		if(++AlteredSlotCounter > 1)
		{
			ApplyAllBox->SetVisibility(ESlateVisibility::Visible);
		}
	}
	else
	{
		if(--AlteredSlotCounter < 2)
		{
			ApplyAllBox->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void  UMAToolWindow::ApplySingleMaterialChange(const FName& SlotName, UMaterialInterface* InMaterialInterface)
{
	if(SelectedStaticMeshes.IsEmpty()){return;}

	for(const auto CurrentSM : SelectedStaticMeshes)
	{
		auto Materials = CurrentSM->GetStaticMaterials();
		const int32 SlotNum = Materials.Num();
		for(int32 MatIndex = 0 ; MatIndex < SlotNum ; MatIndex++)
		{
			if(Materials[MatIndex].MaterialSlotName.IsEqual(SlotName))
			{
				CurrentSM->SetMaterial(MatIndex,InMaterialInterface);
				UEditorAssetLibrary::SaveLoadedAsset(CurrentSM,false);
				break;
			}
		}
	}
	FixUpDirectories();
}

bool UMAToolWindow::RenameMaterialSlot(const FName& CurrentSlotName, const FName& NewSlotName)
{
	if(SelectedStaticMeshes.IsEmpty()){return false;}

	
	const auto SlotIndex = SlotNames.Find(CurrentSlotName);
	if(SlotIndex >= 0)
	{
		SlotNames[SlotIndex] = NewSlotName;
	}
	else
	{
		return false;
	}

	bool bSlotChanged = false;
	for(const auto CurrentSM : SelectedStaticMeshes)
	{
		auto& Materials = CurrentSM->GetStaticMaterials();
		const int32 SlotNum = Materials.Num();
		for(int32 MatIndex = 0 ; MatIndex < SlotNum ; MatIndex++)
		{
			if(Materials[MatIndex].MaterialSlotName.IsEqual(CurrentSlotName))
			{
				Materials[MatIndex].MaterialSlotName = NewSlotName;
				UEditorAssetLibrary::SaveLoadedAsset(CurrentSM,false);
				bSlotChanged = true;
				break;
			}
		}
	}
	
	if(bSlotChanged)
	{
		FixUpDirectories();
		return true;
	}
	return false;
}

void UMAToolWindow::HandleOnAssetPathChanged(const FString& NewPath)
{
	FixUpTheList();
}

void UMAToolWindow::HandleOnAssetRemoved(const FAssetData& InAsset)
{
	FixUpTheList();
}

void UMAToolWindow::HandleOnAssetRenamed(const FAssetData& InAsset, const FString& InOldObjectPath)
{
	FixUpTheList();
}

void UMAToolWindow::FixUpTheList()
{
	if(const auto ToolSettingsSubsystem = GEditor->GetEditorSubsystem<UMAToolSubsystem>())
	{
		ToolSettingsSubsystem->SetMAToolMainScreen(this);
	}
	if(!SelectedStaticMeshes.IsEmpty())
	{
		ResetAllVariables();
		UpdateSelectedAssets();
	}
}

void UMAToolWindow::NativeDestruct()
{
	if(ApplyAllBtn){ApplyAllBtn->OnClicked.RemoveAll(this);}

	if (FModuleManager::Get().IsModuleLoaded(TEXT("AssetRegistry"))) {
		const FAssetRegistryModule& assetRegistryModule = FModuleManager::Get().GetModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		if (assetRegistryModule.GetPtr()) {
			IAssetRegistry& AssetRegistry = assetRegistryModule.Get();
			AssetRegistry.OnAssetRemoved().RemoveAll(this);
			AssetRegistry.OnAssetRenamed().RemoveAll(this);
		}
	}
	FContentBrowserModule& ContentBrowserModule = FModuleManager::GetModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
	ContentBrowserModule.GetOnAssetSelectionChanged().RemoveAll(this);
	ContentBrowserModule.GetOnAssetPathChanged().RemoveAll(this);
	
	Super::NativeDestruct();
}

void UMAToolWindow::FixUpDirectories() const
{
	const FAssetRegistryModule& AssetRegistryModule = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	const FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));

	TArray<FAssetData> FoundAssets;
	AssetRegistryModule.Get().GetAssets(Filter,FoundAssets);

	TArray<UObjectRedirector*> DirectoriesToFixArray;
	for(const FAssetData& AssetData: FoundAssets)
	{
		if(UObjectRedirector* DirectoryToFix = Cast<UObjectRedirector>(AssetData.GetAsset()))
		{
			DirectoriesToFixArray.Add(DirectoryToFix);
		}
	}
	AssetToolsModule.Get().FixupReferencers(DirectoriesToFixArray);
}

