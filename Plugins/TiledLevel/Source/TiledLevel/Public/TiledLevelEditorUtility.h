// Copyright 2022 PufStudio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
class TILEDLEVEL_API FTiledLevelEditorUtility
{
public:
	static class UStaticMesh* MergeTiledLevelAsset(class UTiledLevelAsset* TargetAsset);
	
	static void ConfigThumbnailAssetColor(struct FAssetThumbnailConfig& ThumbnailConfig, class UTiledLevelItem* Item);
 };
 