// Copyright 2023 Leartes Studios. All Rights Reserved.

#include "ObjectDistribution/ODDistributionBase.h"
#include "ActorGroupingUtils.h"
#include "Editor.h"
#include "EditorActorFolders.h"
#include "HairStrandsInterface.h"
#include "LevelEditor.h"
#include "ODPresetData.h"
#include "ODToolSettings.h"
#include "Development/ODDebug.h"
#include "UI/ODToolWindow.h"
#include "ODToolSubsystem.h"
#include "UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"
#include "Subsystems/EditorActorSubsystem.h"
#include "Engine/StaticMeshActor.h"
#include "Kismet/GameplayStatics.h"
#include "ScopedTransaction.h"
#include "Components/BillboardComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Modules/ModuleManager.h"
#include "Development/ODSpawnCenter.h"

void UODDistributionBase::Setup(UODToolWindow* InToolWindow)
{
	ToolWindow = InToolWindow;
	ToolSubsystem = GEditor->GetEditorSubsystem<UODToolSubsystem>();
	if(!ToolSubsystem || !ToolSubsystem->GetODToolSettings())
	{
		UE_LOG(LogTemp,Error,TEXT("ToolSubsystem Not Found!"));
		return;
	}
	ObjectDistributionData = ToolSubsystem->ObjectDistributionData;
	ToolSubsystem->OnPresetLoaded.BindUObject(this,&UODDistributionBase::OnPresetLoaded);

	ToolSubsystem->GetODToolSettings()->bDrawSpawnBounds;

	bIsInMixerMode = ToolSubsystem->bInMixerMode;

	if(bIsInMixerMode)
	{
		if(ToolSubsystem->GetPresetMixerMapData().Num() < 2)
		{
			ToolSubsystem->ToggleMixerMode();
		}
	}
	
	ToolSubsystem->OnAMixerPresetCheckStatusChanged.BindUObject(this,&UODDistributionBase::OnAMixerPresetCheckStatusChanged);
	
	FEditorDelegates::BeginPIE.AddUObject(this, &UODDistributionBase::HandleBeginPIE);
	FEditorDelegates::EndPIE.AddUObject(this, &UODDistributionBase::HandleEndPIE);

	CalculateTotalSpawnCount();

	LoadDistData();
}

void UODDistributionBase::LoadDistData()
{
	if(!ToolSubsystem){return;}

	if(const auto ToolSettings = ToolSubsystem->GetODToolSettings())
	{
		bSimulatePhysics = ToolSettings->bSimulatePhysics;
		KillZ = ToolSettings->KillZ;
		bDisableSimAfterFinish = ToolSettings->bDisableSimAfterFinish;
	
		bDrawSpawnBounds = ToolSettings->bDrawSpawnBounds;
		BoundsColor = ToolSettings->BoundsColor;
		FinishType = ToolSettings->FinishType;
		TargetType = ToolSettings->TargetType;
	}
}

void UODDistributionBase::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	UObject::PostEditChangeProperty(PropertyChangedEvent);
	
	//Dont delete in PIE
	if(IsInPıe() || !ToolSubsystem){return;}

	if(GetDistributionType() != DistributionType)
	{
		if(OnDistributionTypeChangedSignature.ExecuteIfBound(DistributionType))
		{
			return;
		}
	}
	
	if(	PropertyChangedEvent.GetPropertyName() == TEXT("bDrawSpawnBounds")||
		PropertyChangedEvent.GetPropertyName() == TEXT("BoundsColor"))
	{
		if(ToolSubsystem)
		{
			if(const auto ODToolSettings = ToolSubsystem->GetODToolSettings())
			{
				ODToolSettings->bDrawSpawnBounds = bDrawSpawnBounds;
				ODToolSettings->BoundsColor = BoundsColor;

				if(PropertyChangedEvent.ChangeType != EPropertyChangeType::Interactive)
				{
					ODToolSettings->SaveConfig();
					return;
				}
			}
		}
	}

	if(	PropertyChangedEvent.GetPropertyName() == TEXT("FinishType")||
		PropertyChangedEvent.GetPropertyName() == TEXT("TargetType"))
	{
		if(ToolSubsystem)
		{
			if(const auto ODToolSettings = ToolSubsystem->GetODToolSettings())
			{
				ODToolSettings->FinishType = FinishType;
				ODToolSettings->TargetType = TargetType;

				ODToolSettings->SaveConfig();
			}
		}
	}
	
	if(ToolWindow && ToolWindow->GetSpawnCenterRef())
	{
		if(const auto SpawnCenter = Cast<AODSpawnCenter>(ToolWindow->GetSpawnCenterRef()))
		{
			SpawnCenter->GetBillboardComp()->SetVisibility(false);
			SpawnCenter->GetBillboardComp()->SetVisibility(true);
			SpawnCenter->RegenerateBoundsDrawData();
		}
	}
	
	if(!ToolSubsystem->bInMixerMode)
	{
		ToolSubsystem->ObjectDistributionData = ObjectDistributionData;
	}

	if(ToolSubsystem->bInMixerMode)
	{
		if(PropertyChangedEvent.GetPropertyName() == TEXT("bOverridePresetProperties"))
		{
			if(ToolSubsystem)
			{
				ToolSubsystem->RegeneratePresetMixerWithCustomData(bOverridePresetProperties,CustomPresetData);
				CalculateTotalSpawnCount();
			}
			return;
		}
		if(	PropertyChangedEvent.GetPropertyName() == TEXT("bSelectRandomMaterial")||
			PropertyChangedEvent.GetPropertyName() == TEXT("bSelectMaterialRandomly")||
			PropertyChangedEvent.GetPropertyName() == TEXT("StaticMesh")||
			PropertyChangedEvent.GetPropertyName() == TEXT("SecondRandomMaterial"))
		{
			ToolSubsystem->RegeneratePresetMixerWithCustomData(bOverridePresetProperties,CustomPresetData);
			return;
		}
	}
	
	if(PropertyChangedEvent.GetPropertyName() == TEXT("EditablePresetList"))
	{
		if(!ToolSubsystem || EditablePresetList.Equals(FString(TEXT("Select A Preset")))){return;}
		
		if(const auto FoundPresetData = ToolSubsystem->GetPresetData(EditablePresetList))
		{
			FPresetMixData NewPresetMixData;
			NewPresetMixData.PresetName = FName(*EditablePresetList);
			NewPresetMixData.DistObjectData = FoundPresetData->DistObjectData;
			CustomPresetData.Add(NewPresetMixData);
			++CustomPresetDataNum;
		}
		EditablePresetList = FString(TEXT("Select A Preset"));
		return;
	}
	
	if(PropertyChangedEvent.GetPropertyName() == TEXT("CustomPresetData"))
	{
		//Check If Added Manually
		const int32 Num =  CustomPresetData.Num();
		if(Num > 0 && CustomPresetData[Num - 1].PresetName.IsNone()) //If added empty slot manually
		{
			CustomPresetData.RemoveAt(Num - 1);
			return;
		}
		
		if(CustomPresetData.IsEmpty()) // If Emptied Manually
		{
			CustomPresetDataNum = 0;
		}
		else if(CustomPresetDataNum != CustomPresetData.Num()) // If Deleted Only One Manually
		{
			--CustomPresetDataNum;
		}

		if(ToolSubsystem){ToolSubsystem->RegeneratePresetMixerWithCustomData(bOverridePresetProperties,CustomPresetData);}
		CalculateTotalSpawnCount();
		return;
	}

	CheckForSimulationChanges();
	
	if(	PropertyChangedEvent.GetPropertyName() == TEXT("OrientationType")||	
		PropertyChangedEvent.GetPropertyName() == TEXT("ScaleRange")||
		PropertyChangedEvent.GetPropertyName() == TEXT("X")||
		PropertyChangedEvent.GetPropertyName() == TEXT("Y")||
		PropertyChangedEvent.GetPropertyName() == TEXT("LinearDamping")||
		PropertyChangedEvent.GetPropertyName() == TEXT("AngularDamping")||
		PropertyChangedEvent.GetPropertyName() == TEXT("Mass"))
	{
		if(ToolSubsystem->bInMixerMode)
		{
			ToolSubsystem->RegeneratePresetMixerWithCustomData(bOverridePresetProperties,CustomPresetData);
		}
		//CalculateTotalSpawnCount();
	}
	else if(
		PropertyChangedEvent.GetPropertyName() == TEXT("bSpawnDensity") ||
		PropertyChangedEvent.GetPropertyName() == TEXT("SpawnDensity") ||
		PropertyChangedEvent.GetPropertyName() == TEXT("SpawnCount") ||
		PropertyChangedEvent.GetPropertyName() == TEXT("MaxCollisionTest"))
				
	{
		if(ToolSubsystem->bInMixerMode)
		{
			ToolSubsystem->RegeneratePresetMixerWithCustomData(bOverridePresetProperties,CustomPresetData);
		}
		
		CalculateTotalSpawnCount();
		return;
	}
	else if(
		PropertyChangedEvent.GetPropertyName() == TEXT("bSimulatePhysics") ||
		PropertyChangedEvent.GetPropertyName() == TEXT("KillZ") ||
		PropertyChangedEvent.GetPropertyName() == TEXT("bDisableSimAfterFinish"))
	{
		ToolSubsystem->GetODToolSettings()->bSimulatePhysics = bSimulatePhysics;
		ToolSubsystem->GetODToolSettings()->KillZ = KillZ;
		ToolSubsystem->GetODToolSettings()->bDisableSimAfterFinish = bDisableSimAfterFinish;
		
		ToolSubsystem->GetODToolSettings()->SaveConfig();

		SetSimulatePhysics();
		return;
	}
	
	CalculateTotalSpawnCount();
	ReDesignObjects();
}

void UODDistributionBase::CreatePresetModeObjects(TArray<AStaticMeshActor*>& InSpawnedActors)
{
	int32 TotalSpawnedNum = InSpawnedActors.Num();
	
	int32 TotalIndex = 0;
	int32 DistIndex = 0;
	
	for(const auto& ObjectDistData : ObjectDistributionData) //For each unique static meshes
	{
		if(ObjectDistData.StaticMesh.IsNull()){continue;}
		
		FDistObjectData CommonData = ObjectDistData;
		
		if(bOverrideDistributionData)
		{
			CommonData.DistributionProperties = GlobalDistributionData;
		}
		
		int32 CurrentDataSpawnCount = CommonData.DistributionProperties.SpawnCount;

		if(bSpawnDensity && SpawnDensity != 0)
		{
			if(DistIndex == ObjectDistributionData.Num() - 1)
			{
				CurrentDataSpawnCount = TotalSpawnedNum;
			}
			else
			{
				CurrentDataSpawnCount = FMath::RoundToInt(static_cast<float>(CurrentDataSpawnCount) /  static_cast<float>(GetInitialTotalCount()) * TotalSpawnCount);
			}
		}
		if(CurrentDataSpawnCount > 0)
		{
			ToolSubsystem->ObjectDataMap.Add(DistIndex,CommonData);

			const auto LoadedSM = CommonData.StaticMesh.LoadSynchronous();

			if(!IsValid(LoadedSM)){continue;}

			for(int32 Index = 0 ; Index < CurrentDataSpawnCount ; Index++) //For each Related Static Mesh 
			{
				if(!InSpawnedActors.IsValidIndex(TotalIndex)){break;}

				AStaticMeshActor* SpawnedObject = InSpawnedActors[TotalIndex];
				SpawnedObject->GetStaticMeshComponent()->SetStaticMesh(LoadedSM);

				//Add New Material
				if(CommonData.bSelectRandomMaterial && !CommonData.SecondRandomMaterial.IsNull())
				{
					if(CommonData.bSelectMaterialRandomly)
					{
						if(FMath::RandBool())
						{
							if(const auto LoadedMaterial = CommonData.SecondRandomMaterial.LoadSynchronous())
							{
								SpawnedObject->GetStaticMeshComponent()->SetMaterial(0,LoadedMaterial);
							}
						}
					}
					else
					{
						if(const auto LoadedMaterial = CommonData.SecondRandomMaterial.LoadSynchronous())
						{
							SpawnedObject->GetStaticMeshComponent()->SetMaterial(0,LoadedMaterial);
						}
					}
				}
				
				SpawnedObject->GetStaticMeshComponent()->SetLinearDamping(CommonData.DistributionProperties.LinearDamping);
				SpawnedObject->GetStaticMeshComponent()->SetAngularDamping(CommonData.DistributionProperties.AngularDamping);
				SpawnedObject->GetStaticMeshComponent()->SetMassOverrideInKg("",CommonData.DistributionProperties.Mass,true);
				SpawnedObject->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
				SpawnedObject->GetStaticMeshComponent()->SetSimulatePhysics(bSimulatePhysics);
				SpawnedObject->Tags.Add(FName(*FString::FromInt(DistIndex)));
				SpawnedObject->SetFolderPath(FName(TEXT("DistributedActors")));
				TotalIndex++;
				
				FName Name = MakeUniqueObjectName(GetWorld(), AStaticMeshActor::StaticClass(), FName(*CommonData.StaticMesh->GetName()));
				SpawnedObject->SetActorLabel(Name.ToString());
			}
			DistIndex++;
		}

		TotalSpawnedNum -= CurrentDataSpawnCount;
		if(TotalSpawnedNum < 0)
		{
			TotalSpawnedNum = 0;
		}
	}
}

void UODDistributionBase::CreateMixerModeObjects(TArray<AStaticMeshActor*>& InSpawnedActors)
{
	int32 TotalSpawnedNum = InSpawnedActors.Num();
	
	int32 TotalIndex = 0;
	int32 DistIndex = 0;
	
	for(auto& CurrentMixPreset : ToolSubsystem->GetPresetMixerData())
	{
		for(const auto& ObjectDistData : CurrentMixPreset.DistObjectData) //For each unique static meshes
		{
			if(ObjectDistData.StaticMesh.IsNull()){continue;}
			
			FDistObjectData CommonData = ObjectDistData;
			
			if(bOverridePresetProperties && bOverrideDistributionData)
			{
				CommonData.DistributionProperties = GlobalDistributionData;
			}
			
			int32 CurrentDataSpawnCount = CommonData.DistributionProperties.SpawnCount;
			
			if(bOverridePresetProperties && bSpawnDensity && SpawnDensity != 0)
			{
				if(DistIndex == ObjectDistributionData.Num() - 1)
				{
					CurrentDataSpawnCount = TotalSpawnedNum;
				}
				else
				{
					CurrentDataSpawnCount = FMath::RoundToInt(static_cast<float>(CurrentDataSpawnCount) /  static_cast<float>(GetInitialTotalCount()) * TotalSpawnCount);
				}
			}
			if(CurrentDataSpawnCount > 0)
			{
				ToolSubsystem->ObjectDataMap.Add(DistIndex,CommonData);

				const auto LoadedSM = CommonData.StaticMesh.LoadSynchronous();

				if(!IsValid(LoadedSM)){continue;}

				for(int32 Index = 0 ; Index < CurrentDataSpawnCount ; Index++) //For each Related Static Mesh 
				{
					if(!InSpawnedActors.IsValidIndex(TotalIndex)){break;}

					AStaticMeshActor* SpawnedObject = InSpawnedActors[TotalIndex];
					SpawnedObject->GetStaticMeshComponent()->SetStaticMesh(LoadedSM);
					
					if(CommonData.bSelectRandomMaterial && !CommonData.SecondRandomMaterial.IsNull())
					{
						if(CommonData.bSelectMaterialRandomly)
						{
							if(FMath::RandBool())
							{
								if(const auto LoadedMaterial = CommonData.SecondRandomMaterial.LoadSynchronous())
								{
									SpawnedObject->GetStaticMeshComponent()->SetMaterial(0,LoadedMaterial);
								}
							}
						}
						else
						{
							if(const auto LoadedMaterial = CommonData.SecondRandomMaterial.LoadSynchronous())
							{
								SpawnedObject->GetStaticMeshComponent()->SetMaterial(0,LoadedMaterial);
							}
						}
					}
					
					SpawnedObject->GetStaticMeshComponent()->SetLinearDamping(CommonData.DistributionProperties.LinearDamping);
					SpawnedObject->GetStaticMeshComponent()->SetAngularDamping(CommonData.DistributionProperties.AngularDamping);
					SpawnedObject->GetStaticMeshComponent()->SetMassOverrideInKg("",CommonData.DistributionProperties.Mass,true);
					SpawnedObject->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
					SpawnedObject->GetStaticMeshComponent()->SetSimulatePhysics(bSimulatePhysics);
					SpawnedObject->Tags.Add(FName(*FString::FromInt(DistIndex)));
					SpawnedObject->SetFolderPath(FName(TEXT("DistributedActors")));
					TotalIndex++;
					
					FName Name = MakeUniqueObjectName(GetWorld(), AStaticMeshActor::StaticClass(), FName(*CommonData.StaticMesh->GetName()));
					SpawnedObject->SetActorLabel(Name.ToString());
				}
				DistIndex++;
			}

			TotalSpawnedNum -= CurrentDataSpawnCount;
			if(TotalSpawnedNum < 0)
			{
				TotalSpawnedNum = 0;
			}
		}
	}
}

FReply UODDistributionBase::OnFinishDistributionPressed()
{
	//If is in pie then keep changes first
	if(ToolSubsystem)
	{
		if(ToolSubsystem->CreatedDistObjects.IsEmpty())
		{
			OnFinishConditionChangeSignature.ExecuteIfBound(false);
			return FReply::Handled();
		}
	}

	
	// Stop simulation if it is pie
	if(IsInPıe())
	{
		TArray<AActor*> FoundActors;
		UGameplayStatics::GetAllActorsOfClass(GEditor->PlayWorld, AStaticMeshActor::StaticClass(), FoundActors);

		TArray<AActor*> FilteredActors;
		for(auto FoundActor : FoundActors)
		{
			if(FoundActor->GetRootComponent()->IsSimulatingPhysics())
			{
				FilteredActors.Add(FoundActor);
			}
		}
	
		KeepSimulationChanges(FilteredActors);
		bAreAssetsSimulated = true;
		ODDebug::ShowNotifyInfo(TEXT("Positions Saved."));
		
		OnStopSimulationPressed(); //Stop
	}
	
	bAreAssetsSimulated = false;

	if(!ToolSubsystem->CreatedDistObjects.IsEmpty())
	{
		if(FinishType == EODDistributionFinishType::Keep)
		{
			FinishDistAsKeepActors();
		}
		else if(FinishType == EODDistributionFinishType::Batch)
		{
			if(ToolSubsystem)
			{
				ToolSubsystem->CreateInstanceFromDistribution(TargetType);
			}
		}
		else
		{
			FinishDistAsRunningMergeToll();
		}

	}
	
	ToolSubsystem->CreatedDistObjects.Empty();
	ToolSubsystem->DistObjectData.Empty();
	ToolSubsystem->ObjectDataMap.Empty();
	ToolSubsystem->ObjectDistributionData.Empty();
	
	OnAfterODRegenerated.ExecuteIfBound();
	OnFinishConditionChangeSignature.ExecuteIfBound(false);
	return FReply::Handled();
	
}

void UODDistributionBase::FinishDistAsKeepActors() const
{
	if(bDisableSimAfterFinish)
	{
		for(const auto CurrentObject : ToolSubsystem->CreatedDistObjects)
		{
			CurrentObject->GetStaticMeshComponent()->SetSimulatePhysics(false);
			CurrentObject->GetStaticMeshComponent()->SetMobility(EComponentMobility::Static);
		}
	}
		
	const FName GeneratedPresetPath = GenerateUniquePresetPath(ToolSubsystem->CreatedDistObjects[0]);
		
	TArray<AActor*> Actors;
	for(const auto CurrentActor : ToolSubsystem->CreatedDistObjects)
	{
		if(IsValid(CurrentActor))
		{
			CurrentActor->SetFolderPath(GeneratedPresetPath);
			Actors.Add(CurrentActor);
		}
	}

	if(const auto GroupingUtilities = UActorGroupingUtils::Get())
	{
		const bool bIsGroupingActive = GroupingUtilities->IsGroupingActive();
		GroupingUtilities->SetGroupingActive(true);
		UActorGroupingUtils::Get()->GroupActors(Actors);
		GroupingUtilities->SetGroupingActive(bIsGroupingActive);
	}
		
	if(const auto EditorActorSubsystem = GEditor->GetEditorSubsystem<UEditorActorSubsystem>())
	{
		EditorActorSubsystem->SelectNothing();
		EditorActorSubsystem->SetActorSelectionState(Actors[0],true);
	}
	
}

void UODDistributionBase::FinishDistAsRunningMergeToll() const
{
	if(const auto EditorActorSubsystem = GEditor->GetEditorSubsystem<UEditorActorSubsystem>())
	{
		EditorActorSubsystem->SelectNothing();
		
		TArray<AActor*> Actors;
		for(const auto CurrentActor : ToolSubsystem->CreatedDistObjects)
		{
			if(IsValid(CurrentActor))
			{
				Actors.Add(CurrentActor);
			}
		}
		
		EditorActorSubsystem->SetSelectedLevelActors(Actors);
		
		FGlobalTabmanager::Get()->TryInvokeTab(FName("MergeActors"));
	}
}

FName UODDistributionBase::GenerateUniquePresetPath(const AActor* InSampleActor) const
{
	if(!IsValid(InSampleActor) || !ToolSubsystem || !InSampleActor->GetWorld()){return FName();}

	TArray<FString> CollectedFolderNames;
	FActorFolders::Get().ForEachFolderWithRootObject(*InSampleActor->GetWorld(), InSampleActor->GetFolder().GetRootObject(), [&CollectedFolderNames](const FFolder& Folder)
	{
		CollectedFolderNames.Add(Folder.ToString());
		return true;
	});

	const FString PresetName = ToolSubsystem->GetLastSelectedPreset().ToString();

	int32 PresetVariations = 0;
	for(auto CurrentFName : CollectedFolderNames)
	{
		if(CurrentFName.Contains(PresetName))
		{
			++PresetVariations;
		}
	}

	if(PresetVariations < 2)
	{
		PresetVariations = 1;
	}
	
	return FName(*FString::Printf(TEXT("DistributedActors/%s/%s_V%s"),*PresetName,*PresetName,*FString::FromInt(PresetVariations)));
}

FReply UODDistributionBase::OnShuffleDistributionPressed()
{
	//Dont shuffle in PIE
	if(IsInPıe()){return FReply::Handled();}

	auto ShuffleArrayLambda = [](TArray<AStaticMeshActor*>& Array) {
		const int32 ArraySize = Array.Num();
		for (int32 i = 0; i < ArraySize - 1; ++i) {
			const int32 RandomIndex = FMath::RandRange(i, ArraySize - 1);
			if (RandomIndex != i) {
				Array.Swap(i, RandomIndex);
			}
		}
	};
	
	CheckForSimulationChanges();
	
	if(ToolSubsystem->CreatedDistObjects.Num() == 0){return FReply::Handled();}

	ShuffleArrayLambda(ToolSubsystem->CreatedDistObjects);

	ReDesignObjects();
	return FReply::Handled();
}


FReply UODDistributionBase::OnDeleteDistributionPressed()
{
	//Dont delete in PIE
	if(IsInPıe()){return FReply::Handled();}
	
	CheckForSimulationChanges();
	
	if(ToolSubsystem->CreatedDistObjects.Num() == 0){return FReply::Handled();}

	DestroyObjects();

	OnFinishConditionChangeSignature.ExecuteIfBound(false);
	return FReply::Handled();
}
FReply UODDistributionBase::OnStartSimulationPressed()
{
	if(!IsInPıe())
	{
		CheckForSimulationChanges();
	
		if(!GUnrealEd->IsPlayingSessionInEditor())
		{
			FLevelEditorModule& LevelEditorModule = FModuleManager::Get().GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
			const auto ActiveViewport = LevelEditorModule.GetFirstActiveViewport();
			FRequestPlaySessionParams RequestPlaySessionParams;
			RequestPlaySessionParams.DestinationSlateViewport = ActiveViewport;
			RequestPlaySessionParams.WorldType = EPlaySessionWorldType::SimulateInEditor;
			GUnrealEd->RequestPlaySession(RequestPlaySessionParams);
		}
	}
	return FReply::Handled();
}

FReply UODDistributionBase::OnStopSimulationPressed()
{
	if(IsInPıe())
	{
		if(GUnrealEd->IsPlayingSessionInEditor())
		{
			GUnrealEd->EndPlayMap();	
		}
	}
	return FReply::Handled();
}

FReply UODDistributionBase::OnResumeSimulationPressed()
{
	if(IsInPıe())
	{
		if(GUnrealEd->IsPlayingSessionInEditor())
		{
			GUnrealEd->SetPIEWorldsPaused(false);	
		}
	}
	return FReply::Handled();
}

FReply UODDistributionBase::OnPauseSimulationPressed()
{
	if(GUnrealEd->IsPlayingSessionInEditor())
	{
		GUnrealEd->SetPIEWorldsPaused(true);	
	}
	
	return FReply::Handled();
}

FReply UODDistributionBase::ResetParametersPressed()
{
	if(ToolSubsystem && ToolSubsystem->GetODToolSettings())
	{
		ToolSubsystem->GetODToolSettings()->LoadDefaultSettings();
		LoadDistData();


		if(ToolWindow && ToolWindow.Get()->GetSpawnCenterRef())
		{
			if(const auto SpawnCenter = Cast<AODSpawnCenter>(ToolWindow->GetSpawnCenterRef()))
			{
				SpawnCenter->RegenerateBoundsDrawData();
			}
		}
	}
	return FReply::Handled();
}

FReply UODDistributionBase::OnCreateDistributionPressed()
{
	//Dont Create in PIE
	if(IsInPıe()){return FReply::Handled();;}
	
	CheckForSimulationChanges();
	
	if(ToolSubsystem->CreatedDistObjects.Num() > 0){DestroyObjects();}

	if(ToolSubsystem)
	{
		ToolSubsystem->ObjectDataMap.Empty();
	}

	if(TotalSpawnCount <= 0)
	{
		OnFinishConditionChangeSignature.ExecuteIfBound(false);
		return FReply::Handled();;
	}
	
	const auto EditorActorSubsystem = GEditor->GetEditorSubsystem<UEditorActorSubsystem>();
	if(!EditorActorSubsystem) {return FReply::Handled();;}
	EditorActorSubsystem->SelectNothing();

	
	//Spawn Actors
	TArray<AStaticMeshActor*> SpawnedObjects = SpawnAndGetEmptyStaticMeshActors(TotalSpawnCount);
	if(SpawnedObjects.Num() == 0){UE_LOG(LogTemp,Error,TEXT("Failed To Spawn Objects."));return FReply::Handled();;}
	
	if(ToolSubsystem->bInMixerMode)
	{
		CreateMixerModeObjects(SpawnedObjects);
	}
	else
	{
		CreatePresetModeObjects(SpawnedObjects);
	}
	
	ToolSubsystem->CreatedDistObjects = SpawnedObjects;
	
	ReDesignObjects();
	
	if(ToolWindow && ToolWindow.Get()->GetSpawnCenterRef())
	{
		EditorActorSubsystem->SetActorSelectionState(ToolWindow.Get()->GetSpawnCenterRef(),true);
	}
	
	OnFinishConditionChangeSignature.ExecuteIfBound(true);
	return FReply::Handled();
}




FReply UODDistributionBase::OnSelectObjectsPressed()
{
	if(ToolSubsystem->CreatedDistObjects.IsEmpty()){return FReply::Handled();}
	
	const auto EditorActorSubsystem = GEditor->GetEditorSubsystem<UEditorActorSubsystem>();
	
	TArray<AActor*> Actors;
	for (auto StaticMeshActor : ToolSubsystem->CreatedDistObjects)
	{
		AActor* Actor = Cast<AActor>(StaticMeshActor); 
		if (IsValid(Actor))
		{
			Actors.Add(Actor); 
		}
	}
	
	EditorActorSubsystem->SelectNothing();
	EditorActorSubsystem->SetSelectedLevelActors(Actors);
	
	return FReply::Handled();
}


void UODDistributionBase::LevelActorDeleted(const AActor* InActor) const
{
	if(ToolSubsystem && !ToolSubsystem->CreatedDistObjects.IsEmpty())
	{
		int32 FoundIndex = -1;
		const int32 Num = ToolSubsystem->CreatedDistObjects.Num();
		for(int32 Index = 0; Index < Num ; Index++)
		{
			if(ToolSubsystem->CreatedDistObjects[Index] == InActor)
			{
				FoundIndex = Index;
				break;
			}
		}
		if(FoundIndex >= 0)
		{
			ToolSubsystem->CreatedDistObjects.RemoveAt(FoundIndex);
		}
	}
}



TArray<AStaticMeshActor*> UODDistributionBase::SpawnAndGetEmptyStaticMeshActors(const int32& InSpawnCount)
{
	TArray<AStaticMeshActor*> LocalObjects;
	
	for(int Index = 0; Index < InSpawnCount ; Index++)
	{
		LocalObjects.Add(SpawnAndGetStaticMeshActor());
	}
	return LocalObjects;
}

AStaticMeshActor* UODDistributionBase::SpawnAndGetStaticMeshActor()
{
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.bNoFail = true;
	UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
	return EditorWorld->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(),FVector::ZeroVector,FRotator::ZeroRotator,SpawnParams);
}

int32 UODDistributionBase::CalculateSpawnCount()
{
	return TotalSpawnCount;
}

void UODDistributionBase::DestroyObjects() const
{
	const int32 Num = ToolSubsystem->CreatedDistObjects.Num();
	for(int32 Index = 0; Index < Num ; Index++)
	{
		const auto LocalObject = ToolSubsystem->CreatedDistObjects[Num - Index -1];
		ToolSubsystem->CreatedDistObjects.RemoveAt(Num - Index -1);
		if(LocalObject && !LocalObject->IsActorBeingDestroyed())
		{
			LocalObject->Destroy();
		}
	}
	ToolSubsystem->CreatedDistObjects.Empty();
}

bool UODDistributionBase::CheckForSpotSuitability(TObjectPtr<AStaticMeshActor> InSmActor) const
{
	const UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
	if(!EditorWorld){return false;}
	FHitResult HitResult;
	TArray<AActor*> ActorToIgnore;
	ActorToIgnore.Add(InSmActor);

	FVector Origin = InSmActor->GetStaticMeshComponent()->Bounds.Origin;
	const float Radius = InSmActor->GetStaticMeshComponent()->Bounds.SphereRadius;
	
	const ETraceTypeQuery TraceType = UEngineTypes::ConvertToTraceType(ECC_Visibility);
	UKismetSystemLibrary::SphereTraceSingle(EditorWorld,Origin,Origin,Radius,TraceType,false,ActorToIgnore,EDrawDebugTrace::None,HitResult,true);

	
	if(HitResult.bBlockingHit)
	{
		for(int32 Index = 0; Index < MaxCollisionTest ; Index++)
		{
			Origin = InSmActor->GetStaticMeshComponent()->Bounds.Origin;
			
			InSmActor->AddActorWorldOffset(FVector(FMath::RandRange(-1.0f,1.0f),FMath::RandRange(-1.0f,1.0f),FMath::RandRange(-1.0f,1.0f)).GetSafeNormal() * Radius * FMath::Sin(Index + 1.0f));
			
			UKismetSystemLibrary::SphereTraceSingle(EditorWorld,Origin,Origin,Radius,TraceType,false,ActorToIgnore,EDrawDebugTrace::None,HitResult,true);

			if(!HitResult.bBlockingHit)
			{
				return true;
			}
		}
		return false;
	}
	
	return true;
}

void UODDistributionBase::CalculateTotalSpawnCount()
{
	int32 TotalCount = 0;
	int32 UniqueSlots = 0;

	if(ToolSubsystem->bInMixerMode)
	{
		if(bOverridePresetProperties && bOverrideDistributionData)
		{
			for(auto& LocalPresetMixData : ToolSubsystem->GetPresetMixerData())
			{
				for(auto& LocalData : LocalPresetMixData.DistObjectData)
				{
					if(!LocalData.StaticMesh.IsNull())
					{
						TotalCount += GlobalDistributionData.SpawnCount;
						++UniqueSlots;
					}
				}
			}
		}
		else
		{
			for(auto& LocalPresetMixData : ToolSubsystem->GetPresetMixerData())
			{
				for(auto& LocalData : LocalPresetMixData.DistObjectData)
				{
					if(!LocalData.StaticMesh.IsNull())
					{
						TotalCount += LocalData.DistributionProperties.SpawnCount;
						++UniqueSlots;
					}
				}
			}
		}
	}
	else
	{
		if(bOverrideDistributionData)
		{
			for(auto LocalData : ObjectDistributionData)
			{
				if(!LocalData.StaticMesh.IsNull())
				{
					TotalCount += GlobalDistributionData.SpawnCount;
					++UniqueSlots;
				}
			}
		}
		else
		{
			for(auto LocalData : ObjectDistributionData)
			{
				if(!LocalData.StaticMesh.IsNull())
				{
					TotalCount += LocalData.DistributionProperties.SpawnCount;
					++UniqueSlots;
				}
			}
		}
	}
	
	if(TotalCount <= 0)
	{
		TotalSpawnCount = 0;
		return;
	}

	if(bSpawnDensity && SpawnDensity != 0)
	{
		if(ToolSubsystem->bInMixerMode && !bOverridePresetProperties)
		{
			TotalSpawnCount = TotalCount;
			return;
		}
		
		if(SpawnDensity > 0)
		{
			TotalSpawnCount = TotalCount + FMath::RoundToInt(static_cast<float>(TotalCount) / static_cast<float>(UniqueSlots) *  SpawnDensity * 50.0f);
		}
		else
		{
			TotalSpawnCount = FMath::Clamp(FMath::RoundToInt(TotalCount + (SpawnDensity * TotalCount)),UniqueSlots,TotalSpawnCount);
		}
	}
	else
	{
		TotalSpawnCount = TotalCount;
	}
}

int32 UODDistributionBase::GetInitialTotalCount()
{
	int32 ActualTotalCount = 0;
	if(!ToolSubsystem){return ActualTotalCount;}
	
	if(ToolSubsystem->bInMixerMode)
	{
		if(bOverrideDistributionData)
		{
			for(auto LocalPresetData : ToolSubsystem->GetPresetMixerData())
			{
				for(auto LocalData : LocalPresetData.DistObjectData)
				{
					if(!LocalData.StaticMesh.IsNull())
					{
						ActualTotalCount += GlobalDistributionData.SpawnCount;
					}
				}
			}
		}
		else
		{
			for(auto LocalPresetData : ToolSubsystem->GetPresetMixerData())
			{
				for(auto LocalData : LocalPresetData.DistObjectData)
				{
					if(!LocalData.StaticMesh.IsNull())
					{
						ActualTotalCount += LocalData.DistributionProperties.SpawnCount;
					}
				}
			}
		}
	}
	else
	{
		if(bOverrideDistributionData)
		{
			for(auto LocalData : ObjectDistributionData)
			{
				if(!LocalData.StaticMesh.IsNull())
				{
					ActualTotalCount += GlobalDistributionData.SpawnCount;
				}
			}
		}
		else
		{
			for(auto LocalData : ObjectDistributionData)
			{
				if(!LocalData.StaticMesh.IsNull())
				{
					ActualTotalCount += LocalData.DistributionProperties.SpawnCount;
				}
			}
		}
	}

	return ActualTotalCount;
}

void UODDistributionBase::ReDesignObjects()
{
	CollidingObjects = 0;

	InitialRelativeLocations.Empty();
	
	if(!ToolWindow.Get() || !ToolWindow.Get()->GetSpawnCenterRef()){return;}
	
	const int32 ObjectNum = ToolSubsystem->CreatedDistObjects.Num();

	if(ObjectNum == 0){return;}
	
	//Get Center Location
	FVector SpawnCenterLocation = FVector::ZeroVector;
	if(ToolWindow && ToolWindow.Get()->GetSpawnCenterRef())
	{
		SpawnCenterLocation = ToolWindow.Get()->GetSpawnCenterRef()->GetActorLocation();
	}
	
	//Set Location & Rotations
	for(int32 Index = 0; Index < ObjectNum ; Index++)
	{
		FVector CalculatedVec = CalculateLocation(Index,ObjectNum);
		if(const auto CreatedObject = ToolSubsystem->CreatedDistObjects[Index])
		{
			if(!CreatedObject->Tags.IsValidIndex(0)){continue;}
			
			const auto FoundData =  FindDataFromMap(CreatedObject->Tags[0]);
			if(!FoundData){continue;}

			InitialRelativeLocations.Add(CalculatedVec); //For Spawn Center Rotations

			//Add Initial Rotation Differences
			if(ToolWindow.Get() && ToolWindow.Get()->GetSpawnCenterRef())
			{
				CalculatedVec = ToolWindow->GetSpawnCenterRef()->GetActorRotation().RotateVector(CalculatedVec);
			}
			
			CalculatedVec += SpawnCenterLocation;
			CreatedObject->SetActorLocation(CalculatedVec);
			
			if(bTestForCollider)
			{
				if(!CheckForSpotSuitability(CreatedObject))
				{
					++CollidingObjects;
				}
			}
			
			if(FoundData)
			{
				CreatedObject->SetActorRotation(CalculateRotation(CalculatedVec,SpawnCenterLocation,FoundData->DistributionProperties.OrientationType));
				CreatedObject->SetActorScale3D(FVector::OneVector * FMath::RandRange(FoundData->DistributionProperties.ScaleRange.X,FoundData->DistributionProperties.ScaleRange.Y));
				CreatedObject->GetStaticMeshComponent()->SetAbsolute(false,false,true);
			}
			else
			{
				UE_LOG(LogTemp,Error,TEXT("Found Data Is Empty!"));
			}
		}
	}
	OnAfterODRegenerated.ExecuteIfBound();
}


FDistObjectData* UODDistributionBase::FindDataFromMap(const FName& InIndexName) const
{
	if(FDistObjectData* FoundData = ToolSubsystem->ObjectDataMap.Find(FCString::Atoi(*InIndexName.ToString())))
	{
		return FoundData;
	}
	return nullptr;
}

void UODDistributionBase::AddSpawnCenterMotionDifferences(const FVector& InVelocity)
{
	CheckForSimulationChanges();

	if(ToolSubsystem && !ToolSubsystem->CreatedDistObjects.IsEmpty())
	{
		for(const auto CurrentObject : ToolSubsystem->CreatedDistObjects)
		{
			if(CurrentObject){CurrentObject->AddActorWorldOffset(InVelocity);}
		}
	}
}

void UODDistributionBase::ReRotateObjectsOnSpawnCenter() const
{
	if(ToolSubsystem && !ToolSubsystem->CreatedDistObjects.IsEmpty())
	{
		if(ToolWindow && ToolWindow->GetSpawnCenterRef())
		{
			const int32 Num = ToolSubsystem->CreatedDistObjects.Num();
			for(int32 Index = 0; Index < Num ; Index++)
			{
				if(const auto CurrentObject = ToolSubsystem->CreatedDistObjects[Index])
				{
					if(InitialRelativeLocations.IsValidIndex(Index))
					{
						FVector TargetLoc = ToolWindow->GetSpawnCenterRef()->GetActorRotation().RotateVector(InitialRelativeLocations[Index]) + ToolWindow->GetSpawnCenterRef()->GetActorLocation();
						CurrentObject->SetActorLocation(TargetLoc);
					}
				}
			}
		}
	}
}

void UODDistributionBase::CheckForSimulationChanges()
{
	if(!bAreAssetsSimulated){return;}
	
	const auto ReturnValue = ODDebug::ShowMsgDialog(EAppMsgType::YesNo,TEXT("Do you want to finish simulated distribution first?"),true);
	if(ReturnValue == EAppReturnType::Yes)
	{
		OnFinishDistributionPressed();
	}
	bAreAssetsSimulated = false;
}

FVector UODDistributionBase::CalculateLocation(const int32& InIndex, const int32& InLength)
{
	return FVector::ZeroVector;
}

FRotator UODDistributionBase::CalculateRotation(const FVector& InLocation,const FVector& TargetLocation,const EObjectOrientation& Orientation)
{

	if(Orientation == EObjectOrientation::Inside)
	{
		return FRotationMatrix::MakeFromX(TargetLocation - InLocation).Rotator();

	}
	if(Orientation == EObjectOrientation::Outside)
	{
		return FRotationMatrix::MakeFromX(InLocation - TargetLocation).Rotator();

	}
	if(Orientation == EObjectOrientation::RandomZ)
	{
		FRotator RandRot;
		RandRot.Yaw = FMath::FRand() * 360.f;
		RandRot.Pitch = 0.0f;
		RandRot.Roll = 0.0f;
		return RandRot;
	}
	if(Orientation == EObjectOrientation::Random)
	{
		FRotator RandRot;
		RandRot.Yaw = FMath::FRand() * 360.f;
		RandRot.Pitch = FMath::FRand() * 360.f;
		RandRot.Roll = FMath::FRand() * 360.f;
		return RandRot;
	}
	return FRotator::ZeroRotator;
}

void UODDistributionBase::SetSimulatePhysics() const
{
	auto Objects = ToolSubsystem->CreatedDistObjects;

	for(const auto Object : Objects)
	{
		Object->GetStaticMeshComponent()->SetSimulatePhysics(bSimulatePhysics);
	}
}

bool UODDistributionBase::IsInPıe()
{
	if (GEditor == nullptr){return true;}

	if(GEditor->GetPIEWorldContext())
	{
		return true;

	}
	return false;
}

void UODDistributionBase::Tick(float DeltaTime)
{
	if ( LastFrameNumberWeTicked == GFrameCounter )
	{
		return;
	}

	if(bTraceForKillZ)
	{
		KillZCheckTimer += DeltaTime;

		if(KillZCheckTimer > 0.34)
		{
			CheckForKillZInSimulation();

			KillZCheckTimer = 0.0f;
		}
	}
	
	LastFrameNumberWeTicked = GFrameCounter;
}

void UODDistributionBase::CheckForKillZInSimulation()
{
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GEditor->PlayWorld, AStaticMeshActor::StaticClass(), FoundActors);

	if(FoundActors.Num() == 0){return;}
	
	TArray<AActor*> FilteredActors;
	for(auto FoundActor : FoundActors)
	{
		if(FoundActor->GetRootComponent()->IsSimulatingPhysics())
		{
			FilteredActors.Add(FoundActor);
		}
	}
	if(FilteredActors.Num() == 0){return;}
	
	for(const auto SimulatedActor : FilteredActors)
	{
		if(SimulatedActor->GetActorLocation().Z < KillZ)
		{
			if(const AActor* EditorActor = EditorUtilities::GetEditorWorldCounterpartActor(SimulatedActor))
			{
				ActorsInKillZ.AddUnique(*EditorActor->GetName());
				SimulatedActor->Destroy();
			}
		}
	}
}

void UODDistributionBase::HandleBeginPIE(const bool bIsSimulating)
{
	bTraceForKillZ = true;
}

void UODDistributionBase::HandleEndPIE(const bool bIsSimulating)
{
	bTraceForKillZ = false;
	KillZCheckTimer = 0.0f;
	DestroyKillZActors();
}

void UODDistributionBase::DestroyKillZActors()
{
	if(ActorsInKillZ.Num() > 0)
	{
		if(ToolSubsystem)
		{
			ToolSubsystem->DestroyKillZActors(ActorsInKillZ);
		}
	}
	ActorsInKillZ.Empty();
}


void UODDistributionBase::OnPresetLoaded()
{
	if(ToolSubsystem)
	{
		ObjectDistributionData = ToolSubsystem->ObjectDistributionData;
	}
	CalculateTotalSpawnCount();
}

TArray<FString> UODDistributionBase::GetEditableMixerPreset() const
{
	TArray<FString> EditablePreset = TArray<FString>();

	if(!ToolSubsystem){return EditablePreset;}
	
	auto PresetMixerData = ToolSubsystem->GetPresetMixerMapData();
	
	if(PresetMixerData.IsEmpty()){return EditablePreset;} //Just for to be secure

	
	//First add all checked preset names
	for(const auto& LocalPresetMap : PresetMixerData)
	{
		if(LocalPresetMap.CheckState)
		{
			EditablePreset.Add(LocalPresetMap.PresetName);
		}
	}
	
	if(CustomPresetData.IsEmpty() || EditablePreset.IsEmpty()){return EditablePreset;}
	
	for(auto LocalCustomPresetData: CustomPresetData)
	{
		if(EditablePreset.Contains(LocalCustomPresetData.PresetName.ToString()))
		{
			EditablePreset.Remove(LocalCustomPresetData.PresetName.ToString());
		}
	}
	
	return EditablePreset;
}

void UODDistributionBase::OnAMixerPresetCheckStatusChanged(bool InNewCheckStatus,FName InUncheckedPresetName)
{
	if(CustomPresetData.IsEmpty())
	{
		CalculateTotalSpawnCount();

		return;
	}
	const auto Num  = CustomPresetData.Num();
	for(int32 Index = 0 ; Index < Num ; ++Index)
	{
		if(CustomPresetData[Num - Index - 1].PresetName.IsEqual(InUncheckedPresetName))
		{
			CustomPresetData.RemoveAt(Num - Index - 1);
			--CustomPresetDataNum;
			break;			
		}
	}
	CalculateTotalSpawnCount();
}


void UODDistributionBase::KeepSimulationChanges(const TArray<AActor*>& SimActors) const
{
	int32 UpdatedActorCount = 0;
	int32 TotalCopiedPropertyCount = 0;
	FString FirstUpdatedActorLabel;
	const FScopedTransaction Transaction( NSLOCTEXT( "LevelEditorCommands", "KeepSimulationChanges", "Keep Simulation Changes" ) );
	
	for(const auto ActorIt : SimActors)
	{
		auto* SimWorldActor = CastChecked<AActor>( ActorIt);

		// Find our counterpart actor
		AActor* EditorWorldActor = EditorUtilities::GetEditorWorldCounterpartActor( SimWorldActor );
		if( EditorWorldActor != nullptr)
		{
			constexpr auto CopyOptions = static_cast<EditorUtilities::ECopyOptions::Type>(EditorUtilities::ECopyOptions::CallPostEditChangeProperty |
			EditorUtilities::ECopyOptions::CallPostEditMove |
			EditorUtilities::ECopyOptions::OnlyCopyEditOrInterpProperties |
			EditorUtilities::ECopyOptions::FilterBlueprintReadOnly);
			const int32 CopiedPropertyCount = EditorUtilities::CopyActorProperties( SimWorldActor, EditorWorldActor, CopyOptions );

			if( CopiedPropertyCount > 0 )
			{
				++UpdatedActorCount;
				TotalCopiedPropertyCount += CopiedPropertyCount;

				if( FirstUpdatedActorLabel.IsEmpty() )
				{
					FirstUpdatedActorLabel = EditorWorldActor->GetActorLabel();
				}
			}
		}
	}
}

FReply UODDistributionBase::SelectSpawnCenterPressed()
{
	if(!ToolWindow || !ToolWindow->GetSpawnCenterRef()){FReply::Handled();}
	
	const auto EditorActorSubsystem = GEditor->GetEditorSubsystem<UEditorActorSubsystem>();
	EditorActorSubsystem->SelectNothing();
	EditorActorSubsystem->SetActorSelectionState(ToolWindow->GetSpawnCenterRef(),true);
	return FReply::Handled();
}

FReply UODDistributionBase::MoveSpawnCenterToWorldOriginPressed()
{
	if(ToolWindow && ToolWindow->GetSpawnCenterRef())
	{
		Cast<AODSpawnCenter>(ToolWindow->GetSpawnCenterRef())->SetActorLocation(FVector::ZeroVector);

		SelectSpawnCenterPressed();
	}

	
	return FReply::Handled();
}

FReply UODDistributionBase::MoveSpawnCenterToCameraPressed()
{
	if(!ToolWindow){FReply::Handled();}
	
	ToolWindow->GetSpawnCenterToCameraView();
	
	SelectSpawnCenterPressed();

	return FReply::Handled();
}



void UODDistributionBase::BeginDestroy()
{
	ToolWindow = nullptr;

	if(ToolSubsystem && ToolSubsystem->OnPresetLoaded.IsBound())
	{
		ToolSubsystem->OnPresetLoaded.Unbind();
		ToolSubsystem->OnAMixerPresetCheckStatusChanged.Unbind();
	}

	FEditorDelegates::BeginPIE.RemoveAll(this);
	FEditorDelegates::EndPIE.RemoveAll(this);
	
	UObject::BeginDestroy();
}