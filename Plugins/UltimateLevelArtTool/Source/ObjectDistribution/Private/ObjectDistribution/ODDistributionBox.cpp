// Copyright 2023 Leartes Studios. All Rights Reserved.

#include "ObjectDistribution/ODDistributionBox.h"
#include "Editor.h"
#include "ODToolSettings.h"
#include "ODToolSubsystem.h"
#include "Math/Vector.h"

UODDistributionBox::UODDistributionBox(const FObjectInitializer& ObjectInitializer) :Super(ObjectInitializer)
{
	DistributionType = EObjectDistributionType::Box;
}

void UODDistributionBox::LoadDistData()
{
	Super::LoadDistData();
	
	if(GetToolSubsystem() && GetToolSubsystem()->GetODToolSettings())
	{
		const auto& BoxData = GetToolSubsystem()->GetODToolSettings()->BoxDistributionData;
		
		SpawnRange = BoxData.SpawnRange;
	}
}

void UODDistributionBox::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if(GetToolSubsystem() && GetToolSubsystem()->GetODToolSettings())
	{
		auto& BoxData = GetToolSubsystem()->GetODToolSettings()->BoxDistributionData;
		
		BoxData.SpawnRange =  SpawnRange;
		
		if(PropertyChangedEvent.ChangeType != EPropertyChangeType::Interactive)
		{
			GetToolSubsystem()->GetODToolSettings()->SaveConfig();
		}
	}
}


FVector UODDistributionBox::CalculateLocation(const int32& InIndex, const int32& InLength)
{
	return FVector(
		FMath::FRandRange(-SpawnRange.X, SpawnRange.X),
		FMath::FRandRange(-SpawnRange.Y, SpawnRange.Y),
		FMath::FRandRange(-SpawnRange.Z, SpawnRange.Z)
	);
}


