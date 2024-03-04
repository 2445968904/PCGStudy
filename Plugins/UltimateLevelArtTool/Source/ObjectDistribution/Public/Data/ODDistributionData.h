// Copyright 2023 Leartes Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ODMeshData.h"
#include "ODDistributionData.generated.h"

USTRUCT()
struct FODDiskDistributionData
{
	GENERATED_BODY()

	FORCEINLINE FODDiskDistributionData();

	explicit FORCEINLINE FODDiskDistributionData(float InRadius, bool InbRandZRangeCond, const FVector2D&  InRandZRange,bool  InChaos);
	
	UPROPERTY()
	float Radius = 3000.0f;

	UPROPERTY()
	bool bRandZRangeCond = false;
	
	UPROPERTY()
	FVector2D RandZRange = FVector2D(0.0f,500.0f);

	UPROPERTY()
	bool Chaos = false;
};

FODDiskDistributionData::FODDiskDistributionData()
{
}
inline FODDiskDistributionData::FODDiskDistributionData(float InRadius, bool InbRandZRangeCond, const FVector2D& InRandZRange,bool InChaos)
: Radius(InRadius), bRandZRangeCond(InbRandZRangeCond),RandZRange(InRandZRange),Chaos(InChaos)
{
	
}

//********************************************************************************************************************************//


USTRUCT()
struct FODBoxDistributionData
{
	GENERATED_BODY()

	FORCEINLINE FODBoxDistributionData();

	explicit FORCEINLINE FODBoxDistributionData(const FVector& InSpawnRange);
	
	UPROPERTY()
	FVector SpawnRange = FVector(500.0f,500.0f,500.0f);
};

FODBoxDistributionData::FODBoxDistributionData()
{
}
inline FODBoxDistributionData::FODBoxDistributionData(const FVector& InSpawnRange)
: SpawnRange(InSpawnRange)
{
	
}

/********************************************************************************************************************************/
	
USTRUCT()
struct FODLineDistributionData
{
	GENERATED_BODY()

	FORCEINLINE FODLineDistributionData();

	explicit FORCEINLINE FODLineDistributionData(EODLineAxis InLineAxis, const float& InLineLength,bool InbRandomOffset, const FVector& InRandomOffset,bool InPivotCentered);
	
	UPROPERTY()
	EODLineAxis LineAxis  = EODLineAxis::AxisX;
	
	UPROPERTY()
	float LineLength = 5000.0f;

	UPROPERTY()
	bool bRandomOffset = false;
	
	UPROPERTY()
	FVector RandomOffset = FVector::ZeroVector;	

	UPROPERTY()
	bool PivotCentered = true;
};

FODLineDistributionData::FODLineDistributionData()
{
}
inline FODLineDistributionData::FODLineDistributionData(EODLineAxis InLineAxis, const float& InLineLength,bool InbRandomOffset, const FVector& InRandomOffset,bool InPivotCentered)
: LineAxis(InLineAxis),LineLength(InLineLength),bRandomOffset(InbRandomOffset),RandomOffset(InRandomOffset),PivotCentered(InPivotCentered)
{

}

/********************************************************************************************************************************/
	
USTRUCT()
struct FODRingDistributionData
{
	GENERATED_BODY()

	FORCEINLINE FODRingDistributionData();

	explicit FORCEINLINE FODRingDistributionData(const float& InArcAngle, const float& InOffset,float InRadius, bool InbRandZRangeCond, const FVector2D& InRandZRange,bool InChaos);
	
	UPROPERTY()
	float ArcAngle = 360.0f;

	UPROPERTY()
	float Offset =  0.0f;

	UPROPERTY()
	float Radius = 1000.0f;
	
	UPROPERTY()
	bool bRandZRangeCond = false;
	
	UPROPERTY()
	FVector2D RandZRange = FVector2D(0.0f,500.0f);

	UPROPERTY()
	bool Chaos = false;
};

FODRingDistributionData::FODRingDistributionData()
{
}
inline FODRingDistributionData::FODRingDistributionData(const float& InArcAngle, const float& InOffset,float InRadius, bool InbRandZRangeCond, const FVector2D& InRandZRange,bool InChaos)
: ArcAngle(InArcAngle),Offset(InOffset),Radius(InRadius),bRandZRangeCond(InbRandZRangeCond),RandZRange(InRandZRange),Chaos(InChaos)
{

}

/********************************************************************************************************************************/


USTRUCT()
struct FODSphereDistributionData
{
	GENERATED_BODY()

	FORCEINLINE FODSphereDistributionData();

	explicit FORCEINLINE FODSphereDistributionData(const float& InSpiralWinding, const float& InSphereRadius,bool  InChaos);
	
	UPROPERTY()
	float SpiralWinding = 0.5f;

	UPROPERTY()
	float SphereRadius = 1000.0f;

	UPROPERTY()
	bool Chaos = false;
};

FODSphereDistributionData::FODSphereDistributionData()
{
}
inline FODSphereDistributionData::FODSphereDistributionData(const float& InSpiralWinding, const float& InSphereRadius,bool  InChaos)
: SpiralWinding(InSpiralWinding), SphereRadius(InSphereRadius),Chaos(InChaos)
{
	
}

/********************************************************************************************************************************/


USTRUCT()
struct FODGridDistributionData
{
	GENERATED_BODY()

	FORCEINLINE FODGridDistributionData();

	explicit FORCEINLINE FODGridDistributionData(const FIntPoint& InSpiralWinding, const float& InGridLength,bool  InChaos);
	
	UPROPERTY()
	FIntPoint GridSize = FIntPoint(5,5);

	UPROPERTY()
	float GridLength = 500.0f;

	UPROPERTY()
	bool Chaos = false;
};

FODGridDistributionData::FODGridDistributionData()
{
}
inline FODGridDistributionData::FODGridDistributionData(const FIntPoint& InSpiralWinding, const float& InGridLength,bool  InChaos)
: GridSize(InSpiralWinding), GridLength(InGridLength),Chaos(InChaos)
{
	
}

/********************************************************************************************************************************/


USTRUCT()
struct FODHelixDistributionData
{
	GENERATED_BODY()

	FORCEINLINE FODHelixDistributionData();

	explicit FORCEINLINE FODHelixDistributionData(const float& InRadius, const float& InLength,bool  InChaos);
	
	UPROPERTY()
	float Radius = 500.0f;

	UPROPERTY()
	float Length = 2000.0f;
	
	UPROPERTY()
	bool Chaos = false;
};

FODHelixDistributionData::FODHelixDistributionData()
{
}
inline FODHelixDistributionData::FODHelixDistributionData(const float& InRadius, const float& InLength,bool  InChaos)
: Radius(InRadius), Length(InLength),Chaos(InChaos)
{
	
}