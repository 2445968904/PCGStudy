//$ Copyright 2015-23, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Utils/Debug/SceneDebugDataComponent.h"

#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "ComponentVisualizer.h"
#include "Engine/Font.h"

class FDASceneDebugDataComponentVisualizer : public FComponentVisualizer {
public:
	virtual void DrawVisualizationHUD(const UActorComponent* InComponent, const FViewport* Viewport, const FSceneView* View, FCanvas* Canvas) override;
	virtual void DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI) override;
};

