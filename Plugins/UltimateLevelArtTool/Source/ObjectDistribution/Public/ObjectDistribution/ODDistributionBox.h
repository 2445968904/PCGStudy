// Copyright 2023 Leartes Studios. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "ODDistributionBase.h"
#include "ODDistributionBox.generated.h"

/**
 * 
 */
UCLASS()
class OBJECTDISTRIBUTION_API UODDistributionBox : public UODDistributionBase
{
	GENERATED_BODY()

public:
	UODDistributionBox(const FObjectInitializer& ObjectInitializer);

	virtual void LoadDistData() override; 

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	UPROPERTY(EditAnywhere,Category = "Distribution")
	FVector SpawnRange = FVector(500.0f,500.0f,500.0f);
	
	virtual EObjectDistributionType GetDistributionType() override {return EObjectDistributionType::Box;}

private:
	virtual FVector CalculateLocation(const int32& InIndex,const int32& InLength) override;
	
};
