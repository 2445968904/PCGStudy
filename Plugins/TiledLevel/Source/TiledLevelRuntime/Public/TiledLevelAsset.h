// Copyright 2022 PufStudio. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "TiledItemSet.h"
#include "UObject/Object.h"
#include "TiledLevelTypes.h"
#include "TiledLevelAsset.generated.h"

struct FAutoPaintItem;
class UAutoPaintRule;
/*
 * TODO: change this to layer... so that front-back / right-left side editing is possible??
 */
USTRUCT()
struct FTiledFloor
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	int FloorPosition = 0; //0 -> 1F, 5 -> 6F, -2 -> B2    //层数

	UPROPERTY()
	FName FloorName; // ex: 3F, 2F, 1F, B1, B2, ...        //当前层的名称

	UPROPERTY()
	TArray<FTilePlacement> BlockPlacements;
	
	UPROPERTY()
	TArray<FTilePlacement> FloorPlacements;

	UPROPERTY()
	TArray<FEdgePlacement> WallPlacements;

	UPROPERTY()
	TArray<FPointPlacement> PillarPlacements;

	UPROPERTY()
	TArray<FEdgePlacement> EdgePlacements;

	UPROPERTY()
	TArray<FPointPlacement> PointPlacements;

	UPROPERTY()
	TArray<FTilePlacement> AutoPaintGenTiles;
	
	UPROPERTY()
	TArray<FEdgePlacement> AutoPaintGenEdges;
	
	UPROPERTY()
	TArray<FPointPlacement> AutoPaintGenPoints;

	UPROPERTY()
	bool ShouldRenderInEditor = true;

	void UpdatePosition(const int& NewFloorPosition, const float& TileSizeZ)
	{
		const bool IsUp = FloorPosition < NewFloorPosition;
		const int Times = FMath::Abs(NewFloorPosition - FloorPosition);
		for (FTilePlacement& P : BlockPlacements)
		{
			P.OnMoveFloor(TileSizeZ, IsUp, Times);
		}
		for (FTilePlacement& P : FloorPlacements)
		{
			P.OnMoveFloor(TileSizeZ, IsUp, Times);
		}
		for (FEdgePlacement& P : WallPlacements)
		{
			P.OnMoveFloor(TileSizeZ, IsUp, Times);
		}
		for (FPointPlacement& P : PillarPlacements)
		{
			P.OnMoveFloor(TileSizeZ, IsUp, Times);
		}
		for (FEdgePlacement& P : EdgePlacements)
		{
			P.OnMoveFloor(TileSizeZ, IsUp, Times);
		}
		for (FPointPlacement& P : PointPlacements)
		{
			P.OnMoveFloor(TileSizeZ, IsUp, Times);
		}
		FloorPosition = NewFloorPosition;
		if (FloorPosition < 0)
			FloorName =  FName(FString::Printf(TEXT("B%d"), abs(FloorPosition)));
		else
			FloorName = FName(FString::Printf(TEXT("%dF"), FloorPosition + 1));	
	}

	TArray<FItemPlacement> GetItemPlacements() const
	{
		TArray<FItemPlacement> Out;
		Out.Append(BlockPlacements);
		Out.Append(FloorPlacements);
		Out.Append(WallPlacements);
		Out.Append(PillarPlacements);
		Out.Append(EdgePlacements);
		Out.Append(PointPlacements);
		return Out;
	}

	TArray<FItemPlacement*> GetAllPlacements()
	{
		TArray<FItemPlacement*> Out;
		for (auto& P : BlockPlacements)
			Out.Add(&P);
		for (auto& P : FloorPlacements)
			Out.Add(&P);
		for (auto& P : WallPlacements)
			Out.Add(&P);
		for (auto& P : EdgePlacements)
			Out.Add(&P);
		for (auto& P : PillarPlacements)
			Out.Add(&P);
		for (auto& P : PointPlacements)
			Out.Add(&P);
		return Out;
	}

	// Get both block and floor
	TArray<FTilePlacement> GetTilePlacements() const
	{
		TArray<FTilePlacement> Out;
		Out.Append(BlockPlacements);
		Out.Append(FloorPlacements);
		return Out;
	}

	// Get both Wall and Edge, 2d means extent of one axis is fixed
	TArray<FEdgePlacement> Get2DShapePlacements() const
	{
		TArray<FEdgePlacement> Out;
		Out.Append(WallPlacements);
		Out.Append(EdgePlacements);
		return Out;
	}

	// Get both Pillar and point, extent of two axis is fixed
	TArray<FPointPlacement> Get1DShapePlacements() const
	{
		TArray<FPointPlacement> Out;
		Out.Append(PillarPlacements);
		Out.Append(PointPlacements);
		return Out;
	}

	FTiledFloor() {}
	
	FTiledFloor(const int& NewPosition)
		:FloorPosition(NewPosition)
	{
		if (FloorPosition < 0)
			FloorName =  FName(FString::Printf(TEXT("B%d"), abs(FloorPosition)));
		else
			FloorName = FName(FString::Printf(TEXT("%dF"), FloorPosition + 1));
	}

	// this is reverse order
	bool operator< (const FTiledFloor Other) const { return FloorPosition > Other.FloorPosition; }
	bool operator== (const FTiledFloor Other) const { return FloorPosition == Other.FloorPosition; }
	bool operator== (const int InFloorPosition) const { return FloorPosition == InFloorPosition; }
};


// x,y,z tile size, x,y tile num, n floors, z starting floor index
DECLARE_MULTICAST_DELEGATE_SevenParams(FOnTiledLevelAreaChanged, float, float, float, int, int, int, int);
DECLARE_DELEGATE(FOnResetTileSize)
DECLARE_DELEGATE(FRequestForUpdateAutoPaintPlacements)
DECLARE_DELEGATE_OneParam(FMoveHelperFloorGrids, int)

UCLASS(BlueprintType)
class TILEDLEVELRUNTIME_API UTiledLevelAsset : public UObject
{
	GENERATED_BODY()

	UTiledLevelAsset();

public:

	UFUNCTION(Category="Setup", CallInEditor)
	void ConfirmTileSize();

	// WARNING! Reset tile size will clear all existing placements
	UFUNCTION(Category="Setup", CallInEditor)
	void ResetTileSize();

	UPROPERTY(EditAnywhere, Category="Setup", meta=(UIMin = 1, ClampMin = 1, ClampMax=2048, EditCondition="CanEditTileSize"))
	float TileSizeX = 100.f;
	
	UPROPERTY(EditAnywhere, Category="Setup", meta=(UIMin = 1, ClampMin = 1, ClampMax=2048, EditCondition="CanEditTileSize"))
	float TileSizeY = 100.f;
	
	UPROPERTY(EditAnywhere, Category="Setup", meta=(UIMin = 1, ClampMin = 1, ClampMax=2048, EditCondition="CanEditTileSize"))
	float TileSizeZ = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Setup", DisplayName= "Number of X Tiles", meta=(UIMin=1, ClampMin=1, ClampMax=1024, EditCondition="CanEditTileNum"))
	int32 X_Num = 10;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Setup", DisplayName = "Numer of Y Tiles", meta=(UIMin=1, ClampMin=1, ClampMax=1024, EditCondition="CanEditTileNum"))
	int32 Y_Num = 10;

	UPROPERTY()
	bool ViewAsAutoPaint = false;

	void SetNewViewState(bool AsAutoPaint);
    
	UPROPERTY(VisibleAnywhere, Instanced, AdvancedDisplay, Category=Thumbnail)
	TObjectPtr<class UThumbnailInfo> ThumbnailInfo;
	
	UPROPERTY()
	int32 ActiveFloorPosition = 0;
	
	UPROPERTY()
	TArray<FTiledFloor> TiledFloors;

	class FTiledLevelEdMode* EdMode = nullptr;
	
	UPROPERTY()
	TObjectPtr<class ATiledLevel> HostLevel;

	UPROPERTY()
	bool HasOrphanPlacement;

	UFUNCTION()
	void ClearOrphan();
	/*
	 * Use to block some unnecessary "ResetAllInstance", which trigger unwanted make asset dirty
	 * could be a "bug-prone" point...
	 */
	UPROPERTY()
	uint32 VersionNumber = 0;

	bool IsTileSizeConfirmed() const { return !CanEditTileSize; }

	UTiledLevelAsset* CloneAsset(UObject* OuterForClone);

	UPROPERTY(VisibleDefaultsOnly, Category="Setup", AdvancedDisplay)
	bool CanEditTileNum = false;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	void ClearOutOfBoundPlacements();
#endif

	void SetActiveItemSet(UTiledItemSet* NewItemSet);
	void SetActiveAutoPaintRule(UAutoPaintRule* NewRule);
	
	/* Modify above|below ground floor will only affect above|below floors
	 * ie:  B3, B2, B1, 1F, 2F, 3F, 4F
	 *  delete B2: B2(*), B1, 1F, 2F, 3F, 4F
	 *  insert 3F: B3, B2, B1, 1F, 2F, 3F(*), 4F(*), 5F(*)
	 *  (*): the floor changed
	 */ 
	int AddNewFloor(int32 InsertPosition);
	void DuplicatedFloor(int SourcePosition);
	void MoveAllFloors(bool Up);
	void MoveFloor(bool IsUp, int SelectedFloorPosition, int EndFloorPosition, bool ToEnd);
	void ChangeFloorOrdering(int OldPosition, int NewPosition);
	void RemoveFloor(int32 DeleteIndex);
	void EmptyFloor(int FloorPosition);
	void EmptyAllFloors();
	bool IsFloorExists(int32 PositionIndex);
	void RemovePlacements(const TArray<FTilePlacement>& TilesToDelete);
	void RemovePlacements(const TArray<FEdgePlacement>& WallsToDelete);
	void RemovePlacements(const TArray<FPointPlacement>& PointsToDelete);
	void ClearItem(const FGuid& ItemID);
	void ClearItemInActiveFloor(const FGuid& ItemID);
	void ClearInvalidPlacements();
	FTiledFloor* GetActiveFloor();
	FTiledFloor* GetBelowActiveFloor();
    FTiledFloor& GetBottomFloor() { return TiledFloors.Last(); }
    FTiledFloor& GetTopFloor() { return TiledFloors[0]; }
	FTiledFloor* GetFloorFromPosition(int PositionIndex);
	void AddNewTilePlacement(FTilePlacement NewTile);
	void AddNewEdgePlacement(FEdgePlacement NewEdge);
	void AddNewPointPlacement(FPointPlacement NewPoint);
	TArray<FTilePlacement> GetAllBlockPlacements() const;
	TArray<FTilePlacement> GetAllFloorPlacements() const;
	TArray<FPointPlacement> GetAllPillarPlacements() const;
	TArray<FEdgePlacement> GetAllWallPlacements() const;
	TArray<FEdgePlacement> GetAllEdgePlacements() const;
	TArray<FPointPlacement> GetAllPointPlacements() const;
	TArray<FItemPlacement> GetAllItemPlacements() const;
	int GetNumOfAllPlacements() const;
	TSet<UTiledLevelItem*> GetUsedItems() const;
	TSet<UStaticMesh*> GetUsedStaticMeshSet() const;
	void EmptyRegionData(const TArray<FIntVector>& Points);
	void EmptyEdgeRegionData(const TArray<FTiledLevelEdge>& EdgeRegions);
	
	void SetTileSize(const FVector& NewSize)
	{
		TileSizeX = NewSize.X;
		TileSizeY = NewSize.Y;
		TileSizeZ = NewSize.Z;
	}
	FVector GetTileSize() const { return FVector(TileSizeX, TileSizeY, TileSizeZ) ;}
	UTiledItemSet* GetItemSetAsset() const { return ActiveItemSet; }
	TArray<UTiledLevelItem*> GetItemSet() const
	{
		if (!ActiveItemSet)
		{
			ERROR_LOGF("Active Item Set DID NOT exist for ... %s", *this->GetName())
			return TArray<UTiledLevelItem*>{};
		}
		return ActiveItemSet->GetItemSet(); 
	}
	virtual void GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const override;
	FOnTiledLevelAreaChanged OnTiledLevelAreaChanged;
	FOnResetTileSize OnResetTileSize;
	FMoveHelperFloorGrids MoveHelperFloorGrids;

	// auto paint related...
	UAutoPaintRule* GetAutoPaintRule() const { return ActiveAutoPaintRule; }
	bool UpdateAutoPaintData(const FIntVector& NewPos, const FName& NewItemName);
	bool RemoveAutoPaintDataAt(const FIntVector& PosToRemove);
	TArray<FIntVector> GetAllTilePositions();
	void CollectAutoPaintPlacements(TArray<struct FAutoPaintPlacement_Tile>& OutTiles, TArray<struct FAutoPaintPlacement_Edge>& OutEdges,
		TArray<struct FAutoPaintPlacement_Point>& OutPoints);
	FAutoPaintItem GetAutoPaintItemRule(const FName& QueryName);
	void SetAutoPaintData(const TMap<FIntVector, FName>& InData)
	{
		AutoPaintData = InData;
	}
	TMap<FIntVector, FName>& GetAutoPaintData()
	{
		return AutoPaintData;
	}
	void ClearAutoPaintDataForFloor(int InFloorPosition);
	int GetAppliedAutoItemCount();
	void UpdateAutoPaintResults(const TArray<FTilePlacement>& InGenTiles, const TArray<FEdgePlacement>& InGenEdges, const TArray<FPointPlacement>& InGenPoints);
	void ConvertAutoPlacementsToNormal();
	UFUNCTION()
	void OnAutoPaintItemChanged(const FAutoPaintItem& Old, const FAutoPaintItem& New);
	UFUNCTION()
	void OnAutoPaintItemRemoved(const FName& ToRemove);
	
	FRequestForUpdateAutoPaintPlacements RequestForUpdateAutoPaintPlacements;

private:	
	UPROPERTY()
	TObjectPtr<class UTiledItemSet> ActiveItemSet;
	
	UPROPERTY()
	TObjectPtr<class UAutoPaintRule> ActiveAutoPaintRule;
	
	UPROPERTY()
	TMap<FIntVector, FName> AutoPaintData;

	UPROPERTY(VisibleDefaultsOnly, Category="Setup", AdvancedDisplay)
	bool CanEditTileSize = false;
};
