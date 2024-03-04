//$ Copyright 2015-23, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Builders/GridFlow/GridFlowModel.h"

#include "Builders/GridFlow/GridFlowConfig.h"
#include "Core/Dungeon.h"
#include "Core/DungeonLayoutData.h"
#include "Frameworks/FlowImpl/GridFlow/LayoutGraph/GridFlowAbstractGraph.h"
#include "Frameworks/FlowImpl/GridFlow/Tilemap/GridFlowTilemap.h"

void UGridFlowModel::Reset() {
	if (AbstractGraph) {
		AbstractGraph->Clear();
	}

	if (Tilemap) {
		Tilemap->Clear();
	}

	TilemapBuildSetup = {};
}

void UGridFlowModel::GenerateLayoutData(const UDungeonConfig* InConfig, FDungeonLayoutData& OutLayout) const {
	const UGridFlowConfig* GridFlowConfig = Cast<UGridFlowConfig>(InConfig);
	if (!GridFlowConfig) {
		return;
	}

	// Update the world to screen
	{
		FQuat TargetRotation = FQuat::Identity;
		FVector TargetTranslation = GridFlowConfig->GridSize * FVector(TilemapBuildSetup.TilemapOffset.X, TilemapBuildSetup.TilemapOffset.Y, 0);
		if (ADungeon* Dungeon = Cast<ADungeon>(GetOuter())) {
			FTransform BaseTransform = Dungeon->GetTransform();
			TargetTranslation = BaseTransform.TransformPosition(TargetTranslation);
			TargetRotation = BaseTransform.TransformRotation(TargetRotation);
		}

		const float TargetScaleF = 1.0f /
				(FMath::Max(Tilemap->GetWidth(), Tilemap->GetHeight()) * FMath::Max(
				GridFlowConfig->GridSize.X, GridFlowConfig->GridSize.Y));

		FVector TargetScale(TargetScaleF);
		TargetTranslation *= TargetScaleF;
		OutLayout.WorldToScreen = FTransform(TargetRotation, TargetTranslation, TargetScale);
	}

	// TODO: Implement me
	
}

