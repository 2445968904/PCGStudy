// Copyright 2023 Leartes Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/ODMeshData.h"
#include "Tickable.h"
#include "ODDistributionBase.generated.h"

DECLARE_DELEGATE_OneParam(FOnODDistributionTypeChangedSignature, EObjectDistributionType);
DECLARE_DELEGATE(FOnAfterODRegenerated);
DECLARE_DELEGATE_OneParam(FOnFinishConditionChangeSignature,bool);

class UODToolSubsystem;
class UODToolWindow;
class AStaticMeshActor;
class FReply;

UCLASS()
class OBJECTDISTRIBUTION_API UODDistributionBase : public UObject, public FTickableGameObject
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UODToolWindow> ToolWindow;
	
	UPROPERTY()
	TObjectPtr<UODToolSubsystem> ToolSubsystem;

	FDistObjectData* FindDataFromMap(const FName& InIndexName) const;

	UPROPERTY()
	TArray<FVector> InitialRelativeLocations;

	bool bAreAssetsSimulated;

	UPROPERTY()
	bool bIsInMixerMode = false;
	
	int32 CustomPresetDataNum;
	
protected:
	TObjectPtr<UODToolSubsystem> GetToolSubsystem() const {return ToolSubsystem;}

public:
	FOnODDistributionTypeChangedSignature OnDistributionTypeChangedSignature;
	FOnAfterODRegenerated OnAfterODRegenerated;
	FOnFinishConditionChangeSignature OnFinishConditionChangeSignature;
	
	void Setup(UODToolWindow* InToolWindow);

	virtual void LoadDistData();

	virtual void BeginDestroy() override;
	
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	UPROPERTY(EditAnywhere,DisplayName="Override Preset Properties", Category = "Creation",meta=(NoResetToDefault))
	bool bOverridePresetProperties = false;
	
	UPROPERTY(EditAnywhere,Category = "Creation");
	TArray<FDistObjectData> ObjectDistributionData;

	UPROPERTY(EditAnywhere, meta = (GetOptions = "GetEditableMixerPreset",NoResetToDefault),DisplayName="Override Presets",Category = "Creation")
	FString EditablePresetList = FString(TEXT("Select A Preset"));

	UPROPERTY(EditAnywhere,Category = "Creation"); //For Mixer
	TArray<FPresetMixData> CustomPresetData;

	UPROPERTY(EditAnywhere, meta=(InlineEditConditionToggle), Category = "Creation")
	bool bTestForCollider = false;
	
	UPROPERTY(EditAnywhere, meta=(EditCondition="bTestForCollider", ClampMin = 1,ClampMax = 250), Category = "Creation")
	int32 MaxCollisionTest = 100;

	UPROPERTY(EditAnywhere, meta=(EditCondition="false"), Category = "Creation")
	int32 CollidingObjects = 0;

	UPROPERTY(EditAnywhere, meta=(EditCondition="false"), Category = "Creation")
	int32 TotalSpawnCount = 0;
	
	UPROPERTY(EditAnywhere,DisplayName="Override Object Properties" , Category = "Creation|Global Creation")
	bool bOverrideDistributionData = false;

	UPROPERTY(EditAnywhere, meta=(EditCondition="bOverrideDistributionData",EditConditionHides),Category = "Creation|Global Creation")
	FDistObjectPropertyData GlobalDistributionData;

	UPROPERTY(EditAnywhere, meta=(InlineEditConditionToggle), Category = "Creation|Global Creation")
	bool bSpawnDensity = false;
	
	UPROPERTY(EditAnywhere, Category = "Creation|Global Creation", meta = (EditCondition="bSpawnDensity",UIMin = "-1.0", UIMax = "1.0", SliderExponent = 0.5))
	float SpawnDensity  = 0.01;
	
	UPROPERTY(EditAnywhere,Category = "Distribution" , meta=(NoResetToDefault,DisplayPriority=1))
	EObjectDistributionType DistributionType = EObjectDistributionType::Box;

	UPROPERTY(EditAnywhere,Category = "Simulation" , meta=(NoResetToDefault,DisplayPriority=1))
	bool  bSimulatePhysics =true;

	UPROPERTY(EditAnywhere,Category = "Simulation" ,DisplayName="Disable Simulate Pyhsics After Finishing", meta=(EditCondition = "bSimulatePhysics",NoResetToDefault,DisplayPriority=1))
	bool  bDisableSimAfterFinish = true;

	UPROPERTY(EditAnywhere,Category = "Simulation" , meta=(NoResetToDefault,DisplayPriority=1,ToolTip = "Any actor falling below this height gets destroyed both simulation and editor world"),DisplayName = "Simulation Kill Z")
	float KillZ = -500.0f;

	UPROPERTY(EditAnywhere,Category = "SpawnCenter" ,DisplayName="Draw Spawn Bounds")
	bool  bDrawSpawnBounds = true;
	
	UPROPERTY(EditAnywhere,Category = "SpawnCenter" , meta=(EditCondition="bDrawSpawnBounds",EditConditionHides,HideAlphaChannel))
	FLinearColor BoundsColor = FLinearColor( 0.339158f, 0.088386f, 0.953125f);

	UPROPERTY(EditAnywhere,Category = "Settings" , meta=(DisplayPriority = 1 ))
	EODDistributionFinishType FinishType = EODDistributionFinishType::Keep;

	UPROPERTY(EditAnywhere,Category = "Settings" , meta=(DisplayPriority = 2 , EditCondition = "FinishType == EODDistributionFinishType::Batch",EditConditionHides))
	EODMeshConversionType TargetType = EODMeshConversionType::Ism;

public:
#if WITH_EDITOR

	//Creation
	FReply OnCreateDistributionPressed();
	FReply OnDeleteDistributionPressed();

	//Distribution
	FReply OnShuffleDistributionPressed();
	FReply OnSelectObjectsPressed();
	FReply OnFinishDistributionPressed();

	//Simulation
	FReply OnStartSimulationPressed();
	FReply OnStopSimulationPressed();
	FReply OnPauseSimulationPressed();
	FReply OnResumeSimulationPressed();

	//SpawnCenter
	FReply SelectSpawnCenterPressed();
	FReply MoveSpawnCenterToWorldOriginPressed();
	FReply MoveSpawnCenterToCameraPressed();

	//SpawnCenter
	FReply ResetParametersPressed();
#endif

private:

	void CreatePresetModeObjects(TArray<AStaticMeshActor*>& InSpawnedActors);

	void CreateMixerModeObjects(TArray<AStaticMeshActor*>& InSpawnedActors);
	
	void DestroyObjects() const;

	bool  CheckForSpotSuitability(TObjectPtr<AStaticMeshActor> InSmActor) const;

	void CalculateTotalSpawnCount();
	
	int32 GetInitialTotalCount();
public:
	void ReDesignObjects();

	void AddSpawnCenterMotionDifferences(const FVector& InVelocity);
	void ReRotateObjectsOnSpawnCenter() const;
	
	FORCEINLINE void CheckForSimulationChanges();
	
	TArray<FDistObjectData>& GetObjectDistributionData() {return ObjectDistributionData;}
	void SetObjectDistributionData(const TArray<FDistObjectData>& InObjectDistributionData) {ObjectDistributionData = InObjectDistributionData;}

	void LevelActorDeleted(const AActor* InActor) const;

	void KeepSimulationChanges(const TArray<AActor*>& SimActors) const;

private:
	static TArray<AStaticMeshActor*> SpawnAndGetEmptyStaticMeshActors(const int32& InSpawnCount);

	static AStaticMeshActor* SpawnAndGetStaticMeshActor();

	FName GenerateUniquePresetPath(const AActor* InSampleActor) const;
	
	int32 CalculateSpawnCount();

	void FinishDistAsKeepActors() const;
	void FinishDistAsRunningMergeToll() const;

protected:
	virtual EObjectDistributionType GetDistributionType() {return EObjectDistributionType::Box;}
	virtual FVector CalculateLocation(const int32& InIndex,const int32& InLength);
	static FRotator CalculateRotation(const FVector& InLocation,const FVector& TargetLocation, const EObjectOrientation& Orientation);

private:

#pragma region Physics
	void SetSimulatePhysics() const;

public:
	FORCEINLINE bool IsInPıe();
#pragma endregion Physics

#pragma region Tickable
public:
	virtual void Tick( float DeltaTime ) override;
	virtual ETickableTickType GetTickableTickType() const override
	{
		return ETickableTickType::Always;
	}
	virtual TStatId GetStatId() const override
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT( FMyTickableThing, STATGROUP_Tickables );
	}
	virtual bool IsTickableWhenPaused() const override
	{
		return false;
	}
	virtual bool IsTickableInEditor() const override
	{
		return true;
	}
private:
	uint32 LastFrameNumberWeTicked = INDEX_NONE;

#pragma endregion Tickable

#pragma region KillZ

private:
	UPROPERTY()
	TArray<FName> ActorsInKillZ;

	bool bTraceForKillZ;
	float KillZCheckTimer;
	void CheckForKillZInSimulation();
	void DestroyKillZActors();

public:
	void HandleBeginPIE(const bool bIsSimulating);
	void HandleEndPIE(const bool bIsSimulating);
	
#pragma endregion KillZ	

public:
	UFUNCTION()
	void OnPresetLoaded();

#pragma region MixerMode
	
public:
	UFUNCTION()
	TArray<FString> GetEditableMixerPreset() const;

private:
	void OnAMixerPresetCheckStatusChanged(bool InNewCheckStatus, FName InUncheckedPresetName);

#pragma endregion  MixerMode
};


