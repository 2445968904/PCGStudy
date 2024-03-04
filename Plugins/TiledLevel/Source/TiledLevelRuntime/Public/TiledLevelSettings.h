// Copyright 2022 PufStudio. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "TiledLevelTypes.h"
#include "UObject/Object.h"
#include "TiledLevelSettings.generated.h"

UCLASS(config = EditorSettings)
class TILEDLEVELRUNTIME_API UTiledLevelSettings : public UObject
{
	GENERATED_BODY()

public:
	
	UPROPERTY(EditAnywhere, Config, Category="Appearance")
	FLinearColor SpecialItemColor = FLinearColor(0.1,0.1,0.1, 1);
	UPROPERTY(EditAnywhere, Config, Category="Appearance")
	FLinearColor StructureMeshItemColor = FLinearColor(0.341,0.761,0.553, 1);
	UPROPERTY(EditAnywhere, Config, Category="Appearance")
	FLinearColor StructureActorItemColor = FLinearColor(0.207,0.459,0.333, 1);
	UPROPERTY(EditAnywhere, Config, Category="Appearance")
	FLinearColor PropMeshItemColor = FLinearColor(0.0490,0.047,0.271, 1);
	UPROPERTY(EditAnywhere, Config, Category="Appearance")
	FLinearColor PropActorItemColor = FLinearColor(0.188,0.020,0.106, 1);

	// If you are using c++ and try to debug your c++ code, this plugin may crash your project due to an issue I just can't fix perfectly.
	// The cause seemed to be adding custom data to HierarchicalInstancedStaticMeshComponent when editor startup...
	// The hacking approach to avoid it is "turn on" this option to turn off adding custom value, however, this may result in Eraser Tool malfunctioning.
	// Once you finished debugging your c++ code, turn off this option to make all the functionality back.
	UPROPERTY(EditAnywhere, Config, Category="Debug")
	bool bBlockAddCustomData = false;

	// not exposed in config
	
	// Tiled Palette setup
	UPROPERTY()
	float ThumbnailScale = 0.5f;
	UPROPERTY()
	FText PaletteSearchText;
	UPROPERTY()
	bool IsItemsVisible_Block = true;
	UPROPERTY()
	bool IsItemsVisible_Floor = true;
	UPROPERTY()
	bool IsItemsVisible_Wall = true;
	UPROPERTY()
	bool IsItemsVisible_Edge = true;
	UPROPERTY()
	bool IsItemsVisible_Pillar = true;
	UPROPERTY()
	bool IsItemsVisible_Point = true;
	UPROPERTY()
	bool IsItemsVisible_Structure = true;
	UPROPERTY()
	bool IsItemsVisible_Prop = true;
	UPROPERTY()
	bool IsItemsVisible_Actor = true;
	UPROPERTY()
	bool IsItemsVisible_Mesh = true;
	UPROPERTY()
	bool IsItemsVisible_Restriction = true;
	UPROPERTY()
	bool IsItemsVisible_Template = true;

	UPROPERTY()
	uint8 StepSize = 1;

	UPROPERTY()
	FIntPoint StartPoint = {0, 0};

	UPROPERTY()
	TMap<EPlacedType, EPivotPosition> DefaultPlacedTypePivotPositionMap =
	{
		{EPlacedType::Block, EPivotPosition::Bottom},
		{EPlacedType::Floor, EPivotPosition::Bottom},
		{EPlacedType::Wall, EPivotPosition::Bottom},
		{EPlacedType::Edge, EPivotPosition::Center},
		{EPlacedType::Pillar, EPivotPosition::Bottom},
		{EPlacedType::Point, EPivotPosition::Center},
	}
	;
	
	void ResetTiledPaletteSettings();


};