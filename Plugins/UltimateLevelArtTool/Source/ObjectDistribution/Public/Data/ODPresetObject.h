// Copyright 2023 Leartes Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ODPresetObject.generated.h"

DECLARE_DELEGATE(FOnPresetUpdated);
DECLARE_DELEGATE_OneParam(FOnPresetCategoryHidden,bool);

UCLASS()
class OBJECTDISTRIBUTION_API UODPresetObject : public UObject
{
	GENERATED_BODY()

public:
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	virtual void BeginDestroy() override;

	FOnPresetUpdated OnPresetUpdated;
	FOnPresetCategoryHidden OnPresetCategoryHidden;
	
	void SetupPresets();

	void PresetExpansionStateChanged(bool& InIsItOpen);
	
};
