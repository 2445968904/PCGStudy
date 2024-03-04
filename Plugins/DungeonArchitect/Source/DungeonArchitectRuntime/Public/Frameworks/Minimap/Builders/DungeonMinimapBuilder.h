//$ Copyright 2015-23, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"

struct FDungeonMiniMapOverlayTracking;
struct FDungeonMiniMapOverlayIcon2D;
class UDungeonModel;
class FDungeonLayoutData;
class UTexture;
class UTexture2D;
class UTextureRenderTarget2D;

class FDungeonMinimapBuilder {
public:
	virtual ~FDungeonMinimapBuilder() = default;

	struct FLayoutBuildSettings {
		int32 TextureSize{};
		float OutlineThickness{};
		float DoorThickness{};
		float BlurRadius{};
		int32 BlurIterations{};
	};
	
	virtual void BuildLayoutTexture(UWorld* InWorld, const FLayoutBuildSettings& InSettings, const FDungeonLayoutData& LayoutData, const UDungeonModel* InDungeonModel, UTexture*& OutMaskTexture);
	virtual void BuildStaticOverlayTexture(UWorld* InWorld, UTextureRenderTarget2D* InTexture, const FDungeonLayoutData& LayoutData,
			const TArray<FDungeonMiniMapOverlayIcon2D>& OverlayIcons) const;
	virtual void UpdateDynamicOverlayTexture(UWorld* InWorld, UTextureRenderTarget2D* DynamicOverlayTexture,
		const FTransform& WorldToScreen, TArray<FDungeonMiniMapOverlayTracking> DynamicTracking, TArray<FDungeonMiniMapOverlayIcon2D> OverlayIcons);

	struct FFogOfWarUpdateSettings {
		FTransform WorldToScreen{};
		FName FogOfWarTrackingItem{};
		UTexture2D* FogOfWarExploreTexture{};
		float FogOfWarVisibilityDistance{};
	};
	virtual void UpdateFogOfWarTexture(UWorld* InWorld, UTextureRenderTarget2D* InFogOfWarTexture, const TArray<FDungeonMiniMapOverlayTracking>& InTrackedObjects, const FFogOfWarUpdateSettings& InSettings) const;
};
typedef TSharedPtr<FDungeonMinimapBuilder> FDungeonMinimapBuilderPtr;

