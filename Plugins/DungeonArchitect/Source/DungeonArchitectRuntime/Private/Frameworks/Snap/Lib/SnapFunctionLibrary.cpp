//$ Copyright 2015-23, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Snap/Lib/SnapFunctionLibrary.h"

#include "Core/Utils/DungeonModelHelper.h"
#include "Frameworks/Snap/Lib/Connection/SnapConnectionActor.h"
#include "Frameworks/Snap/Lib/SnapDungeonModelBase.h"
#include "Frameworks/ThemeEngine/SceneProviders/SceneProviderCommand.h"

#include "Engine/DirectionalLight.h"
#include "Engine/LevelBounds.h"
#include "Engine/LevelScriptActor.h"
#include "Engine/SkyLight.h"

DEFINE_LOG_CATEGORY_STATIC(LogSnapDungeonFunctionLibrary, Log, All);


bool USnapDungeonFunctionLibrary::BuildSnapThemedDungeon(ADungeon* Dungeon, UWorld* World, AActor* ParentActor, const FString& ThemeId, const FString& ConnectionMarkerPrefix) {
	if (!Dungeon) {
		UE_LOG(LogSnapDungeonFunctionLibrary, Log, TEXT("Dungeon actor is NULL"));
		return false;
	}
	
	if (!World) {
		World = Dungeon->GetWorld();
	}
	const FName DungeonTag = UDungeonModelHelper::GetDungeonIdTag(Dungeon);

	auto SetupSpawnedActor = [&DungeonTag, ParentActor](AActor* InActor) {
		if (InActor) {
			InActor->Tags.Add(DungeonTag);
			InActor->Tags.Add(FSceneProviderCommand::TagComplexActor);
			InActor->SetActorEnableCollision(false);
			if (InActor->GetRootComponent()) {
				InActor->GetRootComponent()->SetMobility(EComponentMobility::Movable);
			}
			if (ParentActor) {
				InActor->AttachToActor(ParentActor, FAttachmentTransformRules(EAttachmentRule::KeepWorld, false));
			}
		}
	};

	TFunction<FTransform(const FTransform& WorldTransform)> GetWorldToTargetTransform;
	{
		/*
		FBox DungeonWorldBox(ForceInit);
		for (const FSnapModuleInstanceSerializedData& ModuleInstance : Model->ModuleInstances) {
			DungeonWorldBox += ModuleInstance.ModuleBounds.TransformBy(ModuleInstance.WorldTransform);
		};
		FVector DungeonWorldCenter = DungeonWorldBox.GetCenter() - Dungeon->GetActorLocation();
		DungeonWorldCenter.Z = 0;
		*/

		const FTransform InverseDungeonTransform = Dungeon->GetTransform().Inverse();
		const FTransform LocalToTargetTransform = ParentActor ? ParentActor->GetTransform() : FTransform::Identity;
		GetWorldToTargetTransform = [&](const FTransform& WorldTransform) {
			return WorldTransform
					//* FTransform(-DungeonWorldCenter)
					* InverseDungeonTransform
					* LocalToTargetTransform;
		};
	}

	USnapDungeonModelBase* Model = Cast<USnapDungeonModelBase>(Dungeon->DungeonModel);
	if (!Model) {
		return false;
	}

	for (const FSnapModuleInstanceSerializedData& ModuleInstance : Model->ModuleInstances) {
        TArray<AActor*> ChunkActors;
        TMap<AActor*, AActor*> TemplateToSpawnedActorMap;

		TSoftObjectPtr<UWorld> LevelToLoad = ModuleInstance.GetThemedLevel(ThemeId);
        const UWorld* ModuleLevel = LevelToLoad.LoadSynchronous();
		if (!ModuleLevel) {
			continue;
		}
		
		const FTransform ChunkTransform = GetWorldToTargetTransform(ModuleInstance.WorldTransform);
        const FGuid AbstractNodeId = ModuleInstance.ModuleInstanceId;
        
        // Spawn the module actors
        for (AActor* ActorTemplate : ModuleLevel->PersistentLevel->Actors) {
            if (!ActorTemplate
				|| ActorTemplate->IsA<AInfo>()
				|| ActorTemplate->IsA<ALevelScriptActor>()
				|| ActorTemplate->IsA<ALevelBounds>()
				|| ActorTemplate->IsA<ASkyLight>()
				|| ActorTemplate->IsA<ADirectionalLight>()
				|| ActorTemplate->IsA<ASnapConnectionActor>()
				)
            {
	            continue;
            }
            AActor* Template = ActorTemplate;
            FTransform ActorTransform = FTransform::Identity;
            if (Template->IsA<ABrush>()) {
                Template = nullptr;
                ActorTransform = ActorTemplate->GetTransform();
            }
            
            FActorSpawnParameters SpawnParams;
            SpawnParams.Template = Template;
            const FTransform SpawnTransform = ActorTransform * ChunkTransform;
            AActor* SpawnedModuleActor = World->SpawnActor<AActor>(ActorTemplate->GetClass(), SpawnTransform, SpawnParams);
        	SetupSpawnedActor(SpawnedModuleActor);
            
            ChunkActors.Add(SpawnedModuleActor);
            TemplateToSpawnedActorMap.Add(ActorTemplate, SpawnedModuleActor);
        }

        // Replace template actor references
        for (AActor* ChunkActor : ChunkActors) {
            if (!ChunkActor) continue;
            for (TFieldIterator<FProperty> PropertyIterator(ChunkActor->GetClass()); PropertyIterator; ++PropertyIterator) {
                FProperty* Property = *PropertyIterator;
                if (!Property) continue;
                if (const FObjectProperty* ObjProperty = CastField<FObjectProperty>(Property)) {
                    UObject* PropertyObjectValue = ObjProperty->GetObjectPropertyValue_InContainer(ChunkActor);
                    if (PropertyObjectValue && PropertyObjectValue->HasAnyFlags(RF_DefaultSubObject | RF_ArchetypeObject)) {
                        continue;
                    }

                    if (AActor* PropertyActor = Cast<AActor>(PropertyObjectValue)) {
                        AActor** CrossReferencePtr = TemplateToSpawnedActorMap.Find(PropertyActor);
                        if (CrossReferencePtr) {
                            AActor* CrossReference = *CrossReferencePtr;
                            ObjProperty->SetObjectPropertyValue_InContainer(ChunkActor, CrossReference);
                        }
                    }
                }
            }
        } 
    }

	// Build the doors
	ULevel* PersistentLevel = World->PersistentLevel;
	for (const FSnapConnectionInstance& ConnectionInfo : Model->Connections) {
		FTransform ConnectionTransform = GetWorldToTargetTransform(ConnectionInfo.WorldTransform);
		ASnapConnectionActor* ConnectionActor = World->SpawnActor<ASnapConnectionActor>(ASnapConnectionActor::StaticClass(), ConnectionTransform);
		ConnectionActor->ConnectionComponent->ConnectionInfo = ConnectionInfo.ConnectionInfo.Get();
		ConnectionActor->ConnectionComponent->ConnectionState = ESnapConnectionState::Door;
		ConnectionActor->ConnectionComponent->DoorType = ConnectionInfo.DoorType;
		ConnectionActor->ConnectionComponent->MarkerPrefix = ConnectionMarkerPrefix;
		if (ConnectionInfo.DoorType == ESnapConnectionDoorType::LockedDoor) {
			ConnectionActor->ConnectionComponent->MarkerName = ConnectionInfo.CustomMarkerName;
		}
		ConnectionActor->BuildConnectionInstance(nullptr, ConnectionInfo.ModuleA, ConnectionInfo.ModuleB,  PersistentLevel);
		for (AActor* SpawnedDoorActor : ConnectionActor->GetSpawnedInstances()) {
			SetupSpawnedActor(SpawnedDoorActor);
		}
		SetupSpawnedActor(ConnectionActor);
	} 

	// Build the walls
	for (const FSnapWallInstance& ConnectionInfo : Model->Walls) {
		FTransform ConnectionTransform = GetWorldToTargetTransform(ConnectionInfo.WorldTransform);
		ASnapConnectionActor* ConnectionActor = World->SpawnActor<ASnapConnectionActor>(ASnapConnectionActor::StaticClass(), ConnectionTransform);
		ConnectionActor->ConnectionComponent->ConnectionInfo = ConnectionInfo.ConnectionInfo.Get();
		ConnectionActor->ConnectionComponent->ConnectionState = ESnapConnectionState::Wall;
		ConnectionActor->ConnectionComponent->DoorType = ESnapConnectionDoorType::NotApplicable;
		ConnectionActor->ConnectionComponent->MarkerPrefix = ConnectionMarkerPrefix;
		ConnectionActor->BuildConnectionInstance(nullptr, ConnectionInfo.ModuleId, {}, PersistentLevel);
		for (AActor* SpawnedDoorActor : ConnectionActor->GetSpawnedInstances()) {
			SetupSpawnedActor(SpawnedDoorActor);
		}
		SetupSpawnedActor(ConnectionActor);
	}
	
	return true;
}

