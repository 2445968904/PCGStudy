// Copyright 2023 PufStudio. All Rights Reserved.

#include "AutoPaintHelper.h"
#include "AutoPaintRule.h"
#include "ProceduralMeshComponent.h"
#include "TiledLevelAsset.h"
#include "TiledLevelEditorHelper.h"
#include "TiledLevelUtility.h"
#include "Components/MaterialBillboardComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"
#include "Engine/Blueprint.h"


// Sets default values
AAutoPaintHelper::AAutoPaintHelper()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	HintBlocks = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("HintBlocks"));
	HintBlocks->SetupAttachment(Root);
	RulePreviewBlocks = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("RulePreviewBlocks"));
	RulePreviewBlocks->SetupAttachment(Root);
	PreviewSpawns = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("PreviewSpawns"));
	PreviewSpawns->SetupAttachment(Root);
}

void AAutoPaintHelper::Init(UTiledLevelAsset* InAsset)
{
	AssetPtr = InAsset;
	RulePtr = AssetPtr->GetAutoPaintRule();
}

bool AAutoPaintHelper::HasInit()
{
	return AssetPtr != nullptr && RulePtr != nullptr;
}

void AAutoPaintHelper::UpdateVisual(float InOpacity)
{
	// clear existing...
	HintBlocks->ClearAllMeshSections();
	M_Hints.Empty();
	HintNames.Empty();

	// draw the hints
	TMap<FName, TArray<FIntVector>> PerItemPoints;
	for (auto elem : AssetPtr->GetAutoPaintData())
	{
		if (PerItemPoints.Contains(elem.Value))
		{
			PerItemPoints[elem.Value].Add(elem.Key);
		}
		else
		{
			PerItemPoints.Add(elem.Value, {elem.Key});
		}
	}
	int SectionId = 0;
	for (auto elem : PerItemPoints)
	{
		const FAutoPaintItem* TargetItem = RulePtr->Items.FindByKey(elem.Key);
		if (!TargetItem) return;
		if (!RulePtr->Items.ContainsByPredicate([=](const FAutoPaintItem& Item)
		{
			return Item.ItemName == elem.Key;
		}))
		{
			return;
		}
		TArray<FVector> Vertices;
		TArray<int32> Triangles;
		TArray<FVector> Normals;
		TArray<FVector2D> UVs;
		FTiledLevelUtility::GenerateCubesWithUniqueFaces(elem.Value, AssetPtr->GetTileSize(), Vertices, Triangles, Normals, UVs, 3.f);
		HintBlocks->CreateMeshSection(SectionId, Vertices, Triangles, Normals, UVs, TArray<FColor>{}, TArray<FProcMeshTangent>{}, true);
		UMaterialInterface* M_Base = LoadObject<UMaterialInterface>(NULL, TEXT("/TiledLevel/Materials/M_UVOutline"), NULL, 0, NULL);
		UMaterialInstanceDynamic* M_Hint = UMaterialInstanceDynamic::Create(M_Base, this);
		M_Hint->SetScalarParameterValue("Opacity", InOpacity);
		M_Hint->SetVectorParameterValue("BorderColor", TargetItem->PreviewColor);
		M_Hint->SetScalarParameterValue("BorderWidth", 0.5f);
		HintBlocks->SetMaterial(SectionId, M_Hint);
		M_Hints.Add(M_Hint);
		HintNames.Add(elem.Key);
		SectionId++;
	}
}

void AAutoPaintHelper::ClearHint()
{
	HintBlocks->ClearAllMeshSections();
	M_Hints.Empty();
}

void AAutoPaintHelper::UpdateOpacity(float NewOpacity)
{
	for (auto& M_Hint : M_Hints)
		M_Hint->SetScalarParameterValue("Opacity", NewOpacity);
}

void AAutoPaintHelper::UpdateMatchedHint(const TArray<FIntVector>& MatchedPositions)
{
	const int SectionId = AssetPtr->GetAppliedAutoItemCount();
	HintBlocks->ClearMeshSection(SectionId);
	if (MatchedPositions.IsEmpty()) return;
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	FTiledLevelUtility::GenerateCubesWithUniqueFaces(MatchedPositions, AssetPtr->GetTileSize(), Vertices, Triangles, Normals, UVs, 5.f);
	HintBlocks->CreateMeshSection(SectionId, Vertices, Triangles, Normals, UVs, TArray<FColor>{}, TArray<FProcMeshTangent>{}, false);
	UMaterialInterface* M_Base = LoadObject<UMaterialInterface>(NULL, TEXT("/TiledLevel/Materials/M_UVOutline"), NULL, 0, NULL);
	UMaterialInstanceDynamic* M_Hint = UMaterialInstanceDynamic::Create(M_Base, this);
	M_Hint->SetScalarParameterValue("Opacity", 1.f);
	M_Hint->SetVectorParameterValue("BorderColor", FLinearColor::Red);
	M_Hint->SetScalarParameterValue("BorderWidth", 0.5f);
	HintBlocks->SetMaterial(SectionId, M_Hint);
	
}
void AAutoPaintHelper::UpdateRulePreview(UAutoPaintItemRule* SelectedItemRule)
{
	RulePreviewBlocks->ClearAllMeshSections();
	for (auto B: BillboardComponents)
	{
		B->SetVisibility(false);
	}
	for (auto T : DuplicatedTypeHints)
	{
		T->SetVisibility(false);
	}
	PreviewSpawns->ClearInstances();
	for (AActor* A : PreviewSpawnActors)
	{
		if (A) A->Destroy();
	}
	PreviewSpawnActors.Empty();
	if (!SelectedItemRule)
	{
		return;
	}
	FTiledItemObj SpawnID = SelectedItemRule->PickWhatToSpawn();
	if (SpawnID.TiledItemID.IsEmpty()) return;
	
	FIntVector Center = FTiledLevelUtility::GetImpactSizeCenter(SelectedItemRule->ImpactSize);
	int Extent = FTiledLevelUtility::GetImpactSizeExtent(SelectedItemRule->ImpactSize);
	
	TArray<FIntVector> PosMods = {
		FIntVector(0),
		FIntVector(Extent + 1, 0, 0),	
		FIntVector(Extent + 1, Extent + 1, 0),	
		FIntVector(0, Extent + 1, 0),	
	};
	
	TMap<EDuplicationType, FText> DupHints_Text;
	DupHints_Text.Add(EDuplicationType::R1, FText::FromString("Rotate Clockwise"));
	DupHints_Text.Add(EDuplicationType::R2, FText::FromString("Mirrored"));
	DupHints_Text.Add(EDuplicationType::R3, FText::FromString("Rotate Counter Clockwise"));
	
	FVector PositionOffset = AssetPtr->GetTileSize() * FVector(AssetPtr->X_Num + 1, 0, 0);
	// draw the hints
	TMap<FName, TArray<FIntVector>> PerItemPoints;
	TArray<FIntVector> NegatePositions_Temp;
	TArray<FIntVector> NegatePositions;
	for (auto elem : SelectedItemRule->AdjacencyRules)
	{
		if (PerItemPoints.Contains(elem.Value.AutoPaintItemName))
		{
			PerItemPoints[elem.Value.AutoPaintItemName].Add(elem.Key);
		}
		else
		{
			PerItemPoints.Add(elem.Value.AutoPaintItemName, {elem.Key});
		}
		if (elem.Value.bNegate)
		{
			NegatePositions_Temp.Add(elem.Key);
		}
	}
	int SectionId = 0;
	for (auto elem : PerItemPoints)
	{
		FLinearColor PreviewColor;
		if (elem.Key == "Empty")
		{
			PreviewColor = FLinearColor::White;
		}
		else
		{
			PreviewColor = RulePtr->Items.FindByKey(elem.Key)->PreviewColor;
		}
		TArray<FVector> Vertices;
		TArray<int32> Triangles;
		TArray<FVector> Normals;
		TArray<FVector2D> UVs;
	
		// Just modify and duplicate the duplicate versions...
		TArray<FIntVector> AdjPositionsWithDups;
		int N_Dups = 0;
		for (EDuplicationType D : SelectedItemRule->GetDuplicateCases())
		{
			for (FIntVector P : elem.Value)
			{
				AdjPositionsWithDups.Add( UAutoPaintRule::RotatePosToDuplicated(P, D)  + Center + PosMods[N_Dups]);
			}
			for (FIntVector P : NegatePositions_Temp)
			{
				NegatePositions.Add(UAutoPaintRule::RotatePosToDuplicated(P, D) + Center + PosMods[N_Dups]);
			}
			N_Dups++;
		}
		FTiledLevelUtility::GenerateCubesWithUniqueFaces(AdjPositionsWithDups, AssetPtr->GetTileSize(), Vertices, Triangles, Normals, UVs, 3.f);
		for (FVector& V : Vertices)
			V += PositionOffset;
		RulePreviewBlocks->CreateMeshSection(SectionId, Vertices, Triangles, Normals, UVs, TArray<FColor>{}, TArray<FProcMeshTangent>{}, false);
		UMaterialInterface* M_Base = LoadObject<UMaterialInterface>(NULL, TEXT("/TiledLevel/Materials/M_UVOutline"), NULL, 0, NULL);
		UMaterialInstanceDynamic* M_Hint = UMaterialInstanceDynamic::Create(M_Base, this);
		M_Hint->SetScalarParameterValue("Opacity", 0.5f);
		M_Hint->SetVectorParameterValue("BorderColor", PreviewColor);
		M_Hint->SetScalarParameterValue("BorderWidth", 0.5f);
		RulePreviewBlocks->SetMaterial(SectionId, M_Hint);
		M_Hints.Add(M_Hint);
		SectionId++;
	}
	// handle negate hint
	UMaterialInterface* M_Empty = LoadObject<UMaterialInterface>(NULL, TEXT("/TiledLevel/Materials/M_EmptyHint"), NULL, 0, NULL);
	/*
	 * this check is for too reduce unused actor components...
	 * no need the delete all comps and then create required amount of them
	 */
	if (NegatePositions.Num() > BillboardComponents.Num())
	{
		const int ToCreate = NegatePositions.Num() - BillboardComponents.Num();
		for (int i = 0; i < ToCreate; i++)
		{
			UMaterialBillboardComponent* NewBill = NewObject<UMaterialBillboardComponent>(this, NAME_None, RF_Transactional);
			NewBill->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);
			NewBill->AddElement(M_Empty, nullptr, false, 64, 64, nullptr);
			NewBill->RegisterComponentWithWorld(GetWorld());
			NewBill->SetVisibility(false);
			BillboardComponents.Add(NewBill);
		}
	}
	int i = 0;
	for (FIntVector Pos : NegatePositions)
	{
		BillboardComponents[i]->SetVisibility(true);
		BillboardComponents[i]->SetRelativeLocation(PositionOffset + AssetPtr->GetTileSize() * (FVector(Pos) + FVector(0.5)));
		i++;
	}
	
	// handle mirrored hint
	if (SelectedItemRule->GetDuplicateCases().Num() > DuplicatedTypeHints.Num())
	{
		const int ToCreate = SelectedItemRule->GetDuplicateCases().Num() - DuplicatedTypeHints.Num() - 1;
		for (i = 0; i < ToCreate; i++)
		{
			UTextRenderComponent* NewText = NewObject<UTextRenderComponent>(this, NAME_None, RF_Transactional);
			NewText->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);
			NewText->SetWorldSize(48);
			NewText->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
			NewText->SetTextRenderColor(FColor(224, 32, 224));
			NewText->RegisterComponentWithWorld(GetWorld());
			NewText->SetRelativeRotation(FRotator(0, 90, 0));
			NewText->SetVisibility(false);
			DuplicatedTypeHints.Add(NewText);
		}
	}
	i = 0;
	for (EDuplicationType D : SelectedItemRule->GetDuplicateCases())
	{
		if (D == EDuplicationType::No) continue;
		DuplicatedTypeHints[i]->SetVisibility(true);
		DuplicatedTypeHints[i]->SetText(DupHints_Text[D]);
		DuplicatedTypeHints[i]->SetRelativeLocation(
			PositionOffset + (FVector(PosMods[1+i] + Center) + FVector(0.5, 0.5 - Extent * 0.5, 2)) * AssetPtr->GetTileSize());
		i++;
		// how high should the text get displayed? just use a random value?
	}
	// handle spawn
	ATiledLevelEditorHelper* THelper = Cast<ATiledLevelEditorHelper>(UGameplayStatics::GetActorOfClass(GetWorld(), ATiledLevelEditorHelper::StaticClass()));

	FAutoPaintSpawnAdjustment* SAdj = &SelectedItemRule->AutoPaintItemSpawns[SpawnID];
	
	const bool IsMirrored = SAdj->MirrorX || SAdj->MirrorY || SAdj->MirrorZ;
	UTiledLevelItem* Item = AssetPtr->GetItemSetAsset()->GetItem(FGuid(SpawnID.TiledItemID));
	if (!Item) return;
	if (Item->SourceType == ETLSourceType::Mesh)
		PreviewSpawns->SetStaticMesh(Item->TiledMesh);
	i = 0;
	for (EDuplicationType D : SelectedItemRule->GetDuplicateCases())
	{
		FDuplicationMod DupMod = UAutoPaintRule::GetDuplicationMod(
			D, FTiledLevelUtility::PlacedTypeToShape(Item->PlacedType), SAdj->RotationTimes, SelectedItemRule->PositionOffset);
		FIntVector SpawnPos = FIntVector(AssetPtr->X_Num + 1, 0, 0) + PosMods[i] + DupMod.PosOffset + Center;
		bool TempX, TempY, TempZ;
		THelper->ResetBrush();
		THelper->SetupPaintBrush(Item);
		if (SAdj->MirrorX) THelper->MirrorPaintBrush(EAxis::X, TempX, TempY, TempZ);
		if (SAdj->MirrorY) THelper->MirrorPaintBrush(EAxis::Y, TempX, TempY, TempZ);
		if (SAdj->MirrorZ) THelper->MirrorPaintBrush(EAxis::Z, TempX, TempY, TempZ);
		THelper->MoveBrush(SpawnPos, false); 
		bool temp;
		for (int r = 0; r < DupMod.RotationTimes; r++)
			THelper->RotateBrush(true, Item->PlacedType, temp);
		FTransform G = THelper->GetPreviewPlacementWorldTransform();
		if (Item->SourceType == ETLSourceType::Mesh)
		{
			if (IsMirrored)
			{
				AStaticMeshActor* NewMirrored = GetWorld()->SpawnActor<AStaticMeshActor>();
				NewMirrored->SetMobility(EComponentMobility::Movable);
				NewMirrored->SetActorLocationAndRotation(G.GetLocation(), G.Rotator());
				NewMirrored->SetActorScale3D(G.GetScale3D());
				NewMirrored->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
				NewMirrored->GetStaticMeshComponent()->SetStaticMesh(Item->TiledMesh);
				PreviewSpawnActors.Add(NewMirrored);
			}
			else
			{
				PreviewSpawns->AddInstance(G);
			}
		}
		else
		{
			AActor* NewActor = GetWorld()->SpawnActor(Cast<UBlueprint>(Item->TiledActor)->GeneratedClass);
			NewActor->SetActorLocationAndRotation(G.GetLocation(), G.Rotator());
			NewActor->SetActorScale3D(G.GetScale3D());
			NewActor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
			PreviewSpawnActors.Add(NewActor);
		}
		i++;
	}
	THelper->ResetBrush();
	
}
