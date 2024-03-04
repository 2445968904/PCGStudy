//$ Copyright 2015-23, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Utils/DungeonBPFunctionLibrary.h"

#include "Core/Dungeon.h"
#include "Core/Utils/DungeonModelHelper.h"
#include "Frameworks/ThemeEngine/SceneProviders/SceneProviderCommand.h"

#include "Engine/LevelStreamingDynamic.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogDungeonBPLibrary, Log, All);

AActor* UDungeonBPFunctionLibrary::SpawnDungeonOwnedActor(ADungeon* Dungeon, TSubclassOf<AActor> ActorClass,
                                                          const FTransform& Transform) {
    if (!Dungeon) {
        UE_LOG(LogDungeonBPLibrary, Error, TEXT("Invalid Dungeon reference passed to SpawnDungeonActor"));
        return nullptr;
    }

    UWorld* World = Dungeon->GetWorld();
    AActor* Actor = World->SpawnActor<AActor>(ActorClass, Transform);
    if (Actor) {
#if WITH_EDITOR
        if (World->WorldType == EWorldType::Editor) {
            Actor->RerunConstructionScripts();
        }
#endif // WITH_EDITOR
        const FName DungeonIdTag = UDungeonModelHelper::GetDungeonIdTag(Dungeon);
        Actor->Tags.Add(DungeonIdTag);

        FSceneProviderCommand::MoveToFolder(Dungeon, Actor);
        FSceneProviderCommand::TagAsComplexObject(Actor);
    }

    return Actor;
}

bool UDungeonBPFunctionLibrary::ActorBelongsToDungeon(ADungeon* Dungeon, AActor* ActorToCheck) {
    if (!Dungeon || !ActorToCheck) return false;
    const FName DungeonIdTag = UDungeonModelHelper::GetDungeonIdTag(Dungeon);
    return ActorToCheck->Tags.Contains(DungeonIdTag);
}

bool UDungeonBPFunctionLibrary::DungeonObjectHasAuthority(UObject* Object) {
    AActor* Actor = nullptr;
    while (Object) {
        Actor = Cast<AActor>(Object);
        if (Actor) break;
        Object = Object->GetOuter();
    }
    
    if (!Actor) return false;
    return Actor->HasAuthority();
}

void UDungeonBPFunctionLibrary::DAGetAllActorsOfClass(const AActor* WorldContextObject, TSubclassOf<AActor> ActorClass, TArray<AActor*>& OutActors) {
    UGameplayStatics::GetAllActorsOfClass(WorldContextObject, ActorClass, OutActors);
}

AActor* UDungeonBPFunctionLibrary::DAGetActorOfClass(const AActor* WorldContextObject, TSubclassOf<AActor> ActorClass) {
    return UGameplayStatics::GetActorOfClass(WorldContextObject, ActorClass);
}

void UDungeonBPFunctionLibrary::DARecreateActorInLevel(AActor* InActor, ULevel* TargetLevel, AActor*& NewTargetActor) {
    if (!InActor || !TargetLevel) {
        return;
    }
    
    UWorld* World = TargetLevel->GetWorld();
    FActorSpawnParameters SpawnParams;
    SpawnParams.Template = InActor;
    SpawnParams.bDeferConstruction = true;
    SpawnParams.OverrideLevel = TargetLevel;
        
    NewTargetActor = World->SpawnActor<AActor>(InActor->GetClass(), SpawnParams);
    const FTransform TargetTransform = InActor->GetActorTransform();
    InActor->Destroy();
    
    NewTargetActor->FinishSpawning(TargetTransform);
}

int32 UDungeonBPFunctionLibrary::DAHashLocation(const FVector& Location) {
    return GetTypeHash(Location);
}

int32 UDungeonBPFunctionLibrary::DAHashTransform(const FTransform& Transform) {
    return GetTypeHash(Transform);
}

int32 UDungeonBPFunctionLibrary::DAHashCombine(int32 A, int32 B) {
    return HashCombine(A, B);
}

