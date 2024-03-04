// Copyright 2022 PufStudio. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
// #include "MeshAttributes.h"
#include "TiledLevelAsset.h"
#include "TiledLevelSettings.h"
#include "TiledLevelUtility.h"
#include "GameFramework/Actor.h"
#include "TiledLevel.generated.h"

DECLARE_DELEGATE(FPreSaveTiledLevelActor)

struct FTilePlacement;
struct FTiledLevelGameData;
class UTiledLevelItem;
class UHierarchicalInstancedStaticMeshComponent;



UCLASS(BlueprintType)
class TILEDLEVELRUNTIME_API ATiledLevel : public AActor
{
	GENERATED_BODY()

public:

	// Sets default values for this actor's properties
	ATiledLevel();

	// TODO: leave all replication features to next update...
	// void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void BeginPlay() override;


#if WITH_EDITOR
	
	UFUNCTION(BlueprintCallable, Category="TiledLevel")
	void SetupInstancedLevel(class UTiledItemSet* SourceItemSet, FIntVector NumTiles);
	
	UFUNCTION(BlueprintCallable, Category="TiledLevel")
	void SetupAssetLevel(class UTiledLevelAsset* SourceAsset);
	
	virtual void PostDuplicate(bool bDuplicateForPIE) override;
	
	virtual void PostInitProperties() override;

	void OnPostWorldInitialization(UWorld* World, const UWorld::InitializationValues IVS);
	
	virtual void PreSave(FObjectPreSaveContext ObjectSaveContext) override;

#endif	
	virtual void Destroyed() override;
	
	UPROPERTY()
	TObjectPtr<USceneComponent> Root;

	// TODO: with this setup, there is no way to handle same SMptr in different ItemSet!?
	UPROPERTY()
	TMap<TObjectPtr<UStaticMesh> , TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> TiledObjectSpawner;
	
	UPROPERTY()
	TArray<TObjectPtr<AActor>> SpawnedTiledActors;

	void SetActiveAsset(UTiledLevelAsset* NewAsset);
	UTiledLevelAsset* GetAsset() const;
	void RemoveAsset();
	template <typename T>
	void PopulateSinglePlacement(const T& Placement, bool bForceBlockCustomData = false);
	template <typename T>
	void RemovePlacements(const TArray<T>& PlacementsToDelete);
	void RemoveInstances(const TMap<UStaticMesh*, TArray<int32>>& TargetInstancesData);
	void DestroyTiledActorByPlacement(const FTilePlacement& Placement);
	void DestroyTiledActorByPlacement(const FEdgePlacement& Placement);
	void DestroyTiledActorByPlacement(const FPointPlacement& Placement);
	int EraseItem(FIntVector Pos, FIntVector Extent, bool bIsFloor = false, bool Both = false);
	int EraseItem(FTiledLevelEdge Edge, FIntVector Extent, bool bIsEdge = false, bool Both = false);
	int EraseItem(FIntVector Pos, int ZExtent, bool bIsPoint = false, bool Both = false);
	int EraseItem_Any(FIntVector Pos, FIntVector Extent);
	// for quick eraser that only focus on specific item
	int EraseSingleItem(FIntVector Pos, FIntVector Extent, FGuid TargetID);
	int EraseSingleItem(FTiledLevelEdge Edge, FIntVector Extent, FGuid TargetID);
	int EraseSingleItem(FIntVector Pos, int ZExtent, FGuid TargetID);
	void ResetAllInstance(bool IgnoreVersion = false, bool bIgnoreCustomData = false);
	void ResetAllInstanceFromData();
	// TODO: separated spawner? better control for anything... need to rewrite so many things?
	// TODO: better reset algorithm? just remove adjacent tiles and recalculate the stuff??

	void MakeEditable();

	UPROPERTY()
	FTiledLevelGameData GametimeData;

#if WITH_EDITOR	
	// break into separate static mesh and actors, but organized properly
	UFUNCTION()
	void Break();
#endif

	// For game time supports
	FBox GetBoundaryBox();
	FTiledLevelGameData MakeGametimeData();
	
	UPROPERTY()
	bool IsInstance;
	
	bool IsInTiledLevelEditor = false;

	FPreSaveTiledLevelActor PreSaveTiledLevelActor;
	
	uint32 VersionNumber;

private:

	UPROPERTY()
	TObjectPtr<UTiledLevelAsset> ActiveAsset;

	// so silly to use raw points... to check things...
	TArray<FGuid> GetEraserActiveItemIDs() const;
	void CreateNewHISM(UTiledLevelItem* SourceItem);
	void UpdateHISM_Properties(UHierarchicalInstancedStaticMeshComponent* InHISM, UTiledLevelItem* InItem);
	AActor* SpawnActorPlacement(const FItemPlacement& ItemPlacement);
	AActor* SpawnMirroredPlacement(const FItemPlacement& ItemPlacement);

private:
#if WITH_EDITOR
	FDelegateHandle OnPostWorldInitializationDelegateHandle;
#endif

};


// template implementations...

/*
 * For actor item: use tags to store position, extent, and guid. WARNING: this may block users to use the last 3 tags...
 * For mesh item: Use custom data to store position and extent information XXX (crash in UE5.X)
 *					-> Use AdditionalData to store its info...
 */
template <typename T>
void ATiledLevel::PopulateSinglePlacement(const T& Placement, bool bForceBlockCustomData)
{
	if (Placement.GetItem()->SourceType == ETLSourceType::Actor)
	{
		if (!IsValid(Placement.GetItem()->TiledActor)) return;
		if (AActor* NewActor = SpawnActorPlacement(Placement))
		{
			FTiledLevelUtility::SetSpawnedActorTag(Placement, NewActor);
			SpawnedTiledActors.Add(NewActor);
		}
	}
	else if (Placement.IsMirrored)
	{
		AActor* NewMirrored = SpawnMirroredPlacement(Placement);
		FTiledLevelUtility::SetSpawnedActorTag(Placement, NewMirrored);
		SpawnedTiledActors.Add(NewMirrored);
	} else
	{
		if (!Placement.GetItem()->TiledMesh) return;
		/* 
		 * BUG: Set custom data at editor startup will crash in UE5.0 and UE5.1 !!?? (sometimes it won't?)
		 * Fixed by totally remove the use of custom data...???
		 */
		CreateNewHISM(Placement.GetItem());
		const int NewIndex = TiledObjectSpawner[Placement.GetItem()->TiledMesh]->AddInstance(Placement.TileObjectTransform);
		const TArray<float> InstanceData = FTiledLevelUtility::ConvertPlacementToHISM_CustomData(Placement);
		if (!GetMutableDefault<UTiledLevelSettings>()->bBlockAddCustomData && !bForceBlockCustomData)
			TiledObjectSpawner[Placement.GetItem()->TiledMesh]->SetCustomData(NewIndex, InstanceData);
	}	
}

template <typename T>
void ATiledLevel::RemovePlacements(const TArray<T>& PlacementsToDelete)
{
	TMap<UStaticMesh*, TArray<int32>> TargetInstanceData;
	for (auto P : PlacementsToDelete)
	{
		if (P.GetItem()->SourceType == ETLSourceType::Actor || P.IsMirrored)
		{
			DestroyTiledActorByPlacement(P);
		}
		else
		{
			if (!TargetInstanceData.Contains(P.GetItem()->TiledMesh))
				TargetInstanceData.Add(P.GetItem()->TiledMesh, TArray<int32>{});
			TArray<float> TargetInfo = FTiledLevelUtility::ConvertPlacementToHISM_CustomData(P);
			FTiledLevelUtility::FindInstanceIndexByPlacement(TargetInstanceData[P.GetItem()->TiledMesh],
				TiledObjectSpawner[P.GetItem()->TiledMesh]->PerInstanceSMCustomData, TargetInfo);
		}
	}
	ActiveAsset->RemovePlacements(PlacementsToDelete);
	RemoveInstances(TargetInstanceData);
}

