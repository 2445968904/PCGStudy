// Copyright 2022 PufStudio. All Rights Reserved.

#include "TiledLevelRestrictionHelper.h"
#include "KismetProceduralMeshLibrary.h"
#include "ProceduralMeshComponent.h"
#include "TiledItemSet.h"
#include "TiledLevelItem.h"
#include "TiledLevelUtility.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"


// Sets default values
ATiledLevelRestrictionHelper::ATiledLevelRestrictionHelper()
{
	PrimaryActorTick.bCanEverTick = false;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	AreaHint = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("AreaHint"));
	AreaHint->SetCanEverAffectNavigation(false);
	AreaHint->SetupAttachment(Root);
}

UTiledLevelRestrictionItem* ATiledLevelRestrictionHelper::GetSourceItem() const
{
	if (!SourceItemSet) return nullptr;
	return Cast<UTiledLevelRestrictionItem>(SourceItemSet->GetItem(SourceItemID));
}

void ATiledLevelRestrictionHelper::SetupPreviewVisual()
{
	if (!GetSourceItem()) return;
	AreaHint->ClearAllMeshSections();
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FProcMeshTangent> Tangents;
	UKismetProceduralMeshLibrary::GenerateBoxMesh(SourceItemSet->GetTileSize()/2, Vertices, Triangles, Normals, UVs, Tangents);
	TArray<FColor> VertexColors; 
	AreaHint->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, VertexColors, Tangents, false);
	// Set material to the color user specified...
	UMaterialInterface* M_Base = LoadObject<UMaterialInterface>(NULL, TEXT("/TiledLevel/Materials/M_UVOutline"), NULL, 0, NULL);
	M_AreaHint = UMaterialInstanceDynamic::Create(M_Base, this);
	M_AreaHint->SetVectorParameterValue("BorderColor", GetSourceItem()->BorderColor);
	M_AreaHint->SetScalarParameterValue("BorderWidth", GetSourceItem()->BorderWidth);
	AreaHint->SetMaterial(0, M_AreaHint);
	AreaHint->SetHiddenInGame(GetSourceItem()->bHiddenInGame);
}

bool ATiledLevelRestrictionHelper::IsInsideTargetedTilePositions(const TArray<FIntVector>& PositionsToCheck)
{
	for (const FIntVector& P : PositionsToCheck)
	{
		if (TargetedTilePositions.Contains(P))
			return true;
	}
	return false;
}

void ATiledLevelRestrictionHelper::AddTargetedPositions(FIntVector NewPoint)
{
	TargetedTilePositions.Add(NewPoint);
	UpdateVisual();
}

void ATiledLevelRestrictionHelper::RemoveTargetedPositions(FIntVector PointToRemove)
{
	TargetedTilePositions.Remove(PointToRemove);
	UpdateVisual();
}

void ATiledLevelRestrictionHelper::UpdateVisual()
{
	if (!GetSourceItem()) return;
	if (!GetAttachParentActor()) return;
	SetActorTransform(GetAttachParentActor()->GetActorTransform()); // make sure transform is the same as parent tiled level actor
	AreaHint->ClearAllMeshSections();
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FProcMeshTangent> Tangents;
	TArray<FColor> VertexColors;
	FTiledLevelUtility::GenerateCubesWithUniqueFaces(TargetedTilePositions, SourceItemSet->GetTileSize(),
		Vertices, Triangles, Normals, UVs);
      
	AreaHint->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, VertexColors, Tangents, false);
	UMaterialInterface* M_Base = LoadObject<UMaterialInterface>(NULL, TEXT("/TiledLevel/Materials/M_UVOutline"), NULL, 0, NULL);
	M_AreaHint = UMaterialInstanceDynamic::Create(M_Base, this);
	M_AreaHint->SetVectorParameterValue("BorderColor", GetSourceItem()->BorderColor);
	M_AreaHint->SetScalarParameterValue("BorderWidth", GetSourceItem()->BorderWidth);
	AreaHint->SetMaterial(0, M_AreaHint);
}
