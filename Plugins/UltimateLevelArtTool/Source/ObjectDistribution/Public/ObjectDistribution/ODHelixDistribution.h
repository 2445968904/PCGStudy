// Copyright 2023 Leartes Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ODDistributionBase.h"
#include "ODHelixDistribution.generated.h"

UCLASS()
class OBJECTDISTRIBUTION_API UODHelixDistribution : public UODDistributionBase
{
	GENERATED_BODY()

public:
	UODHelixDistribution(const FObjectInitializer& ObjectInitializer);

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	virtual void LoadDistData() override; 
	
	UPROPERTY(EditAnywhere,Category = "Distribution")
	float Radius = 500.0f;

	UPROPERTY(EditAnywhere,Category = "Distribution")
	float Length = 2000.0f;
	
	UPROPERTY(EditAnywhere,Category = "Distribution")
	bool Chaos = false;
	
	virtual EObjectDistributionType GetDistributionType() override {return EObjectDistributionType::Helix;}
private:
	virtual FVector CalculateLocation(const int32& InIndex,const int32& InLength) override;
	
};
