//$ Copyright 2015-23, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Utils/DungeonBoundingShapes.h"
#include "Frameworks/Snap/Lib/Connection/SnapConnectionConstants.h"
#include "Frameworks/Snap/Lib/SnapLibrary.h"

#include "Engine/DataAsset.h"
#include "SnapMapModuleDatabase.generated.h"

USTRUCT()
struct DUNGEONARCHITECTRUNTIME_API FSnapMapModuleDatabaseConnectionInfo {
    GENERATED_USTRUCT_BODY()

    UPROPERTY(VisibleAnywhere, Category = Module)
    FGuid ConnectionId;

    UPROPERTY(VisibleAnywhere, Category = Module)
    FTransform Transform;

    UPROPERTY(VisibleAnywhere, Category = Module)
    class USnapConnectionInfo* ConnectionInfo = nullptr;

    UPROPERTY(VisibleAnywhere, Category = Module)
    ESnapConnectionConstraint ConnectionConstraint = ESnapConnectionConstraint::Magnet;
};

USTRUCT()
struct DUNGEONARCHITECTRUNTIME_API FSnapMapModuleDatabaseItem {
    GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere, Category = Module)
    TSoftObjectPtr<UWorld> Level;

    UPROPERTY(EditAnywhere, Category = Module)
    FName Category = "Room";
    
    /** 
       Can the rooms / modules be rotated while stitching them together. E.g. disable this for isometric / platformer games 
       where the rooms are designed to be viewed at a certain angle 
    */
    UPROPERTY(EditAnywhere, Category = Module)
    bool bAllowRotation = true;
    
    /**
     * Alternate theme level file that should have the same structure as the master level file.
     * Use this to make different themed dungeons using the same generated layout.
     * Great for minimaps,  or creating an alternate world (e.g. player time travels and switches between the modern and ancient versions of the dungeon)
     */
    UPROPERTY(EditAnywhere, Category = Module)
    TMap<FString, TSoftObjectPtr<UWorld>> ThemedLevels;

    UPROPERTY(VisibleAnywhere, Category = Module)
    FBox ModuleBounds = FBox(ForceInitToZero);

    UPROPERTY(VisibleAnywhere, Category = Module)
    FDABoundsShapeList ModuleBoundShapes;

    UPROPERTY(VisibleAnywhere, Category = Module)
    TArray<FSnapMapModuleDatabaseConnectionInfo> Connections;
};

uint32 GetTypeHash(const FSnapMapModuleDatabaseItem& A);


UCLASS()
class DUNGEONARCHITECTRUNTIME_API USnapMapModuleDatabase : public UDataAsset {
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category = Module)
    TArray<FSnapMapModuleDatabaseItem> Modules;

};

