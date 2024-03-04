// Copyright 2023 Leartes Studios. All Rights Reserved.

#include "ODToolWindow.h"
#include "Editor.h"
#include "EditorUtilityLibrary.h"
#include "LevelEditorViewport.h"
#include "ODAssetBorder.h"
#include "ODDebug.h"
#include "ODDiskDistribution.h"
#include "ODDistributionLine.h"
#include "ODPresetObject.h"
#include "ODHelixDistribution.h"
#include "Components/Button.h"
#include "Components/DetailsView.h"
#include "Components/EditableTextBox.h"
#include "ODSpawnCenter.h"
#include "ODToolSubsystem.h"
#include "Editor/UnrealEdEngine.h"
#include "ODDistributionBase.h"
#include "ODRingDistribution.h"
#include "ODDistributionBox.h"
#include "ODSphereDistribution.h"
#include "ODTDDistribution.h"
#include "Components/Border.h"
#include "Subsystems/EditorActorSubsystem.h"

void UODToolWindow::NativePreConstruct()
{
	Super::NativePreConstruct();
	
#if WITH_EDITOR
	
	if((PresetObject = Cast<UODPresetObject>(NewObject<UODPresetObject>(this, TEXT("PresetObject")))))
	{
		PresetObject->SetupPresets();
		PresetDetails->SetObject(PresetObject);
	}
	
	if((PropDistributionBase = Cast<UODDistributionBase>(NewObject<UODDistributionBox>(this, TEXT("ODDistributionBase")))))
	{
		PropDistributionBase->OnDistributionTypeChangedSignature.BindUObject(this, &UODToolWindow::OnDistributionTypeChanged);
		PropDistributionBase->OnAfterODRegenerated.BindUObject(this, &UODToolWindow::OnAfterODRegenerated);
		PropDistributionBase->OnFinishConditionChangeSignature.BindUObject(this, &UODToolWindow::OnFinishConditionChanged);
		PropDistributionDetails->SetObject(PropDistributionBase);
	}

	if(IsValid(FinishBtn)){FinishBtn->SetVisibility(ESlateVisibility::Collapsed);}
	
	//Tooltips

	if(IsValid(FinishBtn)){FinishBtn->SetToolTipText(FText::FromName(TEXT("Finish the distribution and perform the operation according to the selected finish type.")));}

	if(IsValid(AddNewPresetBtn)){AddNewPresetBtn->SetToolTipText(FText::FromName(TEXT("Add an empty preset and give it a new name.")));}
	if(IsValid(RenamePresetBtn)){RenamePresetBtn->SetToolTipText(FText::FromName(TEXT("Rename the active preset.")));}
	if(IsValid(RemovePresetBtn)){RemovePresetBtn->SetToolTipText(FText::FromName(TEXT("Remove the active preset.")));}
	if(IsValid(AddSelectedAssetsBtn)){AddSelectedAssetsBtn->SetToolTipText(FText::FromName(TEXT("Create a preset from the selected static meshes in the Content Browser.")));}
	if(IsValid(SavePresetBtn)){SavePresetBtn->SetToolTipText(FText::FromName(TEXT("Save the active preset.")));}
	if(IsValid(SaveAsNewPresetBtn)){SaveAsNewPresetBtn->SetToolTipText(FText::FromName(TEXT("Create a new preset using the existing object distribution data.")));}

#endif // WITH_EDITOR
}

void UODToolWindow::NativeConstruct()
{
	Super::NativeConstruct();
#if WITH_EDITOR
	
	if (!AddNewPresetBtn->OnClicked.IsBound())
	{
		AddNewPresetBtn->OnClicked.AddDynamic(this, &UODToolWindow::AddNewPresetBtnPressed);
	}
	
	if (!AddSelectedAssetsBtn->OnClicked.IsBound())
	{
		AddSelectedAssetsBtn->OnClicked.AddDynamic(this, &UODToolWindow::AddSelectedAssetsBtnPressed);
	}
	
	if (!RenamePresetBtn->OnClicked.IsBound())
	{
		RenamePresetBtn->OnClicked.AddDynamic(this, &UODToolWindow::RenamePresetBtnPressed);
	}

	if (!SavePresetBtn->OnClicked.IsBound())
	{
		SavePresetBtn->OnClicked.AddDynamic(this, &UODToolWindow::SavePresetBtnPressed);
	}

	if (!SaveAsNewPresetBtn->OnClicked.IsBound())
	{
		SaveAsNewPresetBtn->OnClicked.AddDynamic(this, &UODToolWindow::SaveAsNewPresetBtnPressed);
	}

	if (!RemovePresetBtn->OnClicked.IsBound())
	{
		RemovePresetBtn->OnClicked.AddDynamic(this, &UODToolWindow::RemovePresetBtnPressed);
	}
	
	if (!NewPresetText->OnTextCommitted.IsBound())
	{
		NewPresetText->OnTextCommitted.AddDynamic(this,&UODToolWindow::OnNewPresetTextCommitted);
	}

	if (!AddAssetsText->OnTextCommitted.IsBound())
	{
		AddAssetsText->OnTextCommitted.AddDynamic(this,&UODToolWindow::OnAddAssetsTextCommitted);
	}	

	if (!SaveAsText->OnTextCommitted.IsBound())
	{
		SaveAsText->OnTextCommitted.AddDynamic(this,&UODToolWindow::OnSaveAsTextCommitted);
	}

	if (!RenamePresetText->OnTextCommitted.IsBound())
	{
		RenamePresetText->OnTextCommitted.AddDynamic(this,&UODToolWindow::OnRenamePresetTextCommitted);
	}

	if (!FinishBtn->OnClicked.IsBound())
	{
		FinishBtn->OnClicked.AddDynamic(this, &UODToolWindow::FinishBtnPressed);
	}
	if(AssetDropBorder){AssetDropBorder->GetOnAssetDroppedSignature()->BindUObject(this,&UODToolWindow::OnAssetDropped);}

	if(PresetObject){PresetObject->OnPresetCategoryHidden.BindUObject(this,&UODToolWindow::OnPresetCategoryHidden);}
	
	if(PropDistributionBase){PropDistributionBase->Setup(this);}
		

	SpawnSpawnCenter();

	if(GEditor)
	{
		GEditor->OnLevelActorDeleted().AddUObject(this, &UODToolWindow::HandleOnLevelActorDeletedNative);
	}
	
	if (const auto ToolSubsystem = GEditor->GetEditorSubsystem<UODToolSubsystem>())
	{
		ToolSubsystem->LoadActivePreset();
		
		if(PresetSettingsBorder){PresetSettingsBorder->SetVisibility(ToolSubsystem->bInMixerMode ? ESlateVisibility::Collapsed :ESlateVisibility::Visible);}
		
		ToolSubsystem->OnMixerModeChanged.BindUObject(this,&UODToolWindow::OnMixerModeChanged);
	}

#endif // WITH_EDITOR
}


void UODToolWindow::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	
	if(SpawnCenterRef)
	{
		//Position Trace
		if(!bTraceForVelocity)
		{
			if(!SpawnCenterRef->GetActorLocation().Equals(LastSpawnCenterLocation))
			{
				bTraceForVelocity = true;
			}
		}
		if(bTraceForVelocity)
		{
			const FVector VelocityChange =  SpawnCenterRef->GetActorLocation() - LastSpawnCenterLocation;
			SpawnCenterVelocity += VelocityChange;

			//Finish 
			if(VelocityChange.SquaredLength() < 1)
			{
				if(PropDistributionBase){PropDistributionBase->AddSpawnCenterMotionDifferences(SpawnCenterVelocity);}
				bTraceForVelocity = false;
				SpawnCenterVelocity = FVector::ZeroVector;
				
				if(SpawnCenterRef)
				{
					SpawnCenterRef->RegenerateBoundsDrawData();
				}
			}
			LastSpawnCenterLocation = SpawnCenterRef->GetActorLocation();
		}

		//Rotation Trace
		if(!bTraceForRotationDiff)
		{
			if(!SpawnCenterRef->GetActorRotation().Equals(LastSpawnCenterRotation))
			{

				bTraceForRotationDiff = true;
			}
		}
		if(bTraceForRotationDiff)
		{
			const FRotator RotChange =  SpawnCenterRef->GetActorRotation() - LastSpawnCenterRotation;
			SpawnCenterRotation += RotChange;

			//Finish 
			if(RotChange.IsNearlyZero(1))
			{
				if(PropDistributionBase){PropDistributionBase->ReRotateObjectsOnSpawnCenter();}
				bTraceForRotationDiff = false;
				SpawnCenterRotation = FRotator::ZeroRotator;
				
				if(SpawnCenterRef)
				{
					SpawnCenterRef->RegenerateBoundsDrawData();
				}
			}
			LastSpawnCenterRotation = SpawnCenterRef->GetActorRotation();
		}
	}
}

void UODToolWindow::OnDistributionTypeChanged(EObjectDistributionType NewDistributionType)
{
	if(!PropDistributionBase){return;}
	
	if(PropDistributionBase->OnDistributionTypeChangedSignature.IsBound())
	{
		PropDistributionBase->OnDistributionTypeChangedSignature.Unbind();
	}
	if(PropDistributionBase->OnAfterODRegenerated.IsBound())
	{
		PropDistributionBase->OnAfterODRegenerated.Unbind();
	}
	
	PropDistributionBase->OnAfterODRegenerated.Unbind();
	
	if((NewDistributionType == EObjectDistributionType::Box))
	{
		const FName Name = MakeUniqueObjectName(GetWorld(), UODDistributionBox::StaticClass(), FName(TEXT("ODDistributionBox")));
		if(const auto LocalDistBase = NewObject<UODDistributionBox>(this, Name))
		{
			CopyPermanentDistributionSettings(LocalDistBase);

			PropDistributionBase = LocalDistBase;
		}
	}
	if(NewDistributionType == EObjectDistributionType::Ring)
	{
		const FName Name = MakeUniqueObjectName(GetWorld(), UODRingDistribution::StaticClass(), FName(TEXT("ODRingDistribution")));
		if(const auto LocalDistBase = NewObject<UODRingDistribution>(this, Name))
		{
			CopyPermanentDistributionSettings(LocalDistBase);

			PropDistributionBase = LocalDistBase;
		}
	}
	if(NewDistributionType == EObjectDistributionType::Disk)
	{
		const FName Name = MakeUniqueObjectName(GetWorld(), UODDiskDistribution::StaticClass(), FName(TEXT("ODDiskDistribution")));
		if(const auto LocalDistBase = NewObject<UODDiskDistribution>(this, Name))
		{
			CopyPermanentDistributionSettings(LocalDistBase);

			PropDistributionBase = LocalDistBase;
		}
	}
	else if(NewDistributionType == EObjectDistributionType::Sphere)
	{
		const FName Name = MakeUniqueObjectName(GetWorld(), UODSphereDistribution::StaticClass(), FName(TEXT("ODSphereDistribution")));
		if(const auto LocalDistBase = NewObject<UODSphereDistribution>(this, Name))
		{
			CopyPermanentDistributionSettings(LocalDistBase);

			PropDistributionBase = LocalDistBase;
		}
	}
	else if(NewDistributionType == EObjectDistributionType::Helix)
	{
		const FName Name = MakeUniqueObjectName(GetWorld(), UODHelixDistribution::StaticClass(), FName(TEXT("ODHelixDistribution")));
		if(const auto LocalDistBase = NewObject<UODHelixDistribution>(this, Name))
		{
			CopyPermanentDistributionSettings(LocalDistBase);

			PropDistributionBase = LocalDistBase;
		}
	}
	else if(NewDistributionType == EObjectDistributionType::Line)
	{
		const FName Name = MakeUniqueObjectName(GetWorld(), UODDistributionLine::StaticClass(), FName(TEXT("ODLineDistribution")));
		if(const auto LocalDistBase = NewObject<UODDistributionLine>(this, Name))
		{
			CopyPermanentDistributionSettings(LocalDistBase);

			PropDistributionBase = LocalDistBase;
		}
	}
	else if(NewDistributionType == EObjectDistributionType::Grid)
	{
		const FName Name = MakeUniqueObjectName(GetWorld(), UODTDDistribution::StaticClass(), FName(TEXT("ODGridDistribution")));
		if(const auto LocalDistBase = NewObject<UODTDDistribution>(this, Name))
		{
			CopyPermanentDistributionSettings(LocalDistBase);

			PropDistributionBase = LocalDistBase;
		}
	}
	PropDistributionDetails->SetObject(PropDistributionBase);

	PropDistributionBase->Setup(this);

	PropDistributionBase->OnAfterODRegenerated.BindUObject(this, &UODToolWindow::OnAfterODRegenerated);
	PropDistributionBase->OnFinishConditionChangeSignature.BindUObject(this, &UODToolWindow::OnFinishConditionChanged);
	
	PropDistributionBase->ReDesignObjects();
	
	PropDistributionBase->OnDistributionTypeChangedSignature.BindUObject(this, &UODToolWindow::OnDistributionTypeChanged);
}


void UODToolWindow::SpawnSpawnCenter()
{
	const auto EditorWorld = GEditor->GetEditorWorldContext().World();
	if(SpawnCenterRef && !EditorWorld){return;}

	const FVector SpawnLocation = FindSpawnLocationForSpawnCenter();
	
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.bNoFail = true;
	SpawnCenterRef = EditorWorld->SpawnActor<AODSpawnCenter>(AODSpawnCenter::StaticClass(),SpawnLocation,FRotator::ZeroRotator,SpawnParams);
	if(SpawnCenterRef)
	{
		LastSpawnCenterLocation = SpawnCenterRef->GetActorLocation(); //Last SpawnCenter Ref
		if(const auto EditorActorSubsystem = GEditor->GetEditorSubsystem<UEditorActorSubsystem>())
		{
			EditorActorSubsystem->SelectNothing();
			EditorActorSubsystem->SetActorSelectionState(SpawnCenterRef,true);
		}
	}
}

AActor* UODToolWindow::GetSpawnCenterRef() const
{
	return SpawnCenterRef;
}

FVector UODToolWindow::FindSpawnLocationForSpawnCenter()
{
	if(!GCurrentLevelEditingViewportClient || !GEditor){return FVector::ZeroVector;}
	
	const FVector StartLoc = GCurrentLevelEditingViewportClient->GetViewLocation();
	FVector EndLoc = StartLoc + GCurrentLevelEditingViewportClient->GetViewRotation().Vector() * 500.0f;

	auto EditorWorld = GEditor->GetEditorWorldContext().World();
	if(!EditorWorld){return FVector::ZeroVector;}

	static const FName lineTraceSingleName(TEXT("LevelEditorLineTrace"));
	FCollisionQueryParams collisionParams(lineTraceSingleName);
	collisionParams.bTraceComplex = false;
	collisionParams.bReturnPhysicalMaterial = true;
	//collisionParams.bReturnFaceIndex = !UPhysicsSettings::Get()->bSuppressFaceRemapTable;
	FCollisionObjectQueryParams objectParams = FCollisionObjectQueryParams(ECC_WorldStatic);
	
	objectParams.AddObjectTypesToQuery(ECC_Visibility);
	objectParams.AddObjectTypesToQuery(ECC_WorldStatic);

	FHitResult HitResult;
	EditorWorld->LineTraceSingleByObjectType(HitResult, StartLoc, EndLoc, objectParams, collisionParams);

	if(HitResult.bBlockingHit)
	{
		EndLoc.Z = HitResult.Location.Z;
		return EndLoc;
	}
	return EndLoc;
}

void UODToolWindow::GetSpawnCenterToCameraView() const
{
	SpawnCenterRef->SetActorLocation(FindSpawnLocationForSpawnCenter());
}

void UODToolWindow::AddNewPresetBtnPressed()
{
	AddNewPresetBtn->SetVisibility(ESlateVisibility::Collapsed);
	NewPresetText->SetVisibility(ESlateVisibility::Visible);
	NewPresetText->SetKeyboardFocus();
}

void UODToolWindow::AddSelectedAssetsBtnPressed()
{
	const auto LocalSelectedAssets = UEditorUtilityLibrary::GetSelectedAssets();

	auto RunWarningMessage =  [] () -> void
	{
		ODDebug::ShowMsgDialog(EAppMsgType::Ok,FString(TEXT("Please select a valid static mesh asset first.")));
	};

	if(LocalSelectedAssets.IsEmpty())
	{
		RunWarningMessage();
		return;
	}
	
	bool bIsContainsAnySM = false;
	for(const auto FoundSM : LocalSelectedAssets)
	{
		if(FoundSM->IsA(UStaticMesh::StaticClass()))
		{
			bIsContainsAnySM = true;
			break;
		}
	}
	if(!bIsContainsAnySM)
	{
		RunWarningMessage();
		return;
	}
	
	AddSelectedAssetsBtn->SetVisibility(ESlateVisibility::Collapsed);
	AddAssetsText->SetVisibility(ESlateVisibility::Visible);
	AddAssetsText->SetKeyboardFocus();
}

void UODToolWindow::OnAssetDropped(TArrayView<FAssetData> DroppedAssets)
{
	if(AddSelectedAssetsBtn){AddSelectedAssetsBtn->SetVisibility(ESlateVisibility::Collapsed);}
	if(AddAssetsText){AddAssetsText->SetVisibility(ESlateVisibility::Visible);}
	if(AddAssetsText){AddAssetsText->SetKeyboardFocus();}
}

void UODToolWindow::OnAddAssetsTextCommitted(const FText& InText, ETextCommit::Type InCommitMethod)
{
	if(InCommitMethod == ETextCommit::Type::Default){return;}
	
	if(InCommitMethod == ETextCommit::Type::OnEnter)
	{
		if (const auto ToolSubsystem = GEditor->GetEditorSubsystem<UODToolSubsystem>())
		{
			if(!InText.IsEmpty() && !InText.ToString().ToLower().Equals(TEXT("no preset")) && !InText.ToString().ToLower().Equals(TEXT("nopreset")) && !ToolSubsystem->IsPresetAvailable(InText.ToString()))
			{
				//Create if at least one of the selected assets is a static mesh.
				if(ToolSubsystem->CreateNewPresetFromSelectedAssets(InText.ToString()))
				{
					ToolSubsystem->SetActivePreset(InText.ToString());

					if(PresetObject)
					{
						PresetObject->SetupPresets();
					}
					ToolSubsystem->LoadActivePreset();
				
					ODDebug::ShowNotifyInfo(FString(TEXT("Preset Created Succesfully")));
				}
			}
		}
	}
	AddAssetsText->SetText(FText());
	AddSelectedAssetsBtn->SetVisibility(ESlateVisibility::Visible);
	AddAssetsText->SetVisibility(ESlateVisibility::Collapsed);
}



void UODToolWindow::RenamePresetBtnPressed()
{
	if (const auto ToolSubsystem = GEditor->GetEditorSubsystem<UODToolSubsystem>())
	{
		if(ToolSubsystem->GetLastSelectedPreset().IsNone())
		{
			ODDebug::ShowMsgDialog(EAppMsgType::Ok,TEXT("Please select a valid preset first"),true);
		}
		else
		{
			RenamePresetBtn->SetVisibility(ESlateVisibility::Collapsed);
			RenamePresetText->SetVisibility(ESlateVisibility::Visible);
			RenamePresetText->SetKeyboardFocus();
		}
	}
}

void UODToolWindow::SavePresetBtnPressed()
{
	if (const auto ToolSubsystem = GEditor->GetEditorSubsystem<UODToolSubsystem>())
	{
		if(ToolSubsystem->GetLastSelectedPreset().IsNone())
		{
			ODDebug::ShowMsgDialog(EAppMsgType::Ok,TEXT("Please select a valid preset first"),true);
		}
		else
		{
			if(ToolSubsystem->SaveCurrentPreset())
			{
				ODDebug::ShowNotifyInfo(FString(TEXT("Preset Saved Succesfully")));
			}
		}
	}
}

void UODToolWindow::SaveAsNewPresetBtnPressed()
{
	SaveAsNewPresetBtn->SetVisibility(ESlateVisibility::Collapsed);
	SaveAsText->SetVisibility(ESlateVisibility::Visible);
	SaveAsText->SetKeyboardFocus();
}



void UODToolWindow::RemovePresetBtnPressed()
{
	if (const auto ToolSubsystem = GEditor->GetEditorSubsystem<UODToolSubsystem>())
	{
		if(ToolSubsystem->GetLastSelectedPreset().IsNone())
		{
			ODDebug::ShowMsgDialog(EAppMsgType::Ok,TEXT("Please select a valid preset first"),true);
		}
		else
		{
			ToolSubsystem->RemoveCurrentPreset();

			ToolSubsystem->SetLastPresetAsActivePreset(); //V1.3
			
			if(PresetObject)
			{
				PresetObject->SetupPresets();
			}

			ToolSubsystem->LoadActivePreset();

			ODDebug::ShowNotifyInfo(FString(TEXT("Preset removed Succesfully")));
		}
	}
}


void UODToolWindow::OnNewPresetTextCommitted(const FText& InText, ETextCommit::Type InCommitMethod)
{
	if(InCommitMethod == ETextCommit::Type::OnEnter)
	{
		if (const auto ToolSubsystem = GEditor->GetEditorSubsystem<UODToolSubsystem>())
		{
			if(!InText.IsEmpty() && !InText.ToString().ToLower().Equals(TEXT("no preset")) && !InText.ToString().ToLower().Equals(TEXT("nopreset")) && !ToolSubsystem->IsPresetAvailable(InText.ToString()))
			{
				ToolSubsystem->AddNewPreset(InText.ToString());

				//If current preset is none then load newly created one
				if(ToolSubsystem->GetLastSelectedPreset().IsNone())
				{
					ToolSubsystem->SetActivePreset(InText.ToString());

					if (PresetObject)
					{
						PresetObject->SetupPresets();
					}

					ToolSubsystem->LoadActivePreset();
				}
				else
				{
					if (PresetObject)
					{
						PresetObject->SetupPresets();
					}
				}
				
				ODDebug::ShowNotifyInfo(FString(TEXT("Preset Added Succesfully")));
			}
		}
	}
	
	
	NewPresetText->SetText(FText());
	AddNewPresetBtn->SetVisibility(ESlateVisibility::Visible);
	NewPresetText->SetVisibility(ESlateVisibility::Collapsed);
}

void UODToolWindow::OnRenamePresetTextCommitted(const FText& InText, ETextCommit::Type InCommitMethod)
{
	if(InCommitMethod == ETextCommit::Type::OnEnter)
	{
		if (const auto ToolSubsystem = GEditor->GetEditorSubsystem<UODToolSubsystem>())
		{
			if(!InText.IsEmpty() && !InText.ToString().ToLower().Equals(TEXT("no preset")) && !InText.ToString().ToLower().Equals(TEXT("nopreset")) && !ToolSubsystem->IsPresetAvailable(InText.ToString()))
			{
				ToolSubsystem->RenameCurrentPreset(InText.ToString());
				if (PresetObject)
				{
					PresetObject->SetupPresets();
				}
			
				ODDebug::ShowNotifyInfo(FString(TEXT("Preset Renamed Succesfully")));
			}
		}
	}
	RenamePresetText->SetText(FText());
	RenamePresetBtn->SetVisibility(ESlateVisibility::Visible);
	RenamePresetText->SetVisibility(ESlateVisibility::Collapsed);
}

void UODToolWindow::OnSaveAsTextCommitted(const FText& InText, ETextCommit::Type InCommitMethod)
{
	if(InCommitMethod == ETextCommit::Type::OnEnter)
	{
		if (const auto ToolSubsystem = GEditor->GetEditorSubsystem<UODToolSubsystem>())
		{
			if(!InText.IsEmpty() && !InText.ToString().ToLower().Equals(TEXT("no preset")) && !InText.ToString().ToLower().Equals(TEXT("nopreset")) && !ToolSubsystem->IsPresetAvailable(InText.ToString()))
			{
				ToolSubsystem->AddNewPreset(InText.ToString());
				ToolSubsystem->SetActivePreset(InText.ToString());

				ToolSubsystem->SaveCurrentPreset();
				
				if(PresetObject)
				{
					PresetObject->SetupPresets();
				}
			
				ODDebug::ShowNotifyInfo(FString(TEXT("Preset Added Succesfully")));
			}
		}
	}

	SaveAsText->SetText(FText());
	SaveAsNewPresetBtn->SetVisibility(ESlateVisibility::Visible);
	SaveAsText->SetVisibility(ESlateVisibility::Collapsed);
}

void UODToolWindow::FinishBtnPressed()
{
	if(PropDistributionBase)
	{
		PropDistributionBase->OnFinishDistributionPressed();
	}
}

void UODToolWindow::OnFinishConditionChanged(bool InCanFinish)
{
	if(IsValid(FinishBtn)){FinishBtn->SetVisibility(InCanFinish ? ESlateVisibility::Visible :  ESlateVisibility::Collapsed);}
}

void UODToolWindow::OnAfterODRegenerated() const
{
	if(SpawnCenterRef)
	{
		SpawnCenterRef->RegenerateBoundsDrawData();
	}
}

void UODToolWindow::OnMixerModeChanged(bool InIsInMixerMode)
{
	if(!PropDistributionBase){return;}
	if(PresetSettingsBorder){PresetSettingsBorder->SetVisibility(InIsInMixerMode ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);}
	const auto CurrentDistType = PropDistributionBase->DistributionType;
	
	OnDistributionTypeChanged(CurrentDistType);

	TitleBorder->SetBrushColor(InIsInMixerMode ? 
	FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("1E5428FF")))
		:
	FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("242424FF"))));
	
		
}

void UODToolWindow::OnPresetCategoryHidden(bool InbIsItOpen)
{
	if (const auto ToolSubsystem = GEditor->GetEditorSubsystem<UODToolSubsystem>())
	{
		if(!ToolSubsystem->bInMixerMode)
		{
			PresetSettingsBorder->SetVisibility(InbIsItOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		}
	}
}


void UODToolWindow::HandleOnLevelActorDeletedNative(AActor* InActor)
{
	if(!bIsToolDestroying && InActor ==SpawnCenterRef)
	{
		SpawnCenterRef = nullptr;
		SpawnSpawnCenter();
	}
	
	if(PropDistributionBase)
	{
		PropDistributionBase->LevelActorDeleted(InActor);
	}
}

void UODToolWindow::CopyPermanentDistributionSettings(UODDistributionBase* InCreatedDistObject) const
{
	if(PropDistributionBase && InCreatedDistObject)
	{
		//Spawn Data
		InCreatedDistObject->bTestForCollider = PropDistributionBase->bTestForCollider;
		InCreatedDistObject->MaxCollisionTest = PropDistributionBase->MaxCollisionTest;
		InCreatedDistObject->bOverrideDistributionData = PropDistributionBase->bOverrideDistributionData;
		InCreatedDistObject->GlobalDistributionData = PropDistributionBase->GlobalDistributionData;
		InCreatedDistObject->bSpawnDensity = PropDistributionBase->bSpawnDensity;
		InCreatedDistObject->SpawnDensity = PropDistributionBase->SpawnDensity;

		//Simulation Data
		InCreatedDistObject->bSimulatePhysics = PropDistributionBase->bSimulatePhysics;
		InCreatedDistObject->bDisableSimAfterFinish = PropDistributionBase->bDisableSimAfterFinish;
		InCreatedDistObject->KillZ = PropDistributionBase->KillZ;
	}
}

void UODToolWindow::NativeDestruct()
{
	if (const auto ToolSubsystem = GEditor->GetEditorSubsystem<UODToolSubsystem>())
	{
		ToolSubsystem->OnMixerModeChanged.Unbind();
		ToolSubsystem->ToolWindowClosed();
	}
	
	//if (TogglePresetSettingsBtn) { TogglePresetSettingsBtn->OnClicked.RemoveAll(this); }
	if (AddNewPresetBtn) { AddNewPresetBtn->OnClicked.RemoveAll(this); }
	if (AddSelectedAssetsBtn) { AddSelectedAssetsBtn->OnClicked.RemoveAll(this); }
	if (RenamePresetBtn) { RenamePresetBtn->OnClicked.RemoveAll(this); }
	if (SavePresetBtn) { SavePresetBtn->OnClicked.RemoveAll(this); }
	if (SaveAsNewPresetBtn) { SaveAsNewPresetBtn->OnClicked.RemoveAll(this); }
	if (RemovePresetBtn) { RemovePresetBtn->OnClicked.RemoveAll(this); }
	if (FinishBtn) { FinishBtn->OnClicked.RemoveAll(this); }
	if (NewPresetText) { NewPresetText->OnTextCommitted.RemoveAll(this); }
	if (AddAssetsText) { AddAssetsText->OnTextCommitted.RemoveAll(this); }
	if (SaveAsText) { SaveAsText->OnTextCommitted.RemoveAll(this); }
	if (RenamePresetText) { RenamePresetText->OnTextCommitted.RemoveAll(this); }

	if(IsValid(PropDistributionBase))
	{
		if(PropDistributionBase->OnDistributionTypeChangedSignature.IsBound())
		{
			PropDistributionBase->OnDistributionTypeChangedSignature.Unbind();
		}
		if(PropDistributionBase->OnAfterODRegenerated.IsBound())
		{
			PropDistributionBase->OnAfterODRegenerated.Unbind();
		}
		if(PropDistributionBase->OnFinishConditionChangeSignature.IsBound())
		{
			PropDistributionBase->OnFinishConditionChangeSignature.Unbind();
		}
	}

	if(PresetObject){PresetObject->OnPresetCategoryHidden.Unbind();}
	
	bIsToolDestroying = true;
	
	if(SpawnCenterRef)
	{
		SpawnCenterRef->Destroy();
	}
	const auto EditorActorSubsystem = GEditor->GetEditorSubsystem<UEditorActorSubsystem>();
	EditorActorSubsystem->SelectNothing();
	
#if WITH_EDITOR
	if (!GEditor) return;
	GEditor->OnLevelActorDeleted().RemoveAll(this);
#endif
	
	Super::NativeDestruct();
}

