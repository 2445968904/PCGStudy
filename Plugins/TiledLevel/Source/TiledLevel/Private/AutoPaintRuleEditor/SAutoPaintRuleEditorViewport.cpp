// Copyright 2022 PufStudio. All Rights Reserved.

#include "SAutoPaintRuleEditorViewport.h"
#include "AutoPaintRuleEditorViewportClient.h"
#include "AdvancedPreviewScene.h"
#include "SAutoPaintRuleEditorViewportToolbar.h"
#include "TiledItemSet.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"

#define LOCTEXT_NAMESPACE "AutoPaintRuleEditor"

SAutoPaintRuleEditorViewport::FArguments::FArguments()
{
}

void SAutoPaintRuleEditorViewport::Construct(const FArguments& InArgs, TSharedPtr<FAutoPaintRuleEditor> InEditor)
{
	RuleEditorPtr = InEditor;
	
	{
		FAdvancedPreviewScene::ConstructionValues CVS;
		CVS.bCreatePhysicsScene = true; // this is a must for line trace, the world must contain physics scene!
		CVS.LightBrightness = 2;
		CVS.SkyBrightness = 1;
		PreviewScene = MakeShareable(new FAdvancedPreviewScene(CVS));
	}
	
	SEditorViewport::Construct(SEditorViewport::FArguments());
    Skylight = NewObject<USkyLightComponent>();
    AtmosphericFog = NewObject<USkyAtmosphereComponent>();
    PreviewScene->AddComponent(AtmosphericFog, FTransform::Identity);
    PreviewScene->DirectionalLight->SetMobility(EComponentMobility::Movable);
    PreviewScene->DirectionalLight->CastShadows = true;
    PreviewScene->DirectionalLight->CastStaticShadows = true;
    PreviewScene->DirectionalLight->CastDynamicShadows = true;
    PreviewScene->DirectionalLight->SetIntensity(3);
    PreviewScene->SetSkyBrightness(1.0f);
	PreviewScene->SetFloorVisibility(false);
}

SAutoPaintRuleEditorViewport::~SAutoPaintRuleEditorViewport()
{
	if (EditorViewportClient.IsValid())
	{
		EditorViewportClient->Viewport = nullptr;
		EditorViewportClient.Reset();
	}
}

void SAutoPaintRuleEditorViewport::BindCommands()
{
	SEditorViewport::BindCommands();
}

TSharedRef<FEditorViewportClient> SAutoPaintRuleEditorViewport::MakeEditorViewportClient()
{
	EditorViewportClient = MakeShareable(new FAutoPaintRuleEditorViewportClient(RuleEditorPtr, SharedThis(this), *PreviewScene, nullptr));
	EditorViewportClient->SetRealtime(true);
	EditorViewportClient->PreviewSceneFloor = PreviewScene->GetFloorMeshComponent();
	return EditorViewportClient.ToSharedRef();
}

TSharedPtr<SWidget> SAutoPaintRuleEditorViewport::MakeViewportToolbar()
{
	return SNew(SAutoPaintRuleEditorViewportToolbar)
	.EditorViewport(SharedThis(this))
	.IsEnabled(FSlateApplication::Get().GetNormalExecutionAttribute()); // don't know what this is for...
}

EVisibility SAutoPaintRuleEditorViewport::GetTransformToolbarVisibility() const
{
	return SEditorViewport::GetTransformToolbarVisibility();
}

void SAutoPaintRuleEditorViewport::OnFocusViewportToSelection()
{
	SEditorViewport::OnFocusViewportToSelection();
}

TSharedRef<SEditorViewport> SAutoPaintRuleEditorViewport::GetViewportWidget()
{
	return SharedThis(this);
}

TSharedPtr<FExtender> SAutoPaintRuleEditorViewport::GetExtenders() const
{
	TSharedPtr<FExtender> Result(MakeShareable(new FExtender));
	return Result;
}

void SAutoPaintRuleEditorViewport::OnFloatingButtonClicked()
{
}

void SAutoPaintRuleEditorViewport::ActivateEdMode()
{
	EditorViewportClient->ActivateEdMode();
}

#undef LOCTEXT_NAMESPACE
