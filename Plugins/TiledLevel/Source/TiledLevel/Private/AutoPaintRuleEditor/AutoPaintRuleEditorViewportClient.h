// Copyright 2022 PufStudio. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "TiledLevelAsset.h"

class FAutoPaintRuleEditorViewportClient : public FEditorViewportClient
{
public:
    FAutoPaintRuleEditorViewportClient(
    	TWeakPtr<class FAutoPaintRuleEditor> InRuleEditor,
    	const TWeakPtr<class SEditorViewport>& InEditorViewportWidget,
    	FPreviewScene& InPreviewScene,
    	class UTiledItemSet* InDataSource
    );
	
	virtual ~FAutoPaintRuleEditorViewportClient() override;

	void ActivateEdMode();
	const UStaticMeshComponent* PreviewSceneFloor;
	
private:
	TWeakPtr<class FAutoPaintRuleEditor> RuleEditor;
	UAutoPaintRule* Rule;
	UTiledLevelAsset* TempAsset;
	class FTiledLevelEdMode* EdMode = nullptr;
};
