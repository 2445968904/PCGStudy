﻿// Copyright 2022 PufStudio. All Rights Reserved.
 
 #pragma once
 #include "CoreMinimal.h"
 #include "Styling/SlateStyle.h"
 
 class FTiledLevelStyle
 {
 public:
 
 	static void Initialize();
 	static void Shutdown();
 
 	/** reloads textures used by slate renderer */
 	static void ReloadTextures();
 
 	/** @return The Slate style set for the Shooter game */
 	static const ISlateStyle& Get();
 
 	static FName GetStyleSetName();
 
 private:
 	
 	static TSharedRef< class FSlateStyleSet > Create();
 	static TSharedPtr< class FSlateStyleSet > StyleInstance;
 };
