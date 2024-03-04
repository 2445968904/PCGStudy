//$ Copyright 2015-23, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DungeonBPFunctionLibrary.generated.h"

class AActor;
class ADungeon;

UCLASS()
class DUNGEONARCHITECTRUNTIME_API UDungeonBPFunctionLibrary : public UBlueprintFunctionLibrary {
    GENERATED_BODY()

    /** Set basic global leap tracking options */
    UFUNCTION(BlueprintCallable, Category = "Dungeon")
    static AActor* SpawnDungeonOwnedActor(ADungeon* Dungeon, TSubclassOf<AActor> ActorClass, const FTransform& Transform);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Dungeon")
    static bool ActorBelongsToDungeon(ADungeon* Dungeon, AActor* ActorToCheck);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Dungeon")
	static bool DungeonObjectHasAuthority(UObject* Object);
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Dungeon", meta=(DeterminesOutputType="ActorClass", DynamicOutputParam="OutActors"))
	static void DAGetAllActorsOfClass(const AActor* WorldContextObject, TSubclassOf<AActor> ActorClass, TArray<AActor*>& OutActors);

	UFUNCTION(BlueprintCallable, Category="Dungeon", meta=(DeterminesOutputType="ActorClass"))
	static AActor* DAGetActorOfClass(const AActor* WorldContextObject, TSubclassOf<AActor> ActorClass);

	UFUNCTION(BlueprintCallable, Category="Dungeon")
	static void DARecreateActorInLevel(AActor* InActor, ULevel* TargetLevel, AActor*& NewTargetActor);

	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Dungeon")
	static int32 DAHashLocation(const FVector& Location);
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Dungeon")
	static int32 DAHashTransform(const FTransform& Transform);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Dungeon")
	static int32 DAHashCombine(int32 A, int32 B);
};

