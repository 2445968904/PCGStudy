//$ Copyright 2015-23, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Utils/DungeonBoundingShapes.h"
#include "SnapModuleInstanceSerialization.generated.h"

USTRUCT()
struct DUNGEONARCHITECTRUNTIME_API FSnapModuleInstanceSerializedData {
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FGuid ModuleInstanceId;

	UPROPERTY()
	FTransform WorldTransform;

	UPROPERTY()
	TSoftObjectPtr<UWorld> Level;

	UPROPERTY()
	TMap<FString, TSoftObjectPtr<UWorld>> ThemedLevels;
    
	UPROPERTY()
	FName Category;

	UPROPERTY()
	FBox ModuleBounds = FBox(ForceInitToZero);
    
	UPROPERTY()
	FDABoundsShapeList ModuleBoundShapes;
    
	TSoftObjectPtr<UWorld> GetThemedLevel(const FString& InThemeName) const;
};

