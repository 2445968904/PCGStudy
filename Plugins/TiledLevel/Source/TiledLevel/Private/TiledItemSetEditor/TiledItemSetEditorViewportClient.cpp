// Copyright 2022 PufStudio. All Rights Reserved.

#include "TiledItemSetEditorViewportClient.h"
#include "AssetEditorModeManager.h"
#include "CanvasTypes.h"
#include "TiledItemSetEditor.h"

#define LOCTEXT_NAMESPACE "TiledItemSetEditor"

FTiledItemSetEditorViewportClient::FTiledItemSetEditorViewportClient(TWeakPtr<FTiledItemSetEditor> InSetEditor,
	const TWeakPtr<SEditorViewport>& InEditorViewportWidget, FPreviewScene& InPreviewScene, UTiledItemSet* InDataSource)
	:FEditorViewportClient(nullptr, &InPreviewScene, InEditorViewportWidget)
{
	 TiledItemSetEditor = InSetEditor;
	
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

void FTiledItemSetEditorViewportClient::ProcessClick(FSceneView& View, HHitProxy* HitProxy, FKey Key, EInputEvent Event,
	uint32 HitX, uint32 HitY)
{
	FEditorViewportClient::ProcessClick(View, HitProxy, Key, Event, HitX, HitY);
}

void FTiledItemSetEditorViewportClient::Tick(float DeltaSeconds)
{
	FEditorViewportClient::Tick(DeltaSeconds);
}

void FTiledItemSetEditorViewportClient::DrawCanvas(FViewport& InViewport, FSceneView& View, FCanvas& Canvas)
{
	FEditorViewportClient::DrawCanvas(InViewport, View, Canvas);
	if (TiledItemSetEditor.IsValid())
	{
		UStaticMeshComponent* PreviewMeshComponent = TiledItemSetEditor.Pin().Get()->PreviewMesh;
		if (UStaticMesh* DisplayedMesh = PreviewMeshComponent->GetStaticMesh())
		{
			int32 LOD = ComputeStaticMeshLOD(DisplayedMesh->GetRenderData(), PreviewMeshComponent->Bounds.Origin,
				PreviewMeshComponent->Bounds.SphereRadius, View, DisplayedMesh->GetMinLOD().Default);
			FString LODInfo = FString::Printf(TEXT("LOD: %d"), LOD);
			FString SizeInfo = FString::Printf(TEXT("Approx Size: %.0fx%.0fx%.0f"),
				DisplayedMesh->GetBounds().BoxExtent.X * 2.0f,
				DisplayedMesh->GetBounds().BoxExtent.Y * 2.0f,
				DisplayedMesh->GetBounds().BoxExtent.Z * 2.0f
				);

			Canvas.DrawShadowedString(10, 50, *LODInfo, GEngine->GetLargeFont(), FLinearColor::White);
			Canvas.DrawShadowedString(10, 70, *SizeInfo, GEngine->GetLargeFont(), FLinearColor::White);
		}
	}
}

#undef LOCTEXT_NAMESPACE
