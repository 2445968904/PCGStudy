// Copyright 2023 Leartes Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ODMeshData.generated.h"

class UStaticMesh;
class UMaterialInterface;

UENUM(BlueprintType, Category = "Object Distribution Data")
enum class EObjectDistributionType : uint8
{
	Box				UMETA(DisplayName = "Box"),
	Ring			UMETA(DisplayName = "Ring"),
	Disk			UMETA(DisplayName = "Disk"),
	Sphere			UMETA(DisplayName = "Sphere"),
	Helix			UMETA(DisplayName = "Helix"),
	Line			UMETA(DisplayName = "Line"),
	Grid			UMETA(DisplayName = "Grid"),
};

UENUM(BlueprintType, Category = "Object Distribution Data")
enum class EObjectOrientation : uint8
{
	Keep			UMETA(DisplayName = "Keep"),
	Inside			UMETA(DisplayName = "Inside"),
	Outside			UMETA(DisplayName = "Outside"),
	RandomZ			UMETA(DisplayName = "Random Around"),
	Random			UMETA(DisplayName = "Random"),
};

UENUM(BlueprintType, Category = "Object Distribution Data")
enum class EODLineAxis : uint8
{
	None			UMETA(DisplayName = "None"),
	AxisX			UMETA(DisplayName = "AxisX"),
	AxisY			UMETA(DisplayName = "AxisY"),
	AxisZ			UMETA(DisplayName = "AxisZ"),
};

UENUM(BlueprintType, Category = "Object Distribution Data")
enum class EODDistributionFinishType : uint8
{
	Keep			UMETA(DisplayName = "Keep",ToolTip = "It leaves the objects as separate individual static mesh actors."),
	Batch			UMETA(DisplayName = "Batch",ToolTip = "Batch the selected objects under a common actor based on the selected target component type."),
	Merge		    UMETA(DisplayName = "Run Merge Tool",ToolTip = "Runs the built-in merge tool for the created objects. ")

};

UENUM(BlueprintType, Category = "Object Distribution Data")
enum class EODMeshConversionType : uint8
{
	Sm			UMETA(DisplayName =  "SM Component",ToolTip = "Static Mesh Component"),
	Ism     	UMETA(DisplayName = "ISM Component",ToolTip = "Instanced Static Mesh Component"),
	HIsm		UMETA(DisplayName = "HISM Component",ToolTip = "Hierarchical Instanced Static Mesh Component"),
};


USTRUCT(BlueprintType)
struct FDistObjectPropertyData
{
	GENERATED_BODY()

	FORCEINLINE FDistObjectPropertyData();

	explicit FORCEINLINE FDistObjectPropertyData(int32 InSpawnCount, FVector2D InScaleRange,float  InLinearDamping,float  InAngularDamping,float InMass,EObjectOrientation InOrientationType);
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite,meta = (ClampMin = 0),Category = "Object Distribution Data")
	int32 SpawnCount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "Object Distribution Data")
	FVector2D  ScaleRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "Object Distribution Data")
	float  LinearDamping;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "Object Distribution Data")
	float  AngularDamping;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "Object Distribution Data")
	float  Mass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "Object Distribution Data")
	EObjectOrientation OrientationType;
};

FDistObjectPropertyData::FDistObjectPropertyData()
{
	SpawnCount = 5;ScaleRange = FVector2D(1,1);LinearDamping = 0.01;AngularDamping =0.0f; Mass = 100.0f; OrientationType = EObjectOrientation::Keep;
}

FDistObjectPropertyData::FDistObjectPropertyData(int32 InSpawnCount, FVector2D InScaleRange,float  InLinearDamping,float  InAngularDamping,float InMass,EObjectOrientation InOrientationType)
: SpawnCount(InSpawnCount), ScaleRange(InScaleRange),LinearDamping(InLinearDamping),AngularDamping(InAngularDamping),Mass(InMass), OrientationType(InOrientationType)
{
	
}


USTRUCT(BlueprintType)
struct FDistObjectData
{
	GENERATED_BODY()

	FORCEINLINE FDistObjectData();

	explicit FORCEINLINE FDistObjectData(const TSoftObjectPtr<UStaticMesh>& InStaticMesh,const bool InbSelectRandomMaterial, const TSoftObjectPtr<UMaterialInterface>& InSecondRandomMaterial,const bool InbSelectMaterialRandomly , const FDistObjectPropertyData& InDistObjectPropertyData);

	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "Object Distribution Data")
	TSoftObjectPtr<UStaticMesh> StaticMesh;
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = "Object Distribution Data",DisplayName="Override Material",meta=(ToolTip = "Assign new material for first material slot"))
	bool bSelectRandomMaterial = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite,DisplayName="New Material", meta=(EditCondition="bSelectRandomMaterial",EditConditionHides),Category = "Object Distribution Data")
	TSoftObjectPtr<UMaterialInterface> SecondRandomMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,DisplayName="Select Material Randomly", meta=(EditCondition="bSelectRandomMaterial",EditConditionHides,ToolTip = "It randomly changes the assigned material or doesn't change it at all."),Category = "Object Distribution Data")
	bool bSelectMaterialRandomly = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "Object Distribution Data")
	FDistObjectPropertyData DistributionProperties;
};

FDistObjectData::FDistObjectData()
{
	StaticMesh = nullptr; bSelectRandomMaterial = false;SecondRandomMaterial = nullptr;bSelectMaterialRandomly = false; DistributionProperties = FDistObjectPropertyData();
}

FDistObjectData::FDistObjectData(const TSoftObjectPtr<UStaticMesh>& InStaticMesh,const bool InbSelectRandomMaterial, const TSoftObjectPtr<UMaterialInterface>& InSecondRandomMaterial,const bool InbSelectMaterialRandomly, const FDistObjectPropertyData& InDistObjectPropertyData)
: StaticMesh(InStaticMesh),bSelectRandomMaterial(InbSelectRandomMaterial),SecondRandomMaterial(InSecondRandomMaterial),bSelectMaterialRandomly(InbSelectMaterialRandomly), DistributionProperties(InDistObjectPropertyData)
{
	
	
}

USTRUCT(BlueprintType)
struct FPresetMixData
{
	GENERATED_BODY()

	FORCEINLINE FPresetMixData();

	explicit FORCEINLINE FPresetMixData(const FName& IPresetName, const TArray<FDistObjectData>& InDistObjectData);

	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "Preset Data"/*,meta=(EditCondition="false")*/)
	FName PresetName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preset Data")
	TArray<FDistObjectData> DistObjectData;
};

FPresetMixData::FPresetMixData()
{
	PresetName = FName(); DistObjectData = TArray<FDistObjectData>();
}

FPresetMixData::FPresetMixData(const FName& IPresetName, const TArray<FDistObjectData>& InDistObjectData)
: PresetName(IPresetName),DistObjectData(InDistObjectData)
{
	
	
}