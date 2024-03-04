// Copyright 2022 PufStudio. All Rights Reserved.

#include "AutoPaintRuleEditorViewportClient.h"
#include "AssetEditorModeManager.h"
#include "AutoPaintRule.h"
#include "AutoPaintRuleEditor.h"
#include "TiledLevel.h"
#include "TiledLevelEditorHelper.h"
#include "Components/TextRenderComponent.h"
#include "Engine/TextRenderActor.h"
#include "TiledLevelEditor/TiledLevelEdMode.h"

#define LOCTEXT_NAMESPACE "AutoPaintRuleEditor"

FAutoPaintRuleEditorViewportClient::FAutoPaintRuleEditorViewportClient(TWeakPtr<FAutoPaintRuleEditor> InRuleEditor,
	const TWeakPtr<SEditorViewport>& InEditorViewportWidget, FPreviewScene& InPreviewScene, UTiledItemSet* InDataSource)
	:FEditorViewportClient(nullptr, &InPreviewScene, InEditorViewportWidget)
{
	 RuleEditor = InRuleEditor;
	
	// Setup defaults for the common draw helper.
     DrawHelper.bDrawPivot = false;
     DrawHelper.bDrawWorldBox = false;
     DrawHelper.bDrawKillZ = false;
     DrawHelper.bDrawGrid = true;
     DrawHelper.GridColorAxis = FColor(160, 160, 160);
     DrawHelper.GridColorMajor = FColor(144, 144, 144);
     DrawHelper.GridColorMinor = FColor(128, 128, 128);
     DrawHelper.PerspectiveGridSize = 250;
     DrawHelper.NumCells = DrawHelper.PerspectiveGridSize / (32 * 2);
     SetViewMode(VMI_Lit);
   
     //EngineShowFlags.DisableAdvancedFeatures();
     EngineShowFlags.SetSnap(false);
     EngineShowFlags.CompositeEditorPrimitives = true;
     OverrideNearClipPlane(1.0f);
     bUsingOrbitCamera = true;
 
	SetViewRotation(FRotator(-30, 180, 0));
    SetViewLocation(FVector(1000, -100, 500));
}

FAutoPaintRuleEditorViewportClient::~FAutoPaintRuleEditorViewportClient()
{
	if (EdMode)
	{
		if (TempAsset)
		{
			Rule->PreviewData = TempAsset->GetAutoPaintData();
			Rule->X_Num = TempAsset->X_Num;
			Rule->Y_Num = TempAsset->Y_Num;
			Rule->Floor_Num = TempAsset->TiledFloors.Num();
			Rule->StartFloor = TempAsset->GetBottomFloor().FloorPosition;
		}
		EdMode->Exit();
	}
}

void FAutoPaintRuleEditorViewportClient::ActivateEdMode()
{
	if (ModeTools)
	{
		ModeTools->SetToolkitHost(RuleEditor.Pin()->GetToolkitHost());
		ModeTools->SetDefaultMode(FTiledLevelEdMode::EM_TiledLevelEdModeId);
		ModeTools->ActivateDefaultMode();
		UWorld* World = PreviewScene->GetWorld();
		ATiledLevel* LevelActor = World->SpawnActor<ATiledLevel>(ATiledLevel::StaticClass());
		ATiledLevelEditorHelper* Helper = World->SpawnActor<ATiledLevelEditorHelper>(ATiledLevelEditorHelper::StaticClass());
		
		EdMode = static_cast<FTiledLevelEdMode*>(ModeTools->GetActiveMode(FTiledLevelEdMode::EM_TiledLevelEdModeId));
		if (EdMode)
		{
			RuleEditor.Pin()->SetContentFromEdMode(EdMode);
			Rule = RuleEditor.Pin()->GetRuleBeingEdited();
			
			// create a new temp tiled level asset...
			UTiledItemSet* ActiveSet = Rule->SourceItemSet;
			TempAsset = NewObject<UTiledLevelAsset>(Rule, UTiledLevelAsset::StaticClass(), TEXT("Temp"), RF_Transactional);
			TempAsset->X_Num = Rule->X_Num;
			TempAsset->Y_Num = Rule->Y_Num;
			for (int i = 0; i < Rule->Floor_Num; i++)
			{
				TempAsset->AddNewFloor(i);
			}
			// move to lowest floor
			if (Rule->StartFloor != 0)
			{
				const bool bUp = Rule->StartFloor > 0;
				for (int i = 0; i < FMath::Abs(Rule->StartFloor); i++ )
				{
					TempAsset->MoveAllFloors(bUp);
				}
			}
			TempAsset->ActiveFloorPosition = TempAsset->GetBottomFloor().FloorPosition;
			TempAsset->SetTileSize(ActiveSet->GetTileSize());
			TempAsset->SetActiveItemSet(ActiveSet);
			TempAsset->SetActiveAutoPaintRule(Rule);
			TempAsset->SetAutoPaintData(Rule->PreviewData);
			TempAsset->EdMode = EdMode;
			EdMode->SetupData(TempAsset, LevelActor, Helper, PreviewSceneFloor, false, true);

			// Spawn other hint actors 
			ATextRenderActor* Hint1 = World->SpawnActor<ATextRenderActor>(ATextRenderActor::StaticClass());
			Hint1->GetTextRender()->SetText(FText::FromString("Test Paint Area"));
			Hint1->GetTextRender()->HorizontalAlignment = EHorizTextAligment::EHTA_Center;
			Hint1->GetTextRender()->TextRenderColor = FColor(255, 64, 255);
			Hint1->GetTextRender()->SetWorldSize(128);
			Hint1->SetActorLocation(FVector(ActiveSet->GetTileSize().X * TempAsset->X_Num * 0.5 , -ActiveSet->GetTileSize().Y, ActiveSet->GetTileSize().Z * 0.5f));
			Hint1->SetActorRotation(FRotator(0, 90, 0));
			
			ATextRenderActor* Hint2 = World->SpawnActor<ATextRenderActor>(ATextRenderActor::StaticClass());
			Hint2->GetTextRender()->SetText(FText::FromString("Rule Item Demo"));
			Hint2->GetTextRender()->HorizontalAlignment = EHorizTextAligment::EHTA_Center;
			Hint2->GetTextRender()->TextRenderColor = FColor(255, 64, 255);
			Hint2->GetTextRender()->SetWorldSize(128);
			Hint2->SetActorLocation(FVector(ActiveSet->GetTileSize().X * (TempAsset->X_Num + 5)  , -ActiveSet->GetTileSize().Y, ActiveSet->GetTileSize().Z * 0.5f));
			Hint2->SetActorRotation(FRotator(0, 90, 0));

			TempAsset->OnTiledLevelAreaChanged.AddLambda([=](float InX, float InY, float InZ, int InNumX, int InNumY, int InNumZ, int FloorStart)
			{
				 Hint1->SetActorLocation(FVector(ActiveSet->GetTileSize().X * InNumX * 0.5 , -ActiveSet->GetTileSize().Y, ActiveSet->GetTileSize().Z * 0.5f));
				 Hint2->SetActorLocation(FVector(ActiveSet->GetTileSize().X * (InNumX + 10)  , -ActiveSet->GetTileSize().Y, ActiveSet->GetTileSize().Z * 0.5f));
			});
		}
	}
}

#undef LOCTEXT_NAMESPACE
