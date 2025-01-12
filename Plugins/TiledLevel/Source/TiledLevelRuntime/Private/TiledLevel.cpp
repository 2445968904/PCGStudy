﻿// Copyright 2022 PufStudio. All Rights Reserved.

#include "TiledLevel.h"
#include "TiledItemSet.h"
#include "TiledLevelAsset.h"
#include "TiledLevelEditorLog.h"
#include "TiledLevelItem.h"
#include "TiledLevelTypes.h"
#include "TiledLevelUtility.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "ProceduralMeshComponent.h"
#include "TiledLevelRestrictionHelper.h"
#include "UObject/ObjectSaveContext.h"
#include "Engine/Blueprint.h"
#include "Engine/World.h"

#define LOCTEXT_NAMESPACE "TiledLevel"

// Sets default values
ATiledLevel::ATiledLevel()
{
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	Root->Mobility = EComponentMobility::Movable;
	// bReplicates = true;
}

// void ATiledLevel::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
// {
// 	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
//
// 	DOREPLIFETIME(ATiledLevel, GametimeData);
// }

void ATiledLevel::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
}

void ATiledLevel::BeginPlay()
{
	Super::BeginPlay();
	// if (!UKismetSystemLibrary::IsServer(GetWorld()) && GetLocalRole() == ENetRole::ROLE_Authority)
	// {
	// 	GetRootComponent()->SetVisibility(false, true);
	// }
}

#if WITH_EDITOR
void ATiledLevel::SetupInstancedLevel(UTiledItemSet* SourceItemSet, FIntVector NumTiles)
{
	UTiledLevelAsset* CreatedLevelAsset = NewObject<UTiledLevelAsset>(this, UTiledLevelAsset::StaticClass(), NAME_None ,RF_Transactional);
	CreatedLevelAsset->SetActiveItemSet(SourceItemSet);
	CreatedLevelAsset->SetTileSize(SourceItemSet->GetTileSize());
	CreatedLevelAsset->X_Num = NumTiles.X;
	CreatedLevelAsset->Y_Num = NumTiles.Y;
	for (int f = 0; f < NumTiles.Z; f++)
		CreatedLevelAsset->AddNewFloor(0);
	CreatedLevelAsset->ConfirmTileSize();
	SetActiveAsset(CreatedLevelAsset);
	IsInstance = true;
}

void ATiledLevel::SetupAssetLevel(UTiledLevelAsset* SourceAsset)
{
	SetActiveAsset(SourceAsset);
	ResetAllInstance(true);
}


void ATiledLevel::PostDuplicate(bool bDuplicateForPIE)
{
	// need to reset this and where it is duplicated from...
	// I can not figure out how to get actor where this one duplicated from...
	// use regex to do it as possible... so far... reset all with same base label...
	// strip the actor label since duplicated one will have almost the same actor label...

	const FRegexPattern Pattern(TEXT("^(.*\\D)"));
	if (FRegexMatcher Matcher = FRegexMatcher(Pattern, GetActorLabel()); Matcher.FindNext())
	{
		const FString BaseLabel = Matcher.GetCaptureGroup(0);
		TArray<AActor*> Out;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), StaticClass(), Out);
		for (AActor* A : Out)
		{
			if (A->GetActorLabel().Contains(BaseLabel))
			{
				Cast<ATiledLevel>(A)->ResetAllInstance(true);
			}
		}
	}
	Super::PostDuplicate(bDuplicateForPIE);
}

void ATiledLevel::PostInitProperties()
{
	Super::PostInitProperties();
	FWorldDelegates::OnPostWorldInitialization.Remove(OnPostWorldInitializationDelegateHandle);
	OnPostWorldInitializationDelegateHandle.Reset();
	OnPostWorldInitializationDelegateHandle = FWorldDelegates::OnPostWorldInitialization.AddUObject(this, &ATiledLevel::OnPostWorldInitialization);
}

// make TLA syn with its asset... even when changing map...
void ATiledLevel::OnPostWorldInitialization(UWorld* World, const UWorld::InitializationValues IVS)
{
	if (World->WorldType == EWorldType::PIE)
	{
		return;
	}
	if (World->WorldType == EWorldType::Editor && !IsInstance)
	{
		ResetAllInstance(true, true);
	}
}

void ATiledLevel::PreSave(FObjectPreSaveContext ObjectSaveContext)
{
	Super::PreSave(ObjectSaveContext);
	PreSaveTiledLevelActor.ExecuteIfBound();
}

#endif

void ATiledLevel::Destroyed()
{
	TArray<AActor*> AttachedActors; // helpers and spawned actors
	GetAttachedActors(AttachedActors);
	for (AActor* Attached : AttachedActors)
		Attached->Destroy();
	Super::Destroyed();
}

void ATiledLevel::SetActiveAsset(UTiledLevelAsset* NewAsset)
{
	ActiveAsset = NewAsset;
}

UTiledLevelAsset* ATiledLevel::GetAsset() const
{
	return ActiveAsset;
}

void ATiledLevel::RemoveAsset()
{
	ActiveAsset = nullptr;
	for (AActor* SpawnedActor : SpawnedTiledActors)
	{
		if (SpawnedActor)
			SpawnedActor->Destroy();
	}
	SpawnedTiledActors.Empty();
	TiledObjectSpawner.Empty();
    
}

void ATiledLevel::RemoveInstances(const TMap<UStaticMesh*, TArray<int32>>& TargetInstancesData)
{

	TArray<UStaticMesh*> KeysToDelete;
	for (auto& elem : TargetInstancesData)
	{
		TiledObjectSpawner[elem.Key]->RemoveInstances(elem.Value);
		
		if (TiledObjectSpawner[elem.Key]->GetInstanceCount() == 0)
		 	KeysToDelete.Add(elem.Key);
	}
	for (const UStaticMesh* MeshPtr : KeysToDelete)
	{
		TiledObjectSpawner[MeshPtr]->DestroyComponent();
		TiledObjectSpawner.Remove(MeshPtr);
	}
}

void ATiledLevel::DestroyTiledActorByPlacement(const FTilePlacement& Placement)
{
	AActor* ActorToRemove = nullptr;
	if (UTiledLevelRestrictionItem* RestrictionItem = Cast<UTiledLevelRestrictionItem>(Placement.GetItem()))
	{
		for (AActor* A: SpawnedTiledActors)
		{
			if (ATiledLevelRestrictionHelper* Arh = Cast<ATiledLevelRestrictionHelper>(A))
			{
				if (Arh->GetSourceItem() == RestrictionItem)
				{
					Arh->RemoveTargetedPositions(Placement.GridPosition);
					if (Arh->TargetedTilePositions.Num() > 0)
						return;
					ActorToRemove = Arh;
					break;
				}
			}
		}
	}
	else
	{
		FVector ActorTilePosition;
		FVector ActorTileExtent;
		for (AActor* SpawnedActor : SpawnedTiledActors)
		{
			const int NTags = SpawnedActor->Tags.Num();
			ActorTilePosition.InitFromString(SpawnedActor->Tags[NTags - 3].ToString());
			ActorTileExtent.InitFromString(SpawnedActor->Tags[NTags - 2].ToString());
			if (FVector(Placement.GridPosition) == ActorTilePosition && FVector(Placement.Extent) == ActorTileExtent &&
				SpawnedActor->Tags[NTags - 1] == FName(Placement.ItemID.ToString()))
			{
				ActorToRemove = SpawnedActor;
				break;
			}
		}
	}
	if (ActorToRemove)
	{
		ActorToRemove->Destroy();
		SpawnedTiledActors.Remove(ActorToRemove);
	}
}

void ATiledLevel::DestroyTiledActorByPlacement( const FEdgePlacement& Placement)
{
	FVector ActorTilePosition;
	FVector ActorEdgeExtraInfo;
	AActor* ActorToRemove = nullptr;
	for (AActor* SpawnedActor : SpawnedTiledActors)
	{
		const int NTags = SpawnedActor->Tags.Num();
		FVector EdgeExtraInfo = FVector(Placement.GetItem()->Extent.X, Placement.GetItem()->Extent.Z, Placement.Edge.EdgeType == EEdgeType::Horizontal? -1 : 0);
		ActorTilePosition.InitFromString(SpawnedActor->Tags[NTags - 3].ToString());
		ActorEdgeExtraInfo.InitFromString(SpawnedActor->Tags[NTags - 2].ToString());
		if (Placement.GetEdgePosition() == ActorTilePosition && EdgeExtraInfo == ActorEdgeExtraInfo && SpawnedActor->Tags[NTags - 1] == FName(Placement.ItemID.ToString()))
		{
			ActorToRemove = SpawnedActor;
			break;
		}
	}
	if (ActorToRemove)
	{
		ActorToRemove->Destroy();
		SpawnedTiledActors.Remove(ActorToRemove);
	}
}

void ATiledLevel::DestroyTiledActorByPlacement(const FPointPlacement& Placement)
{
	FVector ActorTilePosition;
	FVector ActorTileExtent;
	AActor* ActorToRemove = nullptr;
	for (AActor* SpawnedActor : SpawnedTiledActors)
	{
		const int NTags = SpawnedActor->Tags.Num();
		ActorTilePosition.InitFromString(SpawnedActor->Tags[NTags - 3].ToString());
		ActorTileExtent.InitFromString(SpawnedActor->Tags[NTags - 2].ToString());
		if (FVector(Placement.GridPosition) == ActorTilePosition && SpawnedActor->Tags[NTags - 1] == FName(Placement.ItemID.ToString()))
		{
			ActorToRemove = SpawnedActor;
			break;
		}
	}
	if (ActorToRemove)
	{
		ActorToRemove->Destroy();
		SpawnedTiledActors.Remove(ActorToRemove);
	}
}


int ATiledLevel::EraseItem(FIntVector Pos, FIntVector Extent, bool bIsFloor, bool Both)
{
	FTilePlacement TestPlacement;
	TestPlacement.GridPosition = Pos;
	TestPlacement.Extent = Extent;
	TMap<UStaticMesh*, TArray<int32>> TargetInstanceData;
	TArray<FTilePlacement> TilesToDelete;
	TArray<FTilePlacement> TargetPlacements;
	if (Both)
	{
		TargetPlacements.Append(ActiveAsset->GetAllBlockPlacements());
		TargetPlacements.Append(ActiveAsset->GetAllFloorPlacements());
	} else
	{
		if (bIsFloor)
			TargetPlacements = ActiveAsset->GetAllFloorPlacements();
		else
			TargetPlacements = ActiveAsset->GetAllBlockPlacements();
	}
	for (FTilePlacement& Placement : TargetPlacements)
	{
		if (!FTiledLevelUtility::IsTilePlacementOverlapping(TestPlacement, Placement) ||
			!GetEraserActiveItemIDs().Contains(Placement.GetItem()->ItemID))
				continue;
		TilesToDelete.Add(Placement);
		// if that placement is in hidden floor, don't remove it's instance, just remove data only...
		if (!ActiveAsset->GetFloorFromPosition(Placement.GridPosition.Z)->ShouldRenderInEditor)
			continue;
		if (Placement.GetItem()->SourceType == ETLSourceType::Actor || Placement.IsMirrored)
		{
			DestroyTiledActorByPlacement(Placement);
		}
		else
		{
			 UStaticMesh* TiledMesh = Placement.GetItem()->TiledMesh;
			 if (!TargetInstanceData.Contains(TiledMesh))				
				  TargetInstanceData.Add(TiledMesh, TArray<int32>{});
			 TArray<float> TargetInfo = FTiledLevelUtility::ConvertPlacementToHISM_CustomData(Placement);
			 FTiledLevelUtility::FindInstanceIndexByPlacement(TargetInstanceData[TiledMesh], TiledObjectSpawner[TiledMesh]->PerInstanceSMCustomData, TargetInfo);
		}
	}
	ActiveAsset->RemovePlacements(TilesToDelete);
	RemoveInstances(TargetInstanceData);
	return TilesToDelete.Num();
}

int ATiledLevel::EraseItem(FTiledLevelEdge Edge, FIntVector Extent, bool bIsEdge, bool Both)
{
	TArray<FEdgePlacement> EdgesToDelete;
	TMap<UStaticMesh*, TArray<int32>> TargetInstanceData;
	TArray<FEdgePlacement> TargetPlacements;
	if (Both)
	{
		TargetPlacements.Append(ActiveAsset->GetAllWallPlacements());
		TargetPlacements.Append(ActiveAsset->GetAllEdgePlacements());
	} else
	{
		if (bIsEdge)
			TargetPlacements = ActiveAsset->GetAllEdgePlacements();
		else
			TargetPlacements = ActiveAsset->GetAllWallPlacements();
	}

	for (FEdgePlacement& Placement : TargetPlacements)
	{
		if (!FTiledLevelUtility::IsEdgeOverlapping(Edge, FVector(Extent), Placement.Edge, Placement.GetItem()->Extent)
			|| !GetEraserActiveItemIDs().Contains(Placement.GetItem()->ItemID))
			continue;
		EdgesToDelete.Add(Placement);
		// if that placement is in hidden floor, don't remove it's instance, just remove data only...
		if (!ActiveAsset->GetFloorFromPosition(Placement.Edge.Z)->ShouldRenderInEditor)
			continue;
		UTiledLevelItem* Item = Placement.GetItem();
		if (Item->SourceType == ETLSourceType::Actor || Placement.IsMirrored)
		{
			DestroyTiledActorByPlacement(Placement);
		} else
		{
			 if (!TargetInstanceData.Contains(Item->TiledMesh))				
				  TargetInstanceData.Add(Item->TiledMesh, TArray<int32>{});
			 TArray<float> TargetInfo = FTiledLevelUtility::ConvertPlacementToHISM_CustomData(Placement);
			 FTiledLevelUtility::FindInstanceIndexByPlacement(TargetInstanceData[Item->TiledMesh],TiledObjectSpawner[Item->TiledMesh]->PerInstanceSMCustomData, TargetInfo);
		}
	}
	ActiveAsset->RemovePlacements(EdgesToDelete);
	RemoveInstances(TargetInstanceData);
	return EdgesToDelete.Num();
}

int ATiledLevel::EraseItem(FIntVector Pos, int ZExtent, bool bIsPoint, bool Both)
{
	FPointPlacement TestPlacement;
	TestPlacement.GridPosition = Pos;
	TMap<UStaticMesh*, TArray<int32>> TargetInstanceData;
	TArray<FPointPlacement> PointsToDelete;
	TArray<FPointPlacement> TargetPlacements;
	if (Both)
	{
		TargetPlacements.Append(ActiveAsset->GetAllPillarPlacements());
		TargetPlacements.Append(ActiveAsset->GetAllPointPlacements());
	} else
	{
		if (bIsPoint)
			TargetPlacements = ActiveAsset->GetAllPointPlacements();
		else
			TargetPlacements = ActiveAsset->GetAllPillarPlacements();
	}

	for (FPointPlacement& Placement : TargetPlacements)
	{
		if (!FTiledLevelUtility::IsPointPlacementOverlapping(TestPlacement, ZExtent, Placement, Placement.GetItem()->Extent.Z) ||
			!GetEraserActiveItemIDs().Contains(Placement.GetItem()->ItemID))
			continue;
		// if that placement is in hidden floor, don't remove it's instance, just remove data only...
		if (!ActiveAsset->GetFloorFromPosition(Placement.GridPosition.Z)->ShouldRenderInEditor)
			continue;
		PointsToDelete.Add(Placement);
		if (Placement.GetItem()->SourceType == ETLSourceType::Actor || Placement.IsMirrored)
		{
			DestroyTiledActorByPlacement(Placement);
		}
		else
		{
			 UStaticMesh* TiledMesh = Placement.GetItem()->TiledMesh;
			 if (!TargetInstanceData.Contains(TiledMesh))				
				  TargetInstanceData.Add(TiledMesh, TArray<int32>{});
			 TArray<float> TargetInfo = FTiledLevelUtility::ConvertPlacementToHISM_CustomData(Placement);
			 FTiledLevelUtility::FindInstanceIndexByPlacement(TargetInstanceData[TiledMesh],TiledObjectSpawner[TiledMesh]->PerInstanceSMCustomData, TargetInfo);
		}
	}
	ActiveAsset->RemovePlacements(PointsToDelete);
	RemoveInstances(TargetInstanceData);
	return PointsToDelete.Num();
}

int ATiledLevel::EraseItem_Any(FIntVector Pos, FIntVector Extent)
{
	FTilePlacement TestPlacement;
	TestPlacement.GridPosition = Pos;
	TestPlacement.Extent = Extent;
	TMap<UStaticMesh*, TArray<int32>> TargetInstanceData;
	TArray<FTilePlacement> TileToDelete;
	TArray<FTilePlacement> TargetTilePlacements;
	TargetTilePlacements.Append(ActiveAsset->GetAllBlockPlacements());
	TargetTilePlacements.Append(ActiveAsset->GetAllFloorPlacements());
	TArray<FEdgePlacement> WallToDelete;
	TArray<FEdgePlacement> TargetWallPlacements;
	TargetWallPlacements.Append(ActiveAsset->GetAllWallPlacements());
	TargetWallPlacements.Append(ActiveAsset->GetAllEdgePlacements());
	TArray<FPointPlacement> PointToDelete;
	TArray<FPointPlacement>  TargetPointPlacements;
	TargetPointPlacements.Append(ActiveAsset->GetAllPillarPlacements());
	TargetPointPlacements.Append(ActiveAsset->GetAllPointPlacements());
	
	for (FTilePlacement& Placement : TargetTilePlacements)
	{
		if (!FTiledLevelUtility::IsTilePlacementOverlapping(TestPlacement, Placement) || !GetEraserActiveItemIDs().Contains(Placement.GetItem()->ItemID))
			continue;
		if (Placement.GetItem()->SourceType == ETLSourceType::Actor || Placement.IsMirrored)
		{
			DestroyTiledActorByPlacement(Placement);
		} else
		{
			 UStaticMesh* TiledMesh = Placement.GetItem()->TiledMesh;
			 if (!TargetInstanceData.Contains(TiledMesh))				
				  TargetInstanceData.Add(TiledMesh, TArray<int32>{});
			 TArray<float> TargetInfo = FTiledLevelUtility::ConvertPlacementToHISM_CustomData(Placement);
			 FTiledLevelUtility::FindInstanceIndexByPlacement(TargetInstanceData[TiledMesh],TiledObjectSpawner[TiledMesh]->PerInstanceSMCustomData, TargetInfo);
		}
		TileToDelete.Add(Placement);
	}

	for (FEdgePlacement& Placement : TargetWallPlacements)
	{
		if (!FTiledLevelUtility::IsEdgeInsideTile(Placement.Edge, FIntVector(Placement.GetItem()->Extent), Pos, Extent)
			|| !GetEraserActiveItemIDs().Contains(Placement.GetItem()->ItemID))
			continue;
		UTiledLevelItem* Item = Placement.GetItem();
		if (Item->SourceType == ETLSourceType::Actor || Placement.IsMirrored)
		{
			DestroyTiledActorByPlacement(Placement);
		} else
		{
			  if (!TargetInstanceData.Contains(Item->TiledMesh))				
				  TargetInstanceData.Add(Item->TiledMesh, TArray<int32>{});
			  TArray<float> TargetInfo = FTiledLevelUtility::ConvertPlacementToHISM_CustomData(Placement);
			  FTiledLevelUtility::FindInstanceIndexByPlacement(TargetInstanceData[Item->TiledMesh],TiledObjectSpawner[Item->TiledMesh]->PerInstanceSMCustomData, TargetInfo);
		}
		WallToDelete.Add(Placement);
	}
	
	for (FPointPlacement& Placement : TargetPointPlacements)
	{
		if (!FTiledLevelUtility::IsPointInsideTile(Placement.GridPosition, Placement.GetItem()->Extent.Z, Pos, Extent) ||
			!GetEraserActiveItemIDs().Contains(Placement.GetItem()->ItemID))
			continue;
		if (Placement.GetItem()->SourceType == ETLSourceType::Actor || Placement.IsMirrored)
		{
			DestroyTiledActorByPlacement(Placement);
		} else
		{
			 UStaticMesh* TiledMesh = Placement.GetItem()->TiledMesh;
			 if (!TargetInstanceData.Contains(TiledMesh))				
				  TargetInstanceData.Add(TiledMesh, TArray<int32>{});
			 TArray<float> TargetInfo = FTiledLevelUtility::ConvertPlacementToHISM_CustomData(Placement);
			 FTiledLevelUtility::FindInstanceIndexByPlacement(TargetInstanceData[TiledMesh],TiledObjectSpawner[TiledMesh]->PerInstanceSMCustomData, TargetInfo);
		}
		PointToDelete.Add(Placement);
	}
	
	ActiveAsset->RemovePlacements(TileToDelete);
	ActiveAsset->RemovePlacements(WallToDelete);
	ActiveAsset->RemovePlacements(PointToDelete);
	RemoveInstances(TargetInstanceData);

	return TargetInstanceData.Num();
}

int ATiledLevel::EraseSingleItem(FIntVector Pos, FIntVector Extent, FGuid TargetID)
{
	FTilePlacement TestPlacement;
	TestPlacement.GridPosition = Pos;
	TestPlacement.Extent = Extent;
	TMap<UStaticMesh*, TArray<int32>> TargetInstanceData;
	TArray<int32> ActorIndicesToRemove;
	TArray<FTilePlacement> TilesToDelete;
	UTiledLevelItem* Item = GetAsset()->GetItemSetAsset()->GetItem(TargetID);
	for (int L = Pos.Z ; L < Pos.Z + Extent.Z; L ++)
	{
		FTiledFloor* CheckFloor = ActiveAsset->GetFloorFromPosition(L);
		if (!CheckFloor) break;
		TArray<FTilePlacement> CheckPlacements;
		if (Item->PlacedType == EPlacedType::Block)
			CheckPlacements = CheckFloor->BlockPlacements;
		else
			CheckPlacements = CheckFloor->FloorPlacements;
		for (FTilePlacement& Placement : CheckPlacements)
		{
			if (!FTiledLevelUtility::IsTilePlacementOverlapping(TestPlacement, Placement) || Placement.ItemID != TargetID)
				continue;
			if (Item->SourceType == ETLSourceType::Actor || Placement.IsMirrored)
			{
				DestroyTiledActorByPlacement(Placement);
			} else
			{
				 // find placement instance ID, and its mesh
				 if (!TargetInstanceData.Contains(Item->TiledMesh))				
					  TargetInstanceData.Add(Item->TiledMesh, TArray<int32>{});
				 TArray<float> TargetInfo = FTiledLevelUtility::ConvertPlacementToHISM_CustomData(Placement);
				 FTiledLevelUtility::FindInstanceIndexByPlacement(TargetInstanceData[Item->TiledMesh],TiledObjectSpawner[Item->TiledMesh]->PerInstanceSMCustomData, TargetInfo);
			}
			TilesToDelete.Add(Placement);
		}
	}
	ActiveAsset->RemovePlacements(TilesToDelete);
	if (Item->SourceType == ETLSourceType::Mesh)
		RemoveInstances(TargetInstanceData);
	return TilesToDelete.Num();
}

int ATiledLevel::EraseSingleItem(FTiledLevelEdge Edge, FIntVector Extent, FGuid TargetID)
{
	TArray<FEdgePlacement> WallsToDelete;
	TMap<UStaticMesh*, TArray<int32>> TargetInstanceData;
	UTiledLevelItem* Item = ActiveAsset->GetItemSetAsset()->GetItem(TargetID);
	for (int L = Edge.Z ; L < Edge.Z + Extent.Z; L ++)
	{
		FTiledFloor* CheckFloor = ActiveAsset->GetFloorFromPosition(L);
		if (!CheckFloor) break;
		TArray<FEdgePlacement> CheckPlacements;
		if (Item->PlacedType == EPlacedType::Wall)
			CheckPlacements = CheckFloor->WallPlacements;
		else
			CheckPlacements = CheckFloor->EdgePlacements;
		for (FEdgePlacement& Placement : CheckPlacements)
		{
			if (!FTiledLevelUtility::IsEdgeOverlapping(Edge, FVector(Extent), Placement.Edge, Placement.GetItem()->Extent)
				|| Placement.ItemID != TargetID)
				continue;
			if (Item->SourceType == ETLSourceType::Actor || Placement.IsMirrored)
			{
				DestroyTiledActorByPlacement(Placement);
			} else
			{
				 if (!TargetInstanceData.Contains(Item->TiledMesh))				
					  TargetInstanceData.Add(Item->TiledMesh, TArray<int32>{});
				 TArray<float> TargetInfo = FTiledLevelUtility::ConvertPlacementToHISM_CustomData(Placement);
				 FTiledLevelUtility::FindInstanceIndexByPlacement(TargetInstanceData[Item->TiledMesh],TiledObjectSpawner[Item->TiledMesh]->PerInstanceSMCustomData, TargetInfo);
			}
			WallsToDelete.Add(Placement);
		}
	}
	
	ActiveAsset->RemovePlacements(WallsToDelete);
	if (Item->SourceType == ETLSourceType::Mesh)
		RemoveInstances(TargetInstanceData);
	return WallsToDelete.Num();
}

int ATiledLevel::EraseSingleItem(FIntVector Pos, int ZExtent, FGuid TargetID)
{
	FPointPlacement TestPlacement;
	TestPlacement.GridPosition = Pos;
	TMap<UStaticMesh*, TArray<int32>> TargetInstanceData;
	TArray<int32> ActorIndicesToRemove;
	TArray<FPointPlacement> PointsToDelete;
	UTiledLevelItem* Item = GetAsset()->GetItemSetAsset()->GetItem(TargetID);
	for (int L = Pos.Z ; L < Pos.Z + ZExtent; L ++)
	{
		FTiledFloor* CheckFloor = ActiveAsset->GetFloorFromPosition(L);
		if (!CheckFloor) break;
		TArray<FPointPlacement> CheckPlacements;
		if (Item->PlacedType == EPlacedType::Pillar)
			CheckPlacements = CheckFloor->PillarPlacements;
		else
			CheckPlacements = CheckFloor->PointPlacements;
		for (FPointPlacement& Placement : CheckPlacements)
		{
			if (!FTiledLevelUtility::IsPointPlacementOverlapping(TestPlacement, ZExtent, Placement, Placement.GetItem()->Extent.Z) ||
				Placement.ItemID != TargetID)
				continue;
			if (Item->SourceType == ETLSourceType::Actor || Placement.IsMirrored)
			{
				DestroyTiledActorByPlacement(Placement);
			} else
			{
				 // find placement instance ID, and its mesh
				 if (!TargetInstanceData.Contains(Item->TiledMesh))				
					  TargetInstanceData.Add(Item->TiledMesh, TArray<int32>{});
				 TArray<float> TargetInfo = FTiledLevelUtility::ConvertPlacementToHISM_CustomData(Placement);
				 FTiledLevelUtility::FindInstanceIndexByPlacement(TargetInstanceData[Item->TiledMesh],TiledObjectSpawner[Item->TiledMesh]->PerInstanceSMCustomData, TargetInfo);
			}
			PointsToDelete.Add(Placement);
		}
	}
	ActiveAsset->RemovePlacements(PointsToDelete);
	if (Item->SourceType == ETLSourceType::Mesh)
		RemoveInstances(TargetInstanceData);
	return PointsToDelete.Num();
}

void ATiledLevel::ResetAllInstance(bool IgnoreVersion, bool bIgnoreCustomData)
{
	if (!ActiveAsset) return;
	if (!IgnoreVersion)
	{
		if (ActiveAsset->VersionNumber <= VersionNumber) return;
	}

	VersionNumber = ActiveAsset->VersionNumber;
	for (AActor* SpawnedActor : SpawnedTiledActors)
	{
		if (SpawnedActor)
			SpawnedActor->Destroy();
	}
	SpawnedTiledActors.Empty();
	
	for (auto& elem : TiledObjectSpawner)
	{
		if (elem.Value)
		{
			elem.Value->ClearInstances();
		}
	}

	TiledObjectSpawner.Empty();
	ActiveAsset->ClearInvalidPlacements();

	for (FTiledFloor& F : ActiveAsset->TiledFloors)
	{
		if (!F.ShouldRenderInEditor) continue;
		if (ActiveAsset->ViewAsAutoPaint)
		{
			for (const FTilePlacement& P : F.AutoPaintGenTiles)
				PopulateSinglePlacement(P, bIgnoreCustomData);
			for (const FEdgePlacement& P : F.AutoPaintGenEdges)
				PopulateSinglePlacement(P, bIgnoreCustomData);
			for (const FPointPlacement& P : F.AutoPaintGenPoints)
				PopulateSinglePlacement(P, bIgnoreCustomData);
		}
		else
		{
			for (FTilePlacement& Placement : F.BlockPlacements)
				PopulateSinglePlacement(Placement, bIgnoreCustomData);
			for (FTilePlacement& Placement : F.FloorPlacements)
				PopulateSinglePlacement(Placement, bIgnoreCustomData);
			for (FPointPlacement& Placement : F.PillarPlacements)
				PopulateSinglePlacement(Placement, bIgnoreCustomData);
			for (FEdgePlacement& Placement : F.WallPlacements)
				PopulateSinglePlacement(Placement, bIgnoreCustomData);
			for (FEdgePlacement& Placement : F.EdgePlacements)
				PopulateSinglePlacement(Placement, bIgnoreCustomData);
			for (FPointPlacement& Placement : F.PointPlacements)
				PopulateSinglePlacement(Placement, bIgnoreCustomData);
		}
	}
 
	for (AActor* Actor : SpawnedTiledActors)
	{
		Actor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
	}
}

void ATiledLevel::ResetAllInstanceFromData()
{
	for (AActor* SpawnedActor : SpawnedTiledActors)
	{
		if (SpawnedActor)
			SpawnedActor->Destroy();
	}
	SpawnedTiledActors.Empty();
	for (auto& elem : TiledObjectSpawner)
	{
		if (elem.Value)
		{
			elem.Value->ClearInstances();
		}
	}

	TiledObjectSpawner.Empty();
	
	for (FTilePlacement& Placement : GametimeData.BlockPlacements)
	{
		if (GametimeData.HiddenFloors.Contains(Placement.GridPosition.Z)) continue;
	    PopulateSinglePlacement(Placement);
	}
	for (FTilePlacement& Placement : GametimeData.FloorPlacements)
	{
		if (GametimeData.HiddenFloors.Contains(Placement.GridPosition.Z)) continue;
		PopulateSinglePlacement(Placement);
	}
	for (FPointPlacement& Placement : GametimeData.PillarPlacements)
	{
		if (GametimeData.HiddenFloors.Contains(Placement.GridPosition.Z)) continue;
		PopulateSinglePlacement(Placement);
	}
	for (FEdgePlacement& Placement : GametimeData.WallPlacements)
	{
		if (GametimeData.HiddenFloors.Contains(Placement.GetEdgePosition().Z)) continue;
		PopulateSinglePlacement(Placement);
	}
	for (FEdgePlacement& Placement : GametimeData.EdgePlacements)
	{
		if (GametimeData.HiddenFloors.Contains(Placement.GetEdgePosition().Z)) continue;
		PopulateSinglePlacement(Placement);
	}
	for (FPointPlacement& Placement : GametimeData.PointPlacements)
	{
		if (GametimeData.HiddenFloors.Contains(Placement.GridPosition.Z)) continue;
		PopulateSinglePlacement(Placement);
	}

	for (AActor* Actor : SpawnedTiledActors)
	{
		Actor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
	}
}

void ATiledLevel::MakeEditable()
{
	Modify();
	ActiveAsset = ActiveAsset->CloneAsset(this);
	UTiledItemSet* DupItemSet = CastChecked<UTiledItemSet>(
		StaticDuplicateObject(ActiveAsset->GetItemSetAsset(), ActiveAsset, NAME_None, EObjectFlags::RF_Transactional));
	ActiveAsset->SetActiveItemSet(DupItemSet);
	IsInstance = true;
	ResetAllInstance(true);
	for (UTiledLevelItem* Item : DupItemSet->GetItemSet())
	{
		if (Item->SourceType == ETLSourceType::Actor) continue;
		if (TiledObjectSpawner.Find(Item->TiledMesh))
		{
			Item->UpdateHISM.BindLambda([=]()
			{
				UpdateHISM_Properties(TiledObjectSpawner[Item->TiledMesh], Item);
			});
		}
	}
}


#if WITH_EDITOR
void ATiledLevel::Break()
{
	// TODO: rotation in pitch and roll will affect the break result...   the X/Y scale is swapped
	// This issue still exists in auto sized items.... after I hard code a solution...
	
	for ( auto It = TiledObjectSpawner.CreateIterator(); It; ++It)
	{
		UStaticMesh* MeshPtr = It.Key();
		UHierarchicalInstancedStaticMeshComponent* HISM = It.Value();
		int N = HISM->GetInstanceCount();
		for (int i = 0; i < N; i++)
		{
			FTransform OutTransform;
			HISM->GetInstanceTransform(i, OutTransform, true);
			AStaticMeshActor* NewActor = GetWorld()->SpawnActor<AStaticMeshActor>();
			NewActor->GetStaticMeshComponent()->SetStaticMesh(MeshPtr);
			NewActor->SetMobility(EComponentMobility::Movable);
			NewActor->SetActorTransform(OutTransform);
			if (GetActorRotation().Pitch != 0.f || GetActorRotation().Roll != 0.f || GetActorRotation().Yaw != 0.f)
			{
				const float TestYaw = NewActor->GetActorRotation().Yaw + GetActorRotation().Yaw;
				if ( FMath::IsWithin<float>(TestYaw, 60.f, 120.f) || FMath::IsWithin<float>(TestYaw, -120.f, -60.f))
				{
					 const FVector RawScale = GetActorScale();
					 NewActor->SetActorScale3D(FVector(RawScale.Y, RawScale.X, RawScale.Z));
				}
			}
			NewActor->SetMobility(EComponentMobility::Static);
			FString FloorName = FTiledLevelUtility::GetFloorNameFromPosition(FMath::RoundToInt(HISM->PerInstanceSMCustomData[2 + i*6]));
			NewActor->SetActorLabel(MeshPtr->GetName(), false);
			NewActor->SetFolderPath(FName(FString::Printf(TEXT("%s/%s"), *this->GetName(), *FloorName)));
		}
	}

	for (AActor* SpawnedActor : SpawnedTiledActors)
	{
		const int NTags = SpawnedActor->Tags.Num();
		FVector TilePosition;
		TilePosition.InitFromString(SpawnedActor->Tags[NTags-3].ToString());
		FString FloorName = FTiledLevelUtility::GetFloorNameFromPosition(TilePosition.Z);
		AActor* NewActor = GetWorld()->SpawnActor(SpawnedActor->GetClass());
		NewActor->SetActorTransform(SpawnedActor->GetActorTransform());
		NewActor->SetFolderPath(FName(FString::Printf(TEXT("%s/%s"), *this->GetName(), *FloorName)));
	}
	Destroy();
}

#endif

FBox ATiledLevel::GetBoundaryBox()
{
	FVector BoundaryMin, BoundaryMax;
	const FVector TileSize = GetAsset()->GetTileSize();
	BoundaryMin = GetActorLocation() + FVector(-1,0,FMath::Max(GetAsset()->TiledFloors).FloorPosition * TileSize.Z);
	BoundaryMax = BoundaryMin + FVector(GetAsset()->X_Num, GetAsset()->Y_Num, GetAsset()->TiledFloors.Num()) * TileSize;
	return FBox(BoundaryMin, BoundaryMax);
}

// TODO: calculation of offset is wrong!!! 100% wrong!!!
FTiledLevelGameData ATiledLevel::MakeGametimeData()
{
	const FVector MyLocation = GetActorLocation();
	const FVector TileSize = GetAsset()->GetTileSize();
	const FIntVector Offset = FIntVector(MyLocation / TileSize);
	FTiledLevelGameData Data;
	for (FTiledFloor Floor : GetAsset()->TiledFloors)
	{
		for (auto p : Floor.BlockPlacements)
		{
			p.OffsetGridPosition(Offset, TileSize);
			Data.BlockPlacements.Add(p);
		}
		for (auto p : Floor.FloorPlacements)
		{
			p.OffsetGridPosition(Offset, TileSize);
			Data.FloorPlacements.Add(p);
		}
		for (auto p: Floor.WallPlacements)
		{
			p.OffsetEdgePosition(Offset, TileSize);
			Data.WallPlacements.Add(p);
		}
		for (auto p: Floor.EdgePlacements)
		{
			p.OffsetEdgePosition(Offset, TileSize);
			Data.EdgePlacements.Add(p);
		}
		for (auto p: Floor.PillarPlacements)
		{
			p.OffsetPointPosition(Offset, TileSize);
			Data.PillarPlacements.Add(p);
		}
		for (auto p: Floor.PointPlacements)
		{
			p.OffsetPointPosition(Offset, TileSize);
			Data.PointPlacements.Add(p);
		}
	}
	Data.Boundaries.Add(GetBoundaryBox());
	return Data;
}

TArray<FGuid> ATiledLevel::GetEraserActiveItemIDs() const
{
     TArray<FGuid> Out;
     for (UTiledLevelItem* Item : ActiveAsset->GetItemSet())
     {
         if (Item->bIsEraserAllowed)
         {
             Out.Add(Item->ItemID);
         }
     }
     return Out;
}

void ATiledLevel::CreateNewHISM(UTiledLevelItem* SourceItem)
{
	if (SourceItem->SourceType == ETLSourceType::Actor) return;
	if (TiledObjectSpawner.Find(SourceItem->TiledMesh))
	{
		return;
	}
	UHierarchicalInstancedStaticMeshComponent* NewHISM = NewObject<UHierarchicalInstancedStaticMeshComponent>(this, NAME_None, RF_Transactional);
	NewHISM->SetStaticMesh(SourceItem->TiledMesh);
	NewHISM->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);
	NewHISM->SetMobility(EComponentMobility::Movable);
	NewHISM->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	NewHISM->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECR_Block);
	// 6 custom data for pos and extent (floor), pos, length, height, is horizontal (edge)
	NewHISM->NumCustomDataFloats = 6;
	NewHISM->RegisterComponentWithWorld(GetWorld());
	UpdateHISM_Properties(NewHISM, SourceItem);
	SourceItem->UpdateHISM.BindLambda([=]()
	{
		UpdateHISM_Properties(NewHISM, SourceItem);
	});
	TiledObjectSpawner.Add(SourceItem->TiledMesh, NewHISM);
}

void ATiledLevel::UpdateHISM_Properties(UHierarchicalInstancedStaticMeshComponent* InHISM, UTiledLevelItem* InItem)
{
	// init instance settings
	// TODO: let this property to user customizable in future if strange reset location bug is fixed...
	InHISM->Mobility = EComponentMobility::Movable;
	
	InHISM->SetCullDistances(InItem->CullDistance.Min, InItem->CullDistance.Max);
	InHISM->CastShadow = InItem->CastShadow;
	InHISM->bCastDynamicShadow = InItem->bCastDynamicShadow;
	InHISM->bCastContactShadow = InItem->bCastContactShadow;
	InHISM->RuntimeVirtualTextures = InItem->RuntimeVirtualTextures;
	InHISM->VirtualTextureRenderPassType = InItem->VirtualTextureRenderPassType;
	InHISM->VirtualTextureCullMips = InItem->VirtualTextureCullMips;
	InHISM->TranslucencySortPriority = InItem->TranslucencySortPriority;
	InHISM->bAffectDynamicIndirectLighting = InItem->bAffectDynamicIndirectLighting;
	InHISM->bAffectDistanceFieldLighting = InItem->bAffectDistanceFieldLighting;
	InHISM->bCastShadowAsTwoSided = InItem->bCastShadowAsTwoSided;
	InHISM->bReceivesDecals = InItem->bReceivesDecals;;
	InHISM->bOverrideLightMapRes = InItem->bOverrideLightMapRes;
	InHISM->LightmapType = InItem->LightmapType;
	InHISM->bUseAsOccluder = InItem->bUseAsOccluder;
	InHISM->bEnableDensityScaling = InItem->bEnableDensityScaling;
	InHISM->LightingChannels = InItem->LightingChannels;
	InHISM->bRenderCustomDepth = InItem->bRenderCustomDepth;
	InHISM->CustomDepthStencilWriteMask = InItem->CustomDepthStencilWriteMask;
	InHISM->CustomDepthStencilValue = InItem->CustomDepthStencilValue;
	InHISM->BodyInstance.CopyBodyInstancePropertiesFrom(&InItem->BodyInstance);
	InHISM->bHasCustomNavigableGeometry = InItem->CustomNavigableGeometry;
	int i = 0;
	for (UMaterialInterface* M : InItem->OverrideMaterials)
	{
		InHISM->SetMaterial(i, M);
		i++;
	}
}

AActor* ATiledLevel::SpawnActorPlacement(const FItemPlacement& ItemPlacement)
{
	AActor* NewActor;
	if (!UKismetSystemLibrary::IsServer(GetWorld())) return nullptr;
	if (UTiledLevelRestrictionItem* RestrictionItem = Cast<UTiledLevelRestrictionItem>(ItemPlacement.GetItem()))
	{
		// check if there exist a helper already
		for (AActor* A : SpawnedTiledActors)
		{
			if (ATiledLevelRestrictionHelper* Arh = Cast<ATiledLevelRestrictionHelper>(A))
			{
				if (Arh->GetSourceItem() == RestrictionItem)
				{
					FTilePlacement* TP = (FTilePlacement*)&ItemPlacement;
					Arh->AddTargetedPositions(TP->GridPosition);
					return nullptr;
				}
			}
		}
		// spawn restriction helper if no existing one
		NewActor = GetWorld()->SpawnActor(ATiledLevelRestrictionHelper::StaticClass());
		FTilePlacement* TP = (FTilePlacement*)&ItemPlacement;
		ATiledLevelRestrictionHelper* NewArh = Cast<ATiledLevelRestrictionHelper>(NewActor);
		NewArh->SourceItemSet = Cast<UTiledItemSet>(RestrictionItem->GetOuter());
		NewArh->SourceItemID = RestrictionItem->ItemID;
		NewArh->SetupPreviewVisual();
		NewArh->AddTargetedPositions(TP->GridPosition);
	}
	else
	{
		FActorSpawnParameters SpawnParams;
		if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0))
		{
			SpawnParams.Instigator = PlayerController->GetInstigator();
			SpawnParams.Owner = PlayerController;
		}
		SpawnParams.Template = nullptr;
		FVector Loc = {0, 0,0 };
		FRotator Rot = {0, 0,0 };

		NewActor = GetWorld()->SpawnActor(Cast<UBlueprint>(ItemPlacement.GetItem()->TiledActor)->GeneratedClass, &Loc, &Rot, SpawnParams);
	}
	// // TODO: spawned actor replication??
	// NewActor->SetReplicates(true); // should I make spawned actor replicate? or just stop spawn them if they are client
	NewActor->SetActorLocationAndRotation(ItemPlacement.TileObjectTransform.GetLocation(), ItemPlacement.TileObjectTransform.Rotator());
	NewActor->SetActorScale3D(ItemPlacement.TileObjectTransform.GetScale3D());
	NewActor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
	return NewActor;
}


AActor* ATiledLevel::SpawnMirroredPlacement(const FItemPlacement& ItemPlacement)
{
	// // TODO: spawned actor replication??
	AStaticMeshActor* NewMirrored = GetWorld()->SpawnActor<AStaticMeshActor>();
	NewMirrored->SetMobility(EComponentMobility::Movable);
	NewMirrored->SetActorLocationAndRotation(ItemPlacement.TileObjectTransform.GetLocation(), ItemPlacement.TileObjectTransform.Rotator());
	NewMirrored->SetActorScale3D(ItemPlacement.TileObjectTransform.GetScale3D());
	NewMirrored->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
	NewMirrored->GetStaticMeshComponent()->SetStaticMesh(ItemPlacement.GetItem()->TiledMesh);
	return NewMirrored;
}

#undef LOCTEXT_NAMESPACE
