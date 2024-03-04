// Copyright 2023 Leartes Studios. All Rights Reserved.


#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Data/ODMeshData.h"
#include "ODPresetData.generated.h"

class UStaticMesh;

USTRUCT(BlueprintType)
struct FODPresetData : public FTableRowBase
{
	GENERATED_USTRUCT_BODY()
	
	FODPresetData();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preset Data")
	FName PresetName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preset Data")
	TArray<FDistObjectData> DistObjectData;
};

FORCEINLINE FODPresetData::FODPresetData()
{
	
}


USTRUCT(BlueprintType)
struct FPresetMixerMapData
{
	GENERATED_BODY()
	
	FPresetMixerMapData();

	explicit FORCEINLINE FPresetMixerMapData(const FString& InPresetName, bool InCheckState);
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preset Mixer Data")
	FString PresetName = FString();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preset Mixer Data")
	bool CheckState = false;
};

FORCEINLINE FPresetMixerMapData::FPresetMixerMapData()
{
	
}

FORCEINLINE FPresetMixerMapData::FPresetMixerMapData(const FString& InPresetName, bool InCheckState): PresetName(InPresetName), CheckState(InCheckState)
{
	
}


