// Copyright 2022 PufStudio. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "TiledLevelTypes.h"
#include "Engine/EngineTypes.h"

enum class EImpactSize : uint8;
class ATiledLevel;

class TILEDLEVELRUNTIME_API FTiledLevelUtility
{
public:
	
	static bool IsTilePlacementOverlapping(const FTilePlacement& Tile1, const FTilePlacement& Tile2);
	static bool IsEdgePlacementOverlapping(const FEdgePlacement& Wall1, const FEdgePlacement& Wall2);
	static bool IsEdgeOverlapping(const FTiledLevelEdge& Edge1, const FVector& Extent1, const FTiledLevelEdge& Edge2, const FVector& Extent2);
	static bool IsEdgeInsideTile(const FTiledLevelEdge& Edge, const FIntVector& EdgeExtent, const FIntVector& TilePosition, const FIntVector& TileExtent);
	static bool IsEdgeInsideArea(const FTiledLevelEdge& TestEdge, int32 Length, FIntPoint AreaSize);
	static bool IsPointPlacementOverlapping(const FPointPlacement& Point1, int ZExtent1, const FPointPlacement& Point2, int ZExtent2);
	static bool IsPointOverlapping(const FIntVector& Point1, int ZExtent1, const FIntVector& Point2, int ZExtent2);
	static bool IsPointInsideTile(FIntVector PointPosition, int PointZExtent, const FIntVector TilePosition, const FIntVector& TileExtent);

	// from placement data to get instance indices
	static void FindInstanceIndexByPlacement(TArray<int32>& FoundIndex,const TArray<float>& CustomData,const TArray<float>& SearchData);
	// make HISM custom data from placement
	static TArray<float> ConvertPlacementToHISM_CustomData(const FTilePlacement& P);
	static TArray<float> ConvertPlacementToHISM_CustomData(const FEdgePlacement& P);
	static TArray<float> ConvertPlacementToHISM_CustomData(const FPointPlacement& P);
	static void SetSpawnedActorTag(const FTilePlacement& P, AActor* TargetActor);
	static void SetSpawnedActorTag(const FEdgePlacement& P, AActor* TargetActor);
	static void SetSpawnedActorTag(const FPointPlacement& P, AActor* TargetActor);

	// use for restriction item, otherwise just check whether overlap is fine
	static TArray<FIntVector> GetOccupiedPositions(class UTiledLevelItem* Item, FIntVector StartPosition, bool ShouldRotate);
	static TArray<FIntVector> GetOccupiedPositions(class UTiledLevelItem* Item, FTiledLevelEdge StartEdge);
	
	// used in both set editor and level helper..., will return init mesh location
	static void ApplyPlacementSettings(const FVector& TileSize, UTiledLevelItem* Item,
		class UStaticMeshComponent* TargetMeshComponent,
		class UTiledLevelGrid* TargetBrushGrid,
		class USceneComponent* Center);

	static void ApplyPlacementSettings_TiledActor(const FVector& TileSize, UTiledLevelItem* Item,
		class UTiledLevelGrid* TargetBrushGrid,
		class USceneComponent* Center);

	// returns movement distance
	static float TrySnapPropToFloor(const FVector& InitLocation, uint8 RotationIndex, const FVector& TileSize ,UTiledLevelItem* Item, UStaticMeshComponent* TargetMeshComponent);
	static void TrySnapPropToWall(const FVector& InitLocation, uint8 RotationIndex, const FVector& TileSize, UTiledLevelItem* Item, UStaticMeshComponent* TargetMeshComponent, float Z_Offset);

	// for fixing attached actors will not spawn when duplicate tiled level with Actor attached, and maybe other purposes?
	static void RequestToResetInstances(const class ATiledLevel* RequestedLevel);

	static FString GetFloorNameFromPosition(int FloorPositionIndex);

	// Fill tool algorithms
	static void FloodFill(TArray<TArray<int>>& InBoard, const FIntPoint& InBoardSize, int X , int Y, TArray<FIntPoint>& FilledTarget);
	static void FloodFillByEdges(const TArray<FTiledLevelEdge>& BlockingEdges, TArray<TArray<int>>& InBoard, const FIntPoint& InBoardSize, int X, int Y, TArray<FIntPoint>& FilledTarget);
	static void GetConsecutiveTiles(TArray<TArray<int>>& InBoard, FIntPoint InBoardSize, int X, int Y, TArray<FIntPoint>& OutTiles);
	static TArray<float> GetWeightedCoefficient(TArray<float>& RawCoefficientArray);
	static bool GetFeasibleFillTile(UTiledLevelItem* InItem, int& RotationIndex, TArray<FIntPoint>& CandidatePoints, FIntPoint& OutPoint);
	static TArray<FTiledLevelEdge> GetEdgesAroundArea(const TSet<FIntPoint>& Region, int Z = 1, bool AskForOuter = true);
	static TArray<FTiledLevelEdge> GetEdgesAroundArea(const TSet<FIntVector>& Region, bool AskForOuter = true); // create an easier overload
	static TSet<FIntVector> GetPointsAroundArea(const TSet<FIntVector>& Region, bool AskForOuter = true);
	static bool GetFeasibleFillEdge(UTiledLevelItem* InItem, TArray<FTiledLevelEdge>& InCandidateEdges, FTiledLevelEdge& OutEdge);

	// enum conversion
	static EPlacedShapeType PlacedTypeToShape(const EPlacedType& PlacedType);

	// gametime utilities
	static void ApplyGametimeRequiredTransform(ATiledLevel* TargetTiledLevel, FVector TargetTileSize);
	static bool CheckOverlappedTiledLevels(ATiledLevel* FirstLevel, ATiledLevel* SecondLevel);

	// Copy from ProceduralMeshConversion::BuildMeshDescription
    static struct FMeshDescription BuildMeshDescription(class UProceduralMeshComponent* ProcMeshComp);
	// Copy from FProceduralMeshComponentDetails::ClickedOnConvertToStaticMesh and with minor modifications...
	static class UProceduralMeshComponent* ConvertTiledLevelAssetToProcMesh(class UTiledLevelAsset* TargetAsset, int TargetLOD = 0, UObject* Outer = nullptr, EObjectFlags Flags = RF_NoFlags);

	static void GenerateCubesWithUniqueFaces(TArray<FIntVector> CubePositions, FVector CubeSize, TArray<FVector>& OutVertices, TArray<int32>& OutTriangles,
		TArray<FVector>& OutNormals, TArray<FVector2D>& OutUVs, float PaddingSize=0.f);

	// ProcMeshComponent helper
	// Make sure to set bReturnFaceIndex for FCollisionQueryParams
	static int GetPMC_SectionIdFromHit(struct FHitResult& InHit);

	// auto paint utilities
	static FIntVector GetImpactSizeCenter(EImpactSize ImpactSize);
	static int GetImpactSizeExtent(EImpactSize ImpactSize);
	static FVector RotationXY(const FVector& InPos , float Degree = 90.f);
	static FIntVector RotationXY(const FIntVector& InPos , float Degree = 90.f);

private:
	static bool IsRequested;
	static FTimerHandle TimerHandle;

};


/** TODO: IntVector, IntPoint, or vector ...  
 * Tile or Grid??
 * The naming convention should be the same, the type to use should be the same?
 */
