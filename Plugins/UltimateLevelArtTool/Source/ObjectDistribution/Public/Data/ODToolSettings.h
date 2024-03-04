// Copyright 2023 Leartes Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ODDistributionData.h"
#include "ODToolSettings.generated.h"

DECLARE_DELEGATE(FOnToolSettingsSetToReset);

UCLASS(Config = ODToolSettings)
class OBJECTDISTRIBUTION_API UODToolSettings : public UObject
{
	GENERATED_BODY()

public:
	
	UPROPERTY(Config)
	FODDiskDistributionData DiskDistributionData = FODDiskDistributionData();

	UPROPERTY(Config)
	FODBoxDistributionData BoxDistributionData = FODBoxDistributionData();

	UPROPERTY(Config)
	FODLineDistributionData LineDistributionData = FODLineDistributionData();

	UPROPERTY(Config)
	FODRingDistributionData RingDistributionData = FODRingDistributionData();

	UPROPERTY(Config)
	FODSphereDistributionData SphereDistributionData = FODSphereDistributionData();

	UPROPERTY(Config)
	FODGridDistributionData GridDistributionData = FODGridDistributionData();

	UPROPERTY(Config)
	FODHelixDistributionData HelixDistributionData = FODHelixDistributionData();

	UPROPERTY(Config)
	bool bSimulatePhysics = true;
	
	UPROPERTY(Config)
	bool bDisableSimAfterFinish = false;
	
	UPROPERTY(Config)
	float KillZ = -500.0f;
	
	UPROPERTY(Config)
	bool bDrawSpawnBounds = true;

	UPROPERTY(Config)
	FLinearColor BoundsColor = FLinearColor( 0.339158f, 0.088386f, 0.953125f);
	
	UPROPERTY(Config)
	EODDistributionFinishType FinishType = EODDistributionFinishType::Keep;

	UPROPERTY(Config)
	EODMeshConversionType TargetType = EODMeshConversionType::Ism;
	

	
	void LoadDefaultSettings();

	FOnToolSettingsSetToReset OnToolSettingsSetToReset;
};

inline void UODToolSettings::LoadDefaultSettings()
{
	DiskDistributionData = FODDiskDistributionData();

	BoxDistributionData = FODBoxDistributionData();

	LineDistributionData = FODLineDistributionData();

	RingDistributionData = FODRingDistributionData();

	SphereDistributionData = FODSphereDistributionData();

	GridDistributionData = FODGridDistributionData();

	HelixDistributionData = FODHelixDistributionData();

	bSimulatePhysics = true;
	
	bDisableSimAfterFinish = true;
	
	KillZ = -500.0f;
	
	bDrawSpawnBounds = true;

	BoundsColor = FLinearColor( 0.339158f, 0.088386f, 0.953125f);

	FinishType = EODDistributionFinishType::Keep;

	TargetType = EODMeshConversionType::Ism;

	SaveConfig();

	OnToolSettingsSetToReset.ExecuteIfBound();
}
