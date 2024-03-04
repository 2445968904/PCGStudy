// Copyright 2023 Leartes Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "ODMeshData.h"
#include "ODToolSubsystem.generated.h"

class UODToolSettings;
struct FODPresetData;
struct FPresetMixerMapData;
class UDataTable;
class AStaticMeshActor;
struct FDistObjectData;

DECLARE_DELEGATE(FOnPresetLoaded);
DECLARE_DELEGATE_OneParam(FOnMixerModeChanged, bool)
DECLARE_DELEGATE_TwoParams(FOnAMixerPresetCheckStatusChanged, bool,FName)

UCLASS()
class OBJECTDISTRIBUTION_API UODToolSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()

	
	UPROPERTY()
	TObjectPtr<UODToolSettings> ODToolSettings;
	
	
	UPROPERTY()
	TObjectPtr<UDataTable> PresetData;
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	void LoadDataAssets();
	
	virtual void Deinitialize() override;

	void ToolWindowClosed();

#pragma  region Preset
	
private:
	FName LastSelectedPreset;

public:
	TArray<FString> GetPresetNames() const;
	
	FName GetLastSelectedPreset() const {return LastSelectedPreset;}

	TObjectPtr<UODToolSettings> GetODToolSettings() const {return ODToolSettings;}
	
	bool IsPresetAvailable(const FString& InPResetName) const;
	
	FODPresetData* GetPresetData(const FString& InPResetName) const;
	
	void AddNewPreset(const FString& InPResetName) const;

	bool CreateNewPresetFromSelectedAssets(const FString& InPResetName) const;
	
	void SetActivePreset(const FString& InPResetName);
	
	void SetLastPresetAsActivePreset();
	
	void RenameCurrentPreset(const FString& InNewName);

	bool SaveCurrentPreset() const;

	void RemoveCurrentPreset();

	void LoadActivePreset();

	FOnPresetLoaded OnPresetLoaded;
private:
	void SavePresetDataToDisk() const;

	void RefreshThePresetDataTableForInvalidData() const;

#pragma  endregion Presets

#pragma  region PresetMixer

private:
	//This data updated automatically when a preset checked or unchecked
	UPROPERTY()
	TArray<FPresetMixerMapData> PresetMixerMapData;
	
	//This data updated automatically when a preset checked, unchecked or a custom data modified
	UPROPERTY()
	TArray<FPresetMixData> PresetMixerData; 

public:
	void UpdateMixerCheckListAndResetAllCheckStatus();

	TArray<FPresetMixerMapData>& GetPresetMixerMapData(){return PresetMixerMapData;}
	TArray<FPresetMixData>& GetPresetMixerData(){return PresetMixerData;}

	bool bInMixerMode = false;
	
	void ToggleMixerMode();

	bool IsInMixerMode() const {return bInMixerMode;}
	
	FOnMixerModeChanged OnMixerModeChanged;
	FOnAMixerPresetCheckStatusChanged OnAMixerPresetCheckStatusChanged;

public:
	void MixerPresetChecked(const FPresetMixerMapData& InMixerMapData);
	void RegeneratePresetMixerDataFromScratch();
	void RegeneratePresetMixerWithCustomData(bool bIsOverridingPresetProperties,TArray<FPresetMixData>& InCustomMixData);
	
#pragma  endregion PresetMixer
	
#pragma region ObjectDistribution
public:
	
	inline  static int32 FolderNumberContainer = 1;
	
	FORCEINLINE static int32 GetUniqueFolderNumber() {return FolderNumberContainer++;}
	
	UPROPERTY()
	TArray<FDistObjectData> DistObjectData;

	UPROPERTY()
	TArray<TObjectPtr<AStaticMeshActor>> CreatedDistObjects;

	UPROPERTY()
	TMap<int32,FDistObjectData> ObjectDataMap;

	UPROPERTY()
	TArray<FDistObjectData> ObjectDistributionData;

	void DestroyKillZActors(const TArray<FName>& InActorNames);

	void CreateInstanceFromDistribution(const EODMeshConversionType& InTargetType);

private:
	static FBox GetAllActorsTransportPointWithExtents(const TArray<AStaticMeshActor*>& InActors);
	
#pragma endregion ObjectDistribution
	
};
