// Copyright 2022 PufStudio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TiledLevelRestrictionHelper.generated.h"

UCLASS(NotPlaceable, NotBlueprintable)
class TILEDLEVELRUNTIME_API ATiledLevelRestrictionHelper : public AActor
{
	GENERATED_BODY()

	ATiledLevelRestrictionHelper();
	
public:
	UPROPERTY()
	FGuid SourceItemID;

	UPROPERTY()
	TObjectPtr<class UTiledItemSet> SourceItemSet; 
	
	class UTiledLevelRestrictionItem* GetSourceItem() const;
	
	void SetupPreviewVisual();

	bool IsInsideTargetedTilePositions(const TArray<FIntVector>& PositionsToCheck);

	UPROPERTY()
	TArray<FIntVector> TargetedTilePositions;
	
	void AddTargetedPositions(FIntVector NewPoint);
	
	void RemoveTargetedPositions(FIntVector PointToRemove);

protected:
	UPROPERTY()
	TObjectPtr<class USceneComponent> Root;
	
	UPROPERTY()
	TObjectPtr<class UProceduralMeshComponent> AreaHint;
	
private:
	
	void UpdateVisual();

	UPROPERTY()
	TObjectPtr<class UMaterialInstanceDynamic> M_AreaHint; 
	
};
