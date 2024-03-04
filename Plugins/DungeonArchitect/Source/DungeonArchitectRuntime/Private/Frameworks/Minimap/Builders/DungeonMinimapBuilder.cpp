//$ Copyright 2015-23, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Minimap/Builders/DungeonMinimapBuilder.h"

#include "Core/DungeonLayoutData.h"
#include "Core/Utils/MathUtils.h"
#include "Frameworks/Minimap/DungeonMinimap2D.h"

#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "TextureResource.h"

DEFINE_LOG_CATEGORY_STATIC(LogDungeonMinimapBuilder, Log, All);

namespace {
    enum ECopyRTTMaskChannel : uint8 {
        Red = 1,
        Green = 2,
        Blue = 4,
        Alpha = 8
    };

    void CopyRTTMask(UTextureRenderTarget2D* RenderTexture, TArray<FColor>& OutMask, uint8 Channel) {
        if (!RenderTexture) return;
        
        TArray<FColor> SurfaceData;
        FRenderTarget* RenderTarget = RenderTexture->GameThread_GetRenderTargetResource();
        if (!RenderTarget) return;
        
        RenderTarget->ReadPixels(SurfaceData);

        // Resize the mask array to fit the new data
        {
            int32 NumItemsToAdd = SurfaceData.Num() - OutMask.Num();
            if (NumItemsToAdd > 0) {
                OutMask.AddZeroed(NumItemsToAdd);
            }
        }

        int32 NumItems = OutMask.Num();
#define COPY_MASK(CHANNEL) for (int i = 0; i < NumItems; i++) { OutMask[i].CHANNEL = SurfaceData[i].R; }
        if (Channel & Red)
            COPY_MASK(R);
        if (Channel & Green)
            COPY_MASK(G);
        if (Channel & Blue)
            COPY_MASK(B);
        if (Channel & Alpha)
            COPY_MASK(A);
#undef COPY_MASK

    }
    
    float GetAttributeScaleMultiplier(int32 TextureSize) {
        constexpr float ReferenceTextureSize = 512;
        return TextureSize / ReferenceTextureSize;
    }
    
    void DrawMiniMapCanvasIcon(UCanvas* Canvas, UTexture2D* Icon, const FVector2D& InLocation, float ScreenSize,
                               FLinearColor InColor, float Rotation = 0.0f, EBlendMode BlendMode = BLEND_Masked) {
        if (!Canvas || !Icon) return;
        const float SizeX = ScreenSize;
        const float IconAspect = Icon->GetSizeX() / Icon->GetSizeY();
        const float SizeY = SizeX / IconAspect;
        const FVector2D Size = FVector2D(SizeX, SizeY);
        const FVector2D CanvasLocation = InLocation - Size * 0.5f;
        Canvas->K2_DrawTexture(Icon, CanvasLocation, Size,
                               FVector2D::ZeroVector, FVector2D::UnitVector, InColor, BlendMode, Rotation);
    }
    
    float GetIconPixelSize(const FDungeonMiniMapOverlayIcon2D& OverlayData, int32 TextureSize, const FVector2D& CanvasSize, const FTransform& InWorldToScreen) {
        if (OverlayData.ScreenSizeType == EDungeonMiniMapIconCoordinateSystem2D::Pixels) {
            return OverlayData.ScreenSize * GetAttributeScaleMultiplier(TextureSize);
        }
        if (OverlayData.ScreenSizeType == EDungeonMiniMapIconCoordinateSystem2D::WorldCoordinates) {
            return OverlayData.ScreenSize * InWorldToScreen.GetScale3D().X * CanvasSize.X;
        }
        return 0;
    }


}

void FDungeonMinimapBuilder::BuildLayoutTexture(UWorld* InWorld, const FLayoutBuildSettings& InSettings, const FDungeonLayoutData& LayoutData, const UDungeonModel* InDungeonModel, UTexture*& OutMaskTexture) {
    FDrawToRenderTargetContext RenderContext;
    FVector2D CanvasSize;
    UCanvas* Canvas = nullptr;

    constexpr float ReferenceTextureSize = 512; // TODO: Parameterize me
    const float AttributeScale = InSettings.TextureSize / ReferenceTextureSize;

    UTextureRenderTarget2D* RenderTexture = UKismetRenderingLibrary::CreateRenderTarget2D(InWorld, InSettings.TextureSize, InSettings.TextureSize, RTF_RGBA8);
    if (!RenderTexture) {
        UE_LOG(LogDungeonMinimapBuilder, Error, TEXT("Cannot create render texture for mini map"));
        return;
    }

    TArray<FColor> MaskData;

    // Draw the layout
    {
        UKismetRenderingLibrary::ClearRenderTarget2D(InWorld, RenderTexture, FLinearColor::Black);
        UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(InWorld, RenderTexture, Canvas, CanvasSize, RenderContext);

        if (Canvas) {
            for (const FDungeonLayoutDataBlockItem& LayoutItem : LayoutData.LayoutItems) {
                int32 NumTriangles = LayoutItem.FillTriangles.Num() / 3;
                if (NumTriangles > 0) {
                    for (int i = 0; i < NumTriangles; i++) {
                        int i0 = i * 3 + 0;
                        int i1 = i * 3 + 1;
                        int i2 = i * 3 + 2;
                        FVector2D P0 = LayoutItem.FillTriangles[i0] * CanvasSize;
                        FVector2D P1 = LayoutItem.FillTriangles[i1] * CanvasSize;
                        FVector2D P2 = LayoutItem.FillTriangles[i2] * CanvasSize;

                        FCanvasTriangleItem Triangle(P0, P1, P2, GWhiteTexture);
                        Canvas->DrawItem(Triangle);
                    }

                }
            }
        }
        UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(InWorld, RenderContext);

        // Copy to the mask data
        CopyRTTMask(RenderTexture, MaskData, Red);
    }

    // Draw the outline
    {
        float Thickness = InSettings.OutlineThickness * AttributeScale;
        UKismetRenderingLibrary::ClearRenderTarget2D(InWorld, RenderTexture, FLinearColor::Black);
        UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(InWorld, RenderTexture, Canvas, CanvasSize,
                                                               RenderContext);

        if (Canvas) {
            for (const FDungeonLayoutDataBlockItem& LayoutItem : LayoutData.LayoutItems) {
                int32 NumPoints = LayoutItem.Outline.Num();
                if (NumPoints > 0) {
                    for (int i = 0; i < NumPoints; i++) {
                        int i0 = i;
                        int i1 = (i + 1) % NumPoints;
                        FVector2D P0 = LayoutItem.Outline[i0] * CanvasSize;
                        FVector2D P1 = LayoutItem.Outline[i1] * CanvasSize;

                        Canvas->K2_DrawLine(P0, P1, Thickness);
                    }
                }
            }
        }

        UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(InWorld, RenderContext);

        // Copy to the mask data
        CopyRTTMask(RenderTexture, MaskData, Green);
    }


    // Draw the doors texture
    {
        float Thickness = InSettings.DoorThickness * AttributeScale;
        UKismetRenderingLibrary::ClearRenderTarget2D(InWorld, RenderTexture, FLinearColor::Black);
        UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(InWorld, RenderTexture, Canvas, CanvasSize,
                                                               RenderContext);
        if (Canvas) {
            for (const FDungeonLayoutDataDoorItem& Door : LayoutData.Doors) {
                int32 NumPoints = Door.Outline.Num();
                if (NumPoints > 0) {
                    for (int i = 0; i + 1 < NumPoints; i++) {
                        int i0 = i;
                        int i1 = i + 1;
                        FVector2D P0 = Door.Outline[i0] * CanvasSize;
                        FVector2D P1 = Door.Outline[i1] * CanvasSize;

                        Canvas->K2_DrawLine(P0, P1, Thickness);
                    }
                }
            }
        }
        UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(InWorld, RenderContext);

        // Copy to the mask data
        CopyRTTMask(RenderTexture, MaskData, Blue);
    }

    // We are done with the render texture and all the data has been copied over to the mask data
    UKismetRenderingLibrary::ReleaseRenderTarget2D(RenderTexture);
    RenderTexture = nullptr;

    // Blur the layout data (R channel) and save it in the alpha channel
    {
        TArray<float> BlurData0;
        BlurData0.AddUninitialized(MaskData.Num());
        for (int i = 0; i < MaskData.Num(); i++) {
            BlurData0[i] = MaskData[i].R / 255.0f;
        }

        TArray<float> BlurData1;
        BlurData1.AddUninitialized(MaskData.Num());

        TArray<float> BlurWeights;
        BlurWeights.AddZeroed(MaskData.Num());

        float* BlurData0Ptr = BlurData0.GetData();
        float* BlurData1Ptr = BlurData1.GetData();
        float* BlurWeightsPtr = BlurWeights.GetData();
        const int32 SizeX = InSettings.TextureSize;
        const int32 SizeY = InSettings.TextureSize;

        float ScaledBlurRadius = InSettings.BlurRadius * AttributeScale;

        TArray<int32> BlurKernels = BlurUtils::boxesForGauss(ScaledBlurRadius, InSettings.BlurIterations);
        float* SourceData = BlurData0Ptr;
        float* TargetData = BlurData1Ptr;
        for (int i = 0; i < BlurKernels.Num(); i++) {
            BlurUtils::boxBlur_4(SourceData, TargetData, BlurWeightsPtr, SizeX, SizeY, (BlurKernels[i] - 1) / 2);
            // Swap source / target
            float* Temp = SourceData;
            SourceData = TargetData;
            TargetData = Temp;
        }

        for (int i = 0; i < MaskData.Num(); i++) {
            MaskData[i].A = FMath::RoundToInt(SourceData[i] * 255);
        }
    }

    OutMaskTexture = UTexture2D::CreateTransient(InSettings.TextureSize, InSettings.TextureSize, PF_B8G8R8A8);
#if WITH_EDITORONLY_DATA
    OutMaskTexture->MipGenSettings = TMGS_NoMipmaps;
#endif
    OutMaskTexture->SRGB = 0;
    
    // Update the mask texture
    if (UTexture2D* MaskTex2D = Cast<UTexture2D>(OutMaskTexture)) {
        FByteBulkData* BulkData = &MaskTex2D->GetPlatformData()->Mips[0].BulkData;
        void* TextureData = BulkData->Lock(LOCK_READ_WRITE);
        int32 NumBytes = MaskData.Num() * sizeof(FColor);
        FMemory::Memcpy(TextureData, MaskData.GetData(), NumBytes);
        BulkData->Unlock();
        MaskTex2D->UpdateResource();
    }
}

void FDungeonMinimapBuilder::BuildStaticOverlayTexture(UWorld* InWorld, UTextureRenderTarget2D* InTexture, const FDungeonLayoutData& LayoutData,
        const TArray<FDungeonMiniMapOverlayIcon2D>& OverlayIcons) const {
    TMap<FName, FDungeonMiniMapOverlayIcon2D> OverlayMap;
    for (const FDungeonMiniMapOverlayIcon2D& OverlayItem : OverlayIcons) {
        if (OverlayItem.Icon && !OverlayMap.Contains(OverlayItem.Name)) {
            OverlayMap.Add(OverlayItem.Name, OverlayItem);
        }
    }
    // Grab the items that have icons defined for it
    TArray<FDungeonLayoutDataPointOfInterest> PointsOfInterest;
    for (const FDungeonLayoutDataPointOfInterest& PointOfInterest : LayoutData.PointsOfInterest) {
        if (OverlayMap.Contains(PointOfInterest.Id)) {
            PointsOfInterest.Add(PointOfInterest);
        }
    }

    FDrawToRenderTargetContext RenderContext;
    FVector2D CanvasSize;
    UCanvas* Canvas = nullptr;
    UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(InWorld, InTexture, Canvas, CanvasSize, RenderContext);
    if (Canvas) {
        for (const FDungeonLayoutDataPointOfInterest& PointOfInterest : PointsOfInterest) {
            if (FDungeonMiniMapOverlayIcon2D* SearchResult = OverlayMap.Find(PointOfInterest.Id)) {
                FDungeonMiniMapOverlayIcon2D& OverlayData = *SearchResult;
                const float Rotation = OverlayData.Rotation;
                FVector2D Location = PointOfInterest.Location * CanvasSize;
                const float ScreenSize = GetIconPixelSize(OverlayData, InTexture->GetSurfaceWidth(), CanvasSize, LayoutData.WorldToScreen);
                DrawMiniMapCanvasIcon(Canvas, OverlayData.Icon, Location, ScreenSize, OverlayData.Tint, Rotation);
            }
        }
    }
    UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(InWorld, RenderContext);
}

void FDungeonMinimapBuilder::UpdateDynamicOverlayTexture(UWorld* InWorld, UTextureRenderTarget2D* DynamicOverlayTexture,
        const FTransform& WorldToScreen, TArray<FDungeonMiniMapOverlayTracking> DynamicTracking, TArray<FDungeonMiniMapOverlayIcon2D> OverlayIcons) {
    if (!DynamicOverlayTexture) {
        return;
    }

    UKismetRenderingLibrary::ClearRenderTarget2D(InWorld, DynamicOverlayTexture, FLinearColor::Transparent);

    if (DynamicTracking.Num() == 0) {
        return;
    }

    TMap<FName, FDungeonMiniMapOverlayIcon2D> OverlayMap;
    for (const FDungeonMiniMapOverlayIcon2D& OverlayItem : OverlayIcons) {
        if (OverlayItem.Icon && !OverlayMap.Contains(OverlayItem.Name)) {
            OverlayMap.Add(OverlayItem.Name, OverlayItem);
        }
    }


    FDrawToRenderTargetContext RenderContext;
    FVector2D CanvasSize;
    UCanvas* Canvas = nullptr;
    UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(InWorld, DynamicOverlayTexture, Canvas, CanvasSize,
                                                           RenderContext);
    if (Canvas) {
        for (const FDungeonMiniMapOverlayTracking& Overlay : DynamicTracking) {
            if (Overlay.TrackedActor.IsValid()) {
                FVector WorldLocation = Overlay.TrackedActor->GetActorLocation();
                const FVector ScreenLocation = WorldToScreen.TransformPosition(WorldLocation);
                FVector2D ScreenLocation2D(ScreenLocation.X, ScreenLocation.Y);

                if (FDungeonMiniMapOverlayIcon2D* SearchResult = OverlayMap.Find(Overlay.IconName)) {
                    FDungeonMiniMapOverlayIcon2D& OverlayData = *SearchResult;
                    float Rotation = OverlayData.Rotation;
                    if (Overlay.bOrientToRotation) {
                        const FVector Angles = Overlay.TrackedActor->GetActorRotation().Euler();
                        Rotation += Angles.Z;
                    }

                    FVector2D CanvasLocation = ScreenLocation2D * CanvasSize;
                    const float ScreenSize = GetIconPixelSize(OverlayData, DynamicOverlayTexture->GetSurfaceWidth(), CanvasSize, WorldToScreen);
                    DrawMiniMapCanvasIcon(Canvas, OverlayData.Icon, CanvasLocation, ScreenSize, OverlayData.Tint,
                                          Rotation);
                }
            }
        }
    }
    UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(InWorld, RenderContext);
}

void FDungeonMinimapBuilder::UpdateFogOfWarTexture(UWorld* InWorld, UTextureRenderTarget2D* InFogOfWarTexture, const TArray<FDungeonMiniMapOverlayTracking>& InTrackedObjects, const FFogOfWarUpdateSettings& InSettings) const {
    TWeakObjectPtr<AActor> TrackedActor;
    for (const FDungeonMiniMapOverlayTracking& Overlay : InTrackedObjects) {
        if (Overlay.Id == InSettings.FogOfWarTrackingItem && Overlay.TrackedActor.IsValid()) {
            TrackedActor = Overlay.TrackedActor;
            break;
        }
    }

    if (TrackedActor.IsValid()) {
        FDrawToRenderTargetContext RenderContext;
        FVector2D CanvasSize;
        UCanvas* Canvas = nullptr;
        UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(InWorld, InFogOfWarTexture, Canvas, CanvasSize, RenderContext);
        if (Canvas) {
            const FVector WorldLocation = TrackedActor->GetActorLocation();
            const FVector ScreenLocation = InSettings.WorldToScreen.TransformPosition(WorldLocation);
            const FVector2D ScreenLocation2D(ScreenLocation.X, ScreenLocation.Y);

            const FVector2D CanvasLocation = ScreenLocation2D * CanvasSize;
            const float ScreenSize = InSettings.FogOfWarVisibilityDistance * InSettings.WorldToScreen.GetScale3D().X * CanvasSize.X;
            DrawMiniMapCanvasIcon(Canvas, InSettings.FogOfWarExploreTexture, CanvasLocation, ScreenSize, FLinearColor::White, 0, BLEND_Additive);
        }
        UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(InWorld, RenderContext);
    }
}

