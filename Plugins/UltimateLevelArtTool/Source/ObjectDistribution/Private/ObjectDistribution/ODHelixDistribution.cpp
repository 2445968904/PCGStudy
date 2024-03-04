// Copyright 2023 Leartes Studios. All Rights Reserved.


#include "ObjectDistribution/ODHelixDistribution.h"
#include "Editor.h"
#include "ODToolSettings.h"
#include "ODToolSubsystem.h"

UODHelixDistribution::UODHelixDistribution(const FObjectInitializer& ObjectInitializer) :Super(ObjectInitializer)
{
	DistributionType = EObjectDistributionType::Helix;
	
	if(GetToolSubsystem() && GetToolSubsystem()->GetODToolSettings())
	{
		const auto& HelixData = GetToolSubsystem()->GetODToolSettings()->HelixDistributionData;

		Radius = HelixData.Radius;
		Length = HelixData.Length;
		Chaos = HelixData.Chaos;
	}
}

void UODHelixDistribution::LoadDistData()
{
	Super::LoadDistData();
	
	if(GetToolSubsystem() && GetToolSubsystem()->GetODToolSettings())
	{
		const auto& HelixData = GetToolSubsystem()->GetODToolSettings()->HelixDistributionData;

		Radius = HelixData.Radius;
		Length = HelixData.Length;
		Chaos = HelixData.Chaos;
	}
}

void UODHelixDistribution::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if(GetToolSubsystem() && GetToolSubsystem()->GetODToolSettings())
	{
		auto& HelixData = GetToolSubsystem()->GetODToolSettings()->HelixDistributionData;

		HelixData.Radius = Radius;
		HelixData.Length = Length;
		HelixData.Chaos = Chaos;

		if(PropertyChangedEvent.ChangeType != EPropertyChangeType::Interactive)
		{
			GetToolSubsystem()->GetODToolSettings()->SaveConfig();
		}
	}
}

FVector UODHelixDistribution::CalculateLocation(const int32& InIndex, const int32& InLength)
{
	const float angleStep = 360.0f / InLength;
	const float angle = InIndex * angleStep;
	
	FVector TargetPoint = FVector(
		Radius * cosf(angle),
		Radius * sinf(angle),
		Length * InIndex / (InLength - 1));
	
	if(Chaos)
	{
		TargetPoint.X = TargetPoint.X * FMath::RandRange(0.1f,1.1f);
		TargetPoint.Y = TargetPoint.Y * FMath::RandRange(0.1f,1.1f);
	}
	return TargetPoint;
}
