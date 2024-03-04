//$ Copyright 2015-23, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Utils/DungeonBoundingShapes.h"

#include "Engine/LevelStreamingDynamic.h"
#include "DungeonLevelStreamingModel.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogLevelStreamingModel, Log, All);

class ADungeon;
class ULevelStreaming;

UCLASS()
class UDungeonLevelStreaming : public ULevelStreamingDynamic {
    GENERATED_BODY()
public:
    
};

UCLASS(Blueprintable)
class DUNGEONARCHITECTRUNTIME_API UDungeonStreamingChunk : public UObject {
    GENERATED_UCLASS_BODY()
public:

    UPROPERTY(BlueprintReadOnly, Category=Dungeon)
    FGuid ID;

    UPROPERTY(BlueprintReadOnly, Category=Dungeon)
    FBox Bounds;

    UPROPERTY(BlueprintReadOnly, Category=Dungeon)
    FDABoundsShapeList BoundShapes;
    
    UPROPERTY(BlueprintReadOnly, Category=Dungeon)
    TSet<UDungeonStreamingChunk*> Neighbors;

    UPROPERTY(BlueprintReadOnly, Category=Dungeon)
    bool bSpawnRoomChunk = false;

public:
    UFUNCTION(BlueprintCallable, Category=Dungeon)
    TArray<AActor*> GetLoadedChunkActors();
    
    UFUNCTION(BlueprintCallable, Category=Dungeon)
    TArray<AActor*> GetLoadedChunkActorsOfType(const UClass* ActorClass);
    
    UFUNCTION(BlueprintCallable, Category=Dungeon)
    AActor* GetLoadedChunkActorOfType(const UClass* ActorClass);
    
private:
    UPROPERTY()
    bool bShouldBeVisible = false;

    UPROPERTY()
    bool bShouldBeLoaded = false;

private:
    UPROPERTY(Transient)
    UDungeonLevelStreaming* LevelStreaming = nullptr;

    UPROPERTY(Transient)
    UPackage* LevelPackage = nullptr;

    UPROPERTY()
    TArray<TWeakObjectPtr<AActor>> ChunkListeners;

    TArray<TWeakObjectPtr<AActor>> ManagedActors;
    
private:
    bool bIsLoaded = false;
    bool bIsVisible = false;
    FRandomStream FallbackRandomStream;

public:
    void CreateLevelStreaming(UObject* WorldContextObject, TSoftObjectPtr<UWorld> Level, uint32 InstanceId, const FTransform& Transform, bool& bOutSuccess);
    void SetStreamingLevelState(bool bInVisible, bool bInLoaded);
    bool RequiresStateUpdate(bool bInVisible, bool bInLoaded) const;
    void RegisterManagedActor(TWeakObjectPtr<AActor> InActor);
    virtual void DestroyChunk(UWorld* InWorld);
    
    UPackage* GetLevelPackage() const;
    ULevel* GetLoadedLevel() const;
    ULevelStreaming* GetStreamingLevel() const;
    FTransform GetLevelTransform() const;
    bool IsLevelLoaded() const { return bIsLoaded; }
    bool IsLevelVisible() const { return bIsVisible; }

    UFUNCTION()
    virtual void HandleChunkVisible();

    UFUNCTION()
    virtual void HandleChunkHidden();
    
    UFUNCTION()
    virtual void HandleChunkLoaded();

    UFUNCTION()
    virtual void HandleChunkUnloaded();

private:
    void RegisterStreamingCallbacks();
    void NotifyEventListener_OnVisible();
    void NotifyEventListener_OnHidden();
    void NotifyEventListener_OnLoaded();
    void NotifyEventListener_OnUnloaded();

    ADungeon* GetOwningDungeon() const;
    FRandomStream& GetRandomStream(ADungeon* InDungeon);
};

class UDungeonLevelStreamingNavigation;
enum class EDungeonLevelStreamChunkSelection : uint8;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDALevelStreamerBindableEvent, ADungeon*, Dungeon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDALevelStreamerStateChangeDelegate, ADungeon*, Dungeon, UDungeonStreamingChunk*, Chunk);

UCLASS()
class DUNGEONARCHITECTRUNTIME_API UDungeonLevelStreamingModel : public UObject {
    GENERATED_UCLASS_BODY()

public:
    UPROPERTY()
    TArray<UDungeonStreamingChunk*> Chunks;

    UPROPERTY()
    UDungeonLevelStreamingNavigation* StreamingNavigation;

    UPROPERTY(BlueprintAssignable, Category = Dungeon)
    FDALevelStreamerBindableEvent OnInitialChunksLoaded;

    UPROPERTY(BlueprintAssignable, Category = Dungeon)
    FDALevelStreamerStateChangeDelegate OnChunkLoaded;
    
    UPROPERTY(BlueprintAssignable, Category = Dungeon)
    FDALevelStreamerStateChangeDelegate OnChunkUnloaded;

    UPROPERTY(BlueprintAssignable, Category = Dungeon)
    FDALevelStreamerStateChangeDelegate OnChunkVisible;
    
    UPROPERTY(BlueprintAssignable, Category = Dungeon)
    FDALevelStreamerStateChangeDelegate OnChunkHidden;

public:
    void Initialize(UWorld* InWorld);
    void Release(UWorld* InWorld);
    bool HasNotifiedInitialChunkLoadEvent() const { return bNotifiedInitialChunkLoadEvent; }
    void GetStreamingSourceLocations(UWorld* InWorld, EDungeonLevelStreamChunkSelection InChunkSelection, TArray<FVector>& OutSourceLocations) const;
    
    void NotifyInitialChunksLoaded();
    void NotifyChunkLoaded(UDungeonStreamingChunk* InChunk);
    void NotifyChunkUnloaded(UDungeonStreamingChunk* InChunk);
    void NotifyChunkVisible(UDungeonStreamingChunk* InChunk);
    void NotifyChunkHidden(UDungeonStreamingChunk* InChunk);
    
private:
    bool bNotifiedInitialChunkLoadEvent = false;
};


class DUNGEONARCHITECTRUNTIME_API FLevelStreamLoader {
public:
    static UDungeonLevelStreaming* LoadLevelInstanceBySoftObjectPtr(const UObject* WorldContextObject,
                                                                    const TSoftObjectPtr<UWorld>& Level, uint32 InstanceId,
                                                                    const FTransform& Transform, bool& bOutSuccess, UPackage*& OutLevelPackage);
private:
    // Counter used by LoadLevelInstance to create unique level names
    static UDungeonLevelStreaming* LoadLevelInstance_Internal(UWorld* World, const FString& LongPackageName,
                                                              uint32 InstanceId, const FTransform& Transform, UPackage*& OutLevelPackage);
};

