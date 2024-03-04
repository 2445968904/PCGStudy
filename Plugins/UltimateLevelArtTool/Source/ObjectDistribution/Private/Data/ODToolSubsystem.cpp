// Copyright 2023 Leartes Studios. All Rights Reserved.


#include "ODToolSubsystem.h"
#include "Editor.h"
#include "EditorAssetLibrary.h"
#include "Engine/DataTable.h"
#include "EditorUtilityLibrary.h"
#include "EngineUtils.h"
#include "ODPresetData.h"
#include "Editor/UnrealEdEngine.h"
#include "ODBoundsVisualizer.h"
#include "ODBoundsVisualizerComponent.h"
#include "ODToolAssetData.h"
#include "ODToolSettings.h"
#include "ODToolWindow.h"
#include "UnrealEdGlobals.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Subsystems/EditorActorSubsystem.h"

void UODToolSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

#if WITH_EDITOR
	if (!GEditor) {return;}
	
	LoadDataAssets();
	
	//Register OD Bounds Visualizer
	if(GUnrealEd)
	{
		const TSharedPtr<FODBoundsVisualizer> BoundsVisualizer = MakeShareable(new FODBoundsVisualizer());
		GUnrealEd->RegisterComponentVisualizer(UODBoundsVisualizerComponent::StaticClass()->GetFName(), BoundsVisualizer);
		BoundsVisualizer->OnRegister();
	}
	
	ODToolSettings = NewObject<UODToolSettings>(this, FName(TEXT("ODToolSettings")));

	if(ODToolSettings)
	{	
		ODToolSettings->LoadConfig();
	}
	
	UpdateMixerCheckListAndResetAllCheckStatus();

#endif // WITH_EDITOR
}

void UODToolSubsystem::LoadDataAssets()
{
#if WITH_EDITOR

	if(UObject* ODPresetDataObject = ODToolAssetData::PresetDataPath.TryLoad())
	{
		PresetData = Cast<UDataTable>(ODPresetDataObject);
		
		if(PresetData)
		{
			RefreshThePresetDataTableForInvalidData();
		}
	}

	if(!PresetData)
	{
		UE_LOG(LogTemp,Error,TEXT("PresetData could not be loaded properly!"));
	}
#endif // WITH_EDITOR
}


void UODToolSubsystem::Deinitialize()
{
	CreatedDistObjects.Empty();
	ObjectDataMap.Empty();
	ObjectDistributionData.Empty();
	
	if(GUnrealEd)
	{
		GUnrealEd->UnregisterComponentVisualizer(UODBoundsVisualizerComponent::StaticClass()->GetFName());
	}
	
	Super::Deinitialize();
}

void UODToolSubsystem::ToolWindowClosed()
{
	const int32 Num = CreatedDistObjects.Num();
	if(Num == 0){return;}
	
	for(int32 Index = 0 ; Index < Num ; ++Index)
	{
		if(IsValid(CreatedDistObjects[Num - Index - 1]))
		{
			auto LocalRef = CreatedDistObjects[Num - Index - 1];
			CreatedDistObjects.Remove(LocalRef);
			LocalRef->Destroy();
		}
	}
	CreatedDistObjects.Empty();
}

#pragma region Presets

bool UODToolSubsystem::IsPresetAvailable(const FString& InPResetName) const
{
	if(!PresetData){return false;}	
	
	TArray<FString> PresetNames;
	TArray<FName> RowNames = PresetData->GetRowNames();
	if(RowNames.Num() == 0){return false;}
	
	for (const auto RowName : RowNames)
	{
		if(const auto FoundRow = PresetData->FindRow<FODPresetData>(RowName, "", true))
		{
			if(FoundRow->PresetName.ToString().ToLower().Equals(InPResetName.ToLower()))
			{
				return true;
			}
		}
	}
	return false;
}

FODPresetData* UODToolSubsystem::GetPresetData(const FString& InPResetName) const
{
	if(!PresetData){return nullptr;}	
	
	TArray<FString> PresetNames;
	TArray<FName> RowNames = PresetData->GetRowNames();
	if(RowNames.Num() == 0){return nullptr;}
	
	for (const auto RowName : RowNames)
	{
		if(const auto FoundRow = PresetData->FindRow<FODPresetData>(RowName, "", true))
		{
			if(FoundRow->PresetName.ToString().ToLower().Equals(InPResetName.ToLower()))
			{
				return FoundRow;
			}
		}
	}
	return nullptr;
}

void UODToolSubsystem::AddNewPreset(const FString& InPResetName) const
{
	FODPresetData NewPresetData;
	NewPresetData.PresetName = FName(*InPResetName);
	const FName RowName = DataTableUtils::MakeValidName(*InPResetName);
	PresetData->AddRow(RowName,NewPresetData);
	SavePresetDataToDisk();
}

bool UODToolSubsystem::CreateNewPresetFromSelectedAssets(const FString& InPResetName) const
{
	const auto LocalSelectedAssets = UEditorUtilityLibrary::GetSelectedAssets();
	
	FODPresetData NewPresetData;
	NewPresetData.PresetName = FName(*InPResetName);
	
	for(const auto LocalObject : LocalSelectedAssets)
	{
		if(const auto LocalSM = Cast<UStaticMesh>(LocalObject))
		{
			FDistObjectData NewDistObjectData;
			NewDistObjectData.StaticMesh = LocalSM;
			NewPresetData.DistObjectData.Add(NewDistObjectData);
		}
	}

	if(NewPresetData.DistObjectData.IsEmpty())
	{
		return false;
	}
	
	const FName RowName = DataTableUtils::MakeValidName(*InPResetName);
	PresetData->AddRow(RowName,NewPresetData);
	SavePresetDataToDisk();
	return true;
}

void UODToolSubsystem::SetActivePreset(const FString& InPResetName)
{
	if(InPResetName.ToLower().Equals("no preset"))
	{
		LastSelectedPreset = NAME_None;
	}
	else
	{
		LastSelectedPreset = *InPResetName;
	}
}

void UODToolSubsystem::SetLastPresetAsActivePreset()
{
	if(!PresetData->GetRowNames().IsEmpty())
	{
		if(const auto FoundRow = PresetData->FindRow<FODPresetData>(PresetData->GetRowNames().Last(), "", true))
		{
			LastSelectedPreset = FoundRow->PresetName;
		}
	}
}

void UODToolSubsystem::RenameCurrentPreset(const FString& InNewName)
{
	TArray<FName> RowNames = PresetData->GetRowNames();
	for (const auto RowName : RowNames)
	{
		if(const auto FoundRow = PresetData->FindRow<FODPresetData>(RowName, "", true))
		{
			if(FoundRow->PresetName.IsEqual(LastSelectedPreset))
			{
				FoundRow->PresetName = FName(*InNewName);
				LastSelectedPreset = FoundRow->PresetName;
				break;
			}
		}
	}
	SavePresetDataToDisk();
}

bool UODToolSubsystem::SaveCurrentPreset() const
{
	TArray<FName> RowNames = PresetData->GetRowNames();
	for (const auto RowName : RowNames)
	{
		if(const auto FoundRow = PresetData->FindRow<FODPresetData>(RowName, "", true))
		{
			if(FoundRow->PresetName.IsEqual(LastSelectedPreset))
			{
				FoundRow->DistObjectData = ObjectDistributionData;
				SavePresetDataToDisk();
				return true;
			}
		}
	}
	return false;
}

void UODToolSubsystem::RemoveCurrentPreset()
{
	PresetData->RemoveRow(LastSelectedPreset);
	LastSelectedPreset = NAME_None;
	SavePresetDataToDisk();
}

void UODToolSubsystem::LoadActivePreset()
{
	if(GetLastSelectedPreset().IsNone())
	{
		ObjectDistributionData.Empty();
		OnPresetLoaded.ExecuteIfBound();
		return;
	}
	
	
	TArray<FName> ModAssetRows = PresetData->GetRowNames();
	for(const FName ModAssetRow : ModAssetRows)
	{
		if(const auto FoundRow = PresetData->FindRow<FODPresetData>(ModAssetRow, "", true))
		{
			if(FoundRow->PresetName.IsEqual(LastSelectedPreset))
			{
				ObjectDistributionData = FoundRow->DistObjectData;
				OnPresetLoaded.ExecuteIfBound();
				break;
			}
		}
	}
}


TArray<FString> UODToolSubsystem::GetPresetNames() const
{
	if(!PresetData){return TArray<FString>();}

	TArray<FString> PresetNames;
	PresetNames.Add(TEXT("No Preset"));
	TArray<FName> RowNames = PresetData->GetRowNames();
	if(RowNames.Num() == 0){return PresetNames;}
	
	for (auto RowName : RowNames)
	{
		if(const auto FoundRow = PresetData->FindRow<FODPresetData>(FName(RowName), "", true))
		{
			if(!FoundRow->PresetName.IsNone())
			{
				PresetNames.Add(FoundRow->PresetName.ToString());
			}
		}
	}
	return PresetNames;
}

void UODToolSubsystem::SavePresetDataToDisk() const
{
	if(PresetData)
	{
		if(PresetData->MarkPackageDirty())
		{
			PresetData->PostEditChange();
			UEditorAssetLibrary::SaveLoadedAsset(PresetData);
		}
	}
}

void UODToolSubsystem::RefreshThePresetDataTableForInvalidData() const
{
	if(!PresetData){return;}

	TArray<FName> ModAssetRows = PresetData->GetRowNames();

	if(ModAssetRows.Num() == 0){return;}

	bool bChangesMade = false;
	
	for(const FName ModAssetRow : ModAssetRows)
	{
		if(const auto FoundRow = PresetData->FindRow<FODPresetData>(ModAssetRow, "", true))
		{
			const int32 Length = FoundRow->DistObjectData.Num();
			if(Length == 0)
			{
				continue;
			}
			
			for(int32 Index = 0; Index < Length; Index++)
			{
				if(FoundRow->DistObjectData[Length - Index - 1].StaticMesh.IsNull())
				{
					FoundRow->DistObjectData[Length - Index - 1].StaticMesh == nullptr;
					bChangesMade = true;
				}
			}
		}
	}
	if(bChangesMade)
	{
		SavePresetDataToDisk();
	}
}



void UODToolSubsystem::DestroyKillZActors(const TArray<FName>& InActorNames)
{
	const int32 Num = CreatedDistObjects.Num();
	if(Num == 0){return;}
	
	for(int32 Index = 0 ; Index < Num ; Index++)
	{
		if(InActorNames.Contains(*CreatedDistObjects[Num - Index - 1]->GetName()))
		{
			auto ActorToDelete = CreatedDistObjects[Num - Index - 1];
			CreatedDistObjects.Remove(ActorToDelete);
			ActorToDelete->Destroy();
		}
	}
}

void UODToolSubsystem::CreateInstanceFromDistribution(const EODMeshConversionType& InTargetType)
{
	//Get Editor World
	if(!GEditor){return;}
	const auto EditorWorld = GEditor->GetEditorWorldContext().World();
	if(!EditorWorld){return;}
	
	//Define Mesh Collection Data
	struct FMeshData {
		TObjectPtr<UStaticMesh> StaticMesh;
		TArray<TObjectPtr<UMaterialInterface>> Materials;
		TArray<FTransform> UniqueTransforms; 
	};

	//Create Initial Variables
	TArray<FMeshData> MeshData;
	TObjectPtr<UStaticMeshComponent> CurrentSMComp;
	

	//Create Lambda For Adding Data To MeshData
  	auto AddStaticMeshToMeshData = [&MeshData, &CurrentSMComp]() -> void
	{
  		FMeshData NewMeshData;

  		//First data so add new one and return
  		if(MeshData.IsEmpty())
  		{
  			NewMeshData.StaticMesh = CurrentSMComp->GetStaticMesh();
  			NewMeshData.Materials = CurrentSMComp->GetMaterials();
  			NewMeshData.UniqueTransforms.Add(CurrentSMComp->GetComponentTransform());
  			MeshData.Add(NewMeshData);
  			return;
  		}

  		//Check all the data to see if there is exactly the same
		for (int Index = 0; Index < MeshData.Num(); ++Index)
		{
			//If they are have save mesh
			if(MeshData[Index].StaticMesh == CurrentSMComp->GetStaticMesh())
			{
				//If They Are Same Materials
				int32 MaterialIndex = 0;
				bool bAreMaterialsSame = true;
				for(auto CurrentMat : MeshData[Index].Materials)
				{
					if(CurrentMat != CurrentSMComp->GetMaterial(MaterialIndex))
					{
						bAreMaterialsSame = false;
						break;
					}
					++MaterialIndex;
				}
				
				//Both are exactly same
				if(bAreMaterialsSame)
				{
					//They are same add a new transform
					MeshData[Index].UniqueTransforms.Add(CurrentSMComp->GetComponentTransform());
					return;
				}
			}
		}
  		
  		//No matching existing data so add new
  		NewMeshData.StaticMesh = CurrentSMComp->GetStaticMesh();
  		NewMeshData.Materials = CurrentSMComp->GetMaterials();
  		NewMeshData.UniqueTransforms.Add(CurrentSMComp->GetComponentTransform());
  		MeshData.Add(NewMeshData);
	};

	FBox BoundingBox = GetAllActorsTransportPointWithExtents(CreatedDistObjects);

	for(const auto CurrentActor : CreatedDistObjects)
	{
		const auto CurrentSMActor = Cast<AStaticMeshActor>(CurrentActor);
		if(!CurrentSMActor || !IsValid(CurrentSMActor)){return;}

		CurrentSMComp = CurrentSMActor->GetStaticMeshComponent();

		AddStaticMeshToMeshData();
	}

	if(MeshData.IsEmpty()){return;}
	
	//Spawn And Get An Actor
	FTransform NewActorTransform;
	FVector AverageLocation = BoundingBox.GetCenter();
	AverageLocation.Z = BoundingBox.GetCenter().Z - BoundingBox.GetExtent().Z;
	NewActorTransform.SetTranslation(AverageLocation);

	const FText TransactionDescription = FText::FromString(TEXT("Instance Creation."));
	GEngine->BeginTransaction(TEXT("ObjectDistributionActorCreation"), TransactionDescription, nullptr);
	
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.bNoFail = true;
	const auto SpawnedActor = EditorWorld->SpawnActor<AActor>(AActor::StaticClass(),SpawnParams);
	if(!SpawnedActor){return;}

	USceneComponent* CreatedRootComponent = NewObject<USceneComponent>(SpawnedActor,FName(TEXT("RootComponent")));
	if(!CreatedRootComponent){SpawnedActor->Destroy(); return;}
	CreatedRootComponent->bAutoRegister = true;
	SpawnedActor->SetRootComponent(CreatedRootComponent);
	SpawnedActor->AddInstanceComponent(CreatedRootComponent);
	SpawnedActor->AddOwnedComponent(CreatedRootComponent);
	SpawnedActor->SetActorTransform(NewActorTransform);
	CreatedRootComponent->SetFlags(RF_Transactional);

	const FString NewActorLabel = FString::Printf(TEXT("%s_Instance"),*LastSelectedPreset.ToString());
	SpawnedActor->SetActorLabel(NewActorLabel);
	SpawnedActor->GetRootComponent()->SetMobility(EComponentMobility::Static);

	//Create Components
	if(InTargetType == EODMeshConversionType::Sm)
	{
		for(const auto CurrentMeshData : MeshData)
		{
			for(auto CurrentTransform : CurrentMeshData.UniqueTransforms)
			{
				const FName CompName = MakeUniqueObjectName(EditorWorld, UStaticMeshComponent::StaticClass(), FName(*CurrentMeshData.StaticMesh->GetName()));
				UStaticMeshComponent* CreatedSmComp = NewObject<UStaticMeshComponent>(SpawnedActor,CompName);
				if(!CreatedSmComp){continue;}
				
				CreatedSmComp->SetupAttachment(SpawnedActor->GetRootComponent());
				CreatedSmComp->AttachToComponent(SpawnedActor->GetRootComponent(),FAttachmentTransformRules::KeepWorldTransform);
				SpawnedActor->AddInstanceComponent(CreatedSmComp);
				SpawnedActor->AddOwnedComponent(CreatedSmComp);
				CreatedSmComp->SetStaticMesh(CurrentMeshData.StaticMesh);

				for(int32 MatIndex = 0; MatIndex < CurrentMeshData.Materials.Num() ;  ++MatIndex)
				{
					CreatedSmComp->SetMaterial(MatIndex,CurrentMeshData.Materials[MatIndex]);
				}
				CreatedSmComp->SetWorldTransform(CurrentTransform);
				CreatedSmComp->SetMobility(EComponentMobility::Static);
				CreatedSmComp->SetFlags(RF_Transactional);
			}
		}
	}
	else
	{
		for(const auto CurrentMeshData : MeshData)
		{
			TObjectPtr<UClass> ISmClass = InTargetType == EODMeshConversionType::Ism ? UInstancedStaticMeshComponent::StaticClass() : UHierarchicalInstancedStaticMeshComponent::StaticClass();

			const FName CompName = MakeUniqueObjectName(EditorWorld, UInstancedStaticMeshComponent::StaticClass(), FName(*CurrentMeshData.StaticMesh->GetName()));
			TObjectPtr<UInstancedStaticMeshComponent> CreatedIsmComp = NewObject<UInstancedStaticMeshComponent>(SpawnedActor,ISmClass,CompName);
			if(!CreatedIsmComp){continue;}

			CreatedIsmComp->SetupAttachment(SpawnedActor->GetRootComponent());
			CreatedIsmComp->AttachToComponent(SpawnedActor->GetRootComponent(),FAttachmentTransformRules::KeepWorldTransform);
			SpawnedActor->AddInstanceComponent(CreatedIsmComp);
			SpawnedActor->AddOwnedComponent(CreatedIsmComp);

			CreatedIsmComp->SetStaticMesh(CurrentMeshData.StaticMesh);
			CreatedIsmComp->SetWorldTransform(SpawnedActor->GetActorTransform());
			CreatedIsmComp->SetMobility(EComponentMobility::Static);
			CreatedIsmComp->SetFlags(RF_Transactional);

			for(int32 MatIndex = 0; MatIndex < CurrentMeshData.Materials.Num() ;  ++MatIndex)
			{
				CreatedIsmComp->SetMaterial(MatIndex,CurrentMeshData.Materials[MatIndex]);
			}
			
			for(auto CurrentTransform : CurrentMeshData.UniqueTransforms)
			{
				CreatedIsmComp->AddInstance(CurrentTransform,true);
			}
		}
	}
	
	SpawnedActor->RegisterAllComponents();
	GEngine->EndTransaction();
	
	//Destroy Placeholders
	int32 ObjectNum = CreatedDistObjects.Num();
	for(int32 Index = 0 ; Index < ObjectNum ; ++Index)
	{
		if(AActor* CurrentActor = CreatedDistObjects[ObjectNum - Index - 1])
		{
			CreatedDistObjects.RemoveAt(ObjectNum - Index - 1);
			CurrentActor->Destroy();
		}
	}

	//Selected Newly Created Collection Actor
	if(UEditorActorSubsystem* EditorActorSubsystem = GEditor->GetEditorSubsystem<UEditorActorSubsystem>())
	{
		EditorActorSubsystem->SelectNothing();
		EditorActorSubsystem->SetActorSelectionState(SpawnedActor,true);
	}
}

FBox UODToolSubsystem::GetAllActorsTransportPointWithExtents(const TArray<AStaticMeshActor*>& InActors)
{
	if(InActors.IsEmpty()){return FBox();}

	FVector PositiveExtent;
	FVector NegativeExtent;
	
	int32 Index = 0;
	for(const auto Actor : InActors)
	{
		if(!IsValid(Actor)){continue;}
		
		FVector ActorOrigin;
		FVector ActorExtent;
		Actor->GetActorBounds(false,ActorOrigin,ActorExtent,false);
		const FVector ActorPositiveExtent = ActorOrigin + ActorExtent;
		const FVector ActorNegativeExtent = ActorOrigin - ActorExtent;
		
		if(Index == 0)
		{
			PositiveExtent = ActorPositiveExtent;
			NegativeExtent = ActorNegativeExtent;
			++Index;
			continue;	
		}
		
		if(ActorPositiveExtent.X > PositiveExtent.X)
		{
			PositiveExtent.X = ActorPositiveExtent.X;
		}
		if(ActorPositiveExtent.Y > PositiveExtent.Y)
		{
			PositiveExtent.Y = ActorPositiveExtent.Y;
		}
		if(ActorPositiveExtent.Z > PositiveExtent.Z)
		{
			PositiveExtent.Z = ActorPositiveExtent.Z;
		}
		if(ActorNegativeExtent.X < NegativeExtent.X)
		{
			NegativeExtent.X = ActorNegativeExtent.X;
		}
		if(ActorNegativeExtent.Y < NegativeExtent.Y)
		{
			NegativeExtent.Y = ActorNegativeExtent.Y;
		}
		if(ActorNegativeExtent.Z < NegativeExtent.Z)
		{
			NegativeExtent.Z = ActorNegativeExtent.Z;
		}
		++Index;
	}
	return FBox(NegativeExtent,PositiveExtent);
	
}
#pragma endregion Presets


#pragma region PresetMixer

void UODToolSubsystem::UpdateMixerCheckListAndResetAllCheckStatus()
{
	PresetMixerMapData.Empty();
	
	const auto PresetNames = GetPresetNames();
	if(PresetNames.IsEmpty()){return;}
	
	for(auto& PresetName : PresetNames)
	{
		if(!PresetName.ToLower().Equals(TEXT("no preset")))
		{
			PresetMixerMapData.Add(FPresetMixerMapData(PresetName,false));
		}
	}
}

void UODToolSubsystem::ToggleMixerMode()
{
	bInMixerMode  = !bInMixerMode;

	if(bInMixerMode)
	{
		UpdateMixerCheckListAndResetAllCheckStatus();
	}
	else
	{
		PresetMixerMapData.Empty();
		PresetMixerData.Empty();
	}
	
	OnMixerModeChanged.ExecuteIfBound(bInMixerMode);
}

void UODToolSubsystem::MixerPresetChecked(const FPresetMixerMapData& InMixerMapData)
{
	if(InMixerMapData.CheckState)
	{
		if(const auto FoundPresetData = GetPresetData(InMixerMapData.PresetName))
		{
			FPresetMixData NewPresetMixData;
			NewPresetMixData.PresetName = FName(*InMixerMapData.PresetName);
			NewPresetMixData.DistObjectData = FoundPresetData->DistObjectData;
			PresetMixerData.Add(NewPresetMixData);
			OnAMixerPresetCheckStatusChanged.ExecuteIfBound(true,*InMixerMapData.PresetName);
		}
	}
	else
	{
		const int32 MixDatNum = PresetMixerData.Num();
		for(auto Index = 0; Index < MixDatNum ; ++Index)
		{
			if(PresetMixerData[MixDatNum - Index - 1].PresetName.IsEqual(*InMixerMapData.PresetName,ENameCase::IgnoreCase))
			{
				PresetMixerData.RemoveAt(MixDatNum - Index - 1);
				OnAMixerPresetCheckStatusChanged.ExecuteIfBound(false,*InMixerMapData.PresetName);
				return;
			}
		}
	}
}

void UODToolSubsystem::RegeneratePresetMixerDataFromScratch()
{
	PresetMixerData.Empty();

	TArray<FString> CheckedPresetNames;
	for(auto LocalName :PresetMixerMapData)
	{
		if(LocalName.CheckState)
		{
			CheckedPresetNames.Add(LocalName.PresetName);
		}
	}
	if(CheckedPresetNames.IsEmpty()){return;}
	
	TArray<FString> PresetNames;
	TArray<FName> RowNames = PresetData->GetRowNames();
	if(RowNames.Num() == 0){return;}

	FPresetMixData NewPresetMixData;
	
	for (const auto RowName : RowNames)
	{
		if(const auto FoundRow = PresetData->FindRow<FODPresetData>(RowName, "", true))
		{
			if(CheckedPresetNames.Contains(FoundRow->PresetName.ToString()))
			{
				NewPresetMixData.PresetName = FoundRow->PresetName;
				NewPresetMixData.DistObjectData = FoundRow->DistObjectData;
				PresetMixerData.Add(NewPresetMixData);
				NewPresetMixData = FPresetMixData();
			}
		}
	}
}

void UODToolSubsystem::RegeneratePresetMixerWithCustomData(bool bIsOverridingPresetProperties, TArray<FPresetMixData>& InCustomMixData)
{
	RegeneratePresetMixerDataFromScratch();
	
	if(bIsOverridingPresetProperties && !InCustomMixData.IsEmpty())
	{
		for(auto LocalCustomData : InCustomMixData)
		{
			for(auto& LocalMixPresetData : PresetMixerData)
			{
				if(LocalCustomData.PresetName.IsEqual(LocalMixPresetData.PresetName))
				{
					LocalMixPresetData.DistObjectData = LocalCustomData.DistObjectData;
					break;
				}
			}
		}
	}
}

#pragma endregion PresetMixer


