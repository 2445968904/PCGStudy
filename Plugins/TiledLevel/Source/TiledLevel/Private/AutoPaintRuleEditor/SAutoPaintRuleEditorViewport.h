// Copyright 2022 PufStudio. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "SCommonEditorViewportToolbarBase.h"
#include "SEditorViewport.h"

class SAutoPaintRuleEditorViewport : public SEditorViewport, public ICommonEditorViewportToolbarInfoProvider
{
public:
	SLATE_BEGIN_ARGS(SAutoPaintRuleEditorViewport);
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs, TSharedPtr<class FAutoPaintRuleEditor> InEditor);
	~SAutoPaintRuleEditorViewport();

	// editor viewport interface
	virtual void BindCommands() override;
	virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;
	virtual TSharedPtr<SWidget> MakeViewportToolbar() override;
	virtual EVisibility GetTransformToolbarVisibility() const override;
	virtual void OnFocusViewportToSelection() override;
	//end of editor viewport

	// common editor viewport toolbar
	virtual TSharedRef<SEditorViewport> GetViewportWidget() override;
	virtual TSharedPtr<FExtender> GetExtenders() const override;
	virtual void OnFloatingButtonClicked() override;
	// end of common editor viewport toolbar

	void ActivateEdMode();	
	TSharedPtr<class FAdvancedPreviewScene> GetPreviewScene() const { return PreviewScene; }
	TWeakPtr<class FAutoPaintRuleEditor> GetRuleEditor() const { return RuleEditorPtr; }
private:
	TWeakPtr<FAutoPaintRuleEditor> RuleEditorPtr;

	// viewport client
	TSharedPtr<class FAutoPaintRuleEditorViewportClient> EditorViewportClient;

	// The preview scenes
    TSharedPtr<class FAdvancedPreviewScene> PreviewScene;
    class USkyLightComponent* Skylight = nullptr;
    class USkyAtmosphereComponent* AtmosphericFog = nullptr;
	
};
