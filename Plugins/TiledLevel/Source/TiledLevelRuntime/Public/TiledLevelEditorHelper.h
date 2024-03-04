// Copyright 2022 PufStudio. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "TiledLevelTypes.h"
#include "GameFramework/Actor.h"
#include "TiledLevelEditorHelper.generated.h"

class UTiledLevelGrid;
class UTiledLevelItem;
class UTiledLevelAsset;
class UProceduralMeshComponent;

UCLASS(NotPlaceable, NotBlueprintable)
class TILEDLEVELRUNTIME_API ATiledLevelEditorHelper : public AActor
{
	GENERATED_UCLASS_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void Destroyed() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	
public:
	UPROPERTY()
	TObjectPtr<USceneComponent> Root;
	
	UPROPERTY()
	TObjectPtr<UTiledLevelGrid> AreaHint;

	UPROPERTY()
	TObjectPtr<UProceduralMeshComponent> FloorGrids;

	UPROPERTY()
	TObjectPtr<UProceduralMeshComponent> ProcedurePreviewGrids;
	
	UPROPERTY()
	TObjectPtr<UTiledLevelGrid> CustomGizmo;
	
	UPROPERTY()
	TObjectPtr<UTiledLevelGrid> Brush;
	
	UPROPERTY()
	TObjectPtr<USceneComponent> Center;
	
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> PreviewMesh;

	// for debug
	UPROPERTY()
	TObjectPtr<UTiledLevelGrid> CenterHint;

	void SetupPaintBrush(UTiledLevelItem* Item);
	void MirrorPaintBrush(EAxis::Type MirrorAxis, bool& MirrorXState, bool& MirrorYState, bool& MirrorZState);
	void SetupEraserBrush(FIntVector EraserExtent, EPlacedType SnapTarget);
	void ResetBrush();
	void ToggleQuickEraserBrush(bool bIsSet);
	void ToggleValidBrush(bool IsValid);
	void MoveBrush(FIntVector TilePosition, bool IsSnapEnabled = true);
	void MoveBrush(FTiledLevelEdge TileEdge, bool IsSnapEnable = true ); //this move to edge will not handle vertical or horizontal
	void RotateBrush(bool IsCW, EPlacedType PlacedType, bool& ShouldRotateExtent);
	void RotateEraserBrush(bool CW, bool& ShouldEraseVertical);
	// brush extent may swap between X and Y due to rotation!!
	bool IsBrushExtentChanged();	
	FTransform GetPreviewPlacementWorldTransform();
	void ResizeArea(float X, float Y, float Z, int Num_X, int Num_Y, int Num_Floors, int LowestFloor);
	void MoveFloorGrids(int TargetFloorIndex);

	void SetHelperGridsVisibility(bool IsVisible);

	void UpdateFillPreviewGrids(TArray<FIntPoint> InFillBoard, int InFillFloorPosition = 0, int InFillHeight = 1);
	void UpdateFillPreviewEdges(TArray<FTiledLevelEdge> InEdgePoints, int InFillHeight = 1);
	void SetupAutoPaintBrush(FLinearColor ItemColor);
	void ApplyAdditionalAutoPaintTransform(const FTransform& AdditionalTransform) const;

	uint8 BrushRotationIndex;

	// For gameplay support
	bool IsEditorHelper = false; 
	void SetupPreviewBrushInGame(UTiledLevelItem* Item);
	void SetShouldUsePreviewMaterial(bool ShouldUse) { ShouldUsePreviewMaterial = ShouldUse; }
	void SetCanBuildPreviewMaterial(UMaterialInterface* NewMaterial) { CanBuildPreviewMaterial = NewMaterial; }
	void SetCanNotBuildPreviewMaterial(UMaterialInterface* NewMaterial) { CanNotBuildPreviewMaterial = NewMaterial; }
	void UpdatePreviewHint(bool CanBuild);
	void SetTileSize(const FVector& NewTileSize) { TileSize = NewTileSize; }

private:

	UPROPERTY()
	TObjectPtr<AActor> PreviewActor;

	UPROPERTY()
	TObjectPtr<UTiledLevelItem> ActiveItem;
	
	UPROPERTY()
	TObjectPtr<UMaterialInterface> M_FloorGrids;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> M_HelperFloor;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> M_FillPreview;

	UPROPERTY()
	TObjectPtr<UStaticMesh> SM_Cube;

	FVector TileSize;
	FVector TileExtent;
	FVector PreviewMeshInitLocation;
	FVector PreviewActorRelativeLocation;
	FVector CachedCenterScale{1, 1, 1};
	FVector CachedBrushLocation;
	bool IsBrushValid;
	TArray<FIntPoint> CacheFillTiles;
	TArray<FTiledLevelEdge> CacheFillEdges;
	
	// If not, use original material for valid, and hidden actors for not valid 
	bool ShouldUsePreviewMaterial = true;
	
	UPROPERTY()
	UMaterialInterface* CanBuildPreviewMaterial;
	UPROPERTY()
	UMaterialInterface* CanNotBuildPreviewMaterial;
	
};
