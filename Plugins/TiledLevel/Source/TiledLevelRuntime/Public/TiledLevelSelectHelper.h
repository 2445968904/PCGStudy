// Copyright 2022 PufStudio. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "TiledLevelGrid.h"
#include "TiledLevelTypes.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "GameFramework/Actor.h"
#include "TiledLevelSelectHelper.generated.h"

enum EDisplayStatus
{
    Empty,
    Selection,
    Template
};
//选中的类
UCLASS(NotPlaceable, NotBlueprintable)
class TILEDLEVELRUNTIME_API ATiledLevelSelectHelper : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ATiledLevelSelectHelper();

	void SyncToSelection(FVector InTileSize, const TArray<FTilePlacement>& InTiles, const TArray<FEdgePlacement>& InEdges, const TArray<FPointPlacement>& InPoints, bool bForTemplate = false);
    void PopulateSelected(bool bForTemplate = false);
	void Move(FIntVector NewPosition, FVector InTileSize, FTransform AnchorTransform);
	void RotateSelection(bool IsClockwise = true);
	void EmptyHints();
#if WITH_EDITOR	
	void Hide();
	void Unhide();
#endif	
	int GetNumOfCopiedPlacements() const;

    void GetSelectedPlacements(TArray<FTilePlacement>& OutTiles, TArray<FEdgePlacement>& OutEdges, TArray<FPointPlacement>& OutPoints, bool bFromTemplate = false);
	FIntVector GetSelectedExtent(bool bForTemplate = false);
    
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY()
	TObjectPtr<USceneComponent> Root;
	
	UPROPERTY()
	TObjectPtr<UTiledLevelGrid> SelectionArea;
	
private:
	TArray<FTilePlacement> SelectedTiles;
	TArray<FEdgePlacement> SelectedEdges;
	TArray<FPointPlacement> SelectedPoints;
	TArray<FTilePlacement> TemplateTiles;
	TArray<FEdgePlacement> TemplateEdges;
	TArray<FPointPlacement> TemplatePoints;
	TArray<FTilePlacement> GetModifiedTiles(bool IsFromTemplate = false);
	TArray<FEdgePlacement> GetModifiedEdges(bool IsFromTemplate = false);
	TArray<FPointPlacement> GetModifiedPoints(bool IsFromTemplate = false);
	int RotationIndex = 0;
	//这是一个映射
	UPROPERTY()
	TMap<TObjectPtr<UStaticMesh>, TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> HintMeshSpawner;
	
	UPROPERTY()
	TArray<TObjectPtr<AActor>> HintActors;

	FVector TileSize;
	FIntVector SelectionExtent;
    FIntVector TemplateExtent;
    EDisplayStatus DisplayStatus = EDisplayStatus::Empty;

	void EmptyData(bool bTemplate = false);

	void CreateHISM(UStaticMesh* MeshPtr);

};
