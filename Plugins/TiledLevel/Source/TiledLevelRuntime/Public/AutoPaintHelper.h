// Copyright 2023 PufStudio. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AutoPaintHelper.generated.h"

class UAutoPaintRule;
class UTextRenderComponent;
class UMaterialInstanceDynamic;
class UMaterialBillboardComponent;
class UAutoPaintItemRule;
class UProceduralMeshComponent;
class UHierarchicalInstancedStaticMeshComponent;

UCLASS()
class TILEDLEVELRUNTIME_API AAutoPaintHelper : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AAutoPaintHelper();

	void Init(class UTiledLevelAsset* InAsset);

	bool HasInit();
	
	void UpdateVisual(float InOpacity);

	void ClearHint();
	
	void UpdateOpacity(float NewOpacity);

	void UpdateMatchedHint(const TArray<FIntVector>& MatchedPositions);

	void UpdateRulePreview(UAutoPaintItemRule* SelectedItemRule);

	TArray<FName> HintNames;

private:
	UPROPERTY()
	TObjectPtr<UTiledLevelAsset> AssetPtr;
	
	UPROPERTY()
	TObjectPtr<UAutoPaintRule> RulePtr;

	UPROPERTY()
	TObjectPtr<USceneComponent> Root;
	
	UPROPERTY()
	TObjectPtr<UProceduralMeshComponent> HintBlocks;

	UPROPERTY()
	TObjectPtr<UProceduralMeshComponent> RulePreviewBlocks;

	UPROPERTY()
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> PreviewSpawns;

	UPROPERTY()
	TArray<TObjectPtr<AActor>> PreviewSpawnActors;

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> M_Hints;

	UPROPERTY()
	TArray<TObjectPtr<UMaterialBillboardComponent>> BillboardComponents;

	UPROPERTY()
	TArray<TObjectPtr<UTextRenderComponent>> DuplicatedTypeHints;
};
