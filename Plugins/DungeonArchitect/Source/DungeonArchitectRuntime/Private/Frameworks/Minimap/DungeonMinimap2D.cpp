//$ Copyright 2015-23, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Minimap/DungeonMinimap2D.h"

#include "Core/Dungeon.h"
#include "Core/DungeonLayoutData.h"
#include "Core/DungeonModel.h"
#include "Frameworks/Minimap/Builders/DungeonMinimapBuilder.h"
#include "Frameworks/Minimap/Builders/DungeonMinimapBuilderFactory.h"

#include "Engine/TextureRenderTarget2D.h"
#include "EngineUtils.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "RenderUtils.h"
#include "TextureResource.h"

DEFINE_LOG_CATEGORY_STATIC(LogDungeonMiniMap, Log, All);

ADungeonMinimapBase::ADungeonMinimapBase() {
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;

    if (!IsRunningCommandlet())
    {
        struct FConstructorStatics
        {
            ConstructorHelpers::FObjectFinderOptional<UTexture2D> Texture;
            FConstructorStatics()
                : Texture(TEXT("/DungeonArchitect/Core/Materials/MiniMap/Textures/FogOfWarMask"))
            {
            }
        };
        static FConstructorStatics ConstructorStatics;

        FogOfWarExploreTexture = ConstructorStatics.Texture.Get();
    }
}

void ADungeonMinimapBase::RecreateFogOfWarTexture() {
    if (FogOfWarTexture) {
        UKismetRenderingLibrary::ReleaseRenderTarget2D(FogOfWarTexture);
    }
    FogOfWarTexture = UKismetRenderingLibrary::CreateRenderTarget2D(GetWorld(), FogOfWarTextureSize, FogOfWarTextureSize, RTF_RGBA8);
    UKismetRenderingLibrary::ClearRenderTarget2D(GetWorld(), FogOfWarTexture, bEnableFogOfWar ? FLinearColor::Black : FLinearColor::White);

    UpdateFogOfWarTexture();
}

void ADungeonMinimapBase::UpdateFogOfWarTexture() const {
    if (bEnableFogOfWar && MinimapBuilder.IsValid()) {
        FDungeonMinimapBuilder::FFogOfWarUpdateSettings UpdateSettings;
        UpdateSettings.WorldToScreen = WorldToScreen;
        UpdateSettings.FogOfWarTrackingItem = FogOfWarTrackingItem;
        UpdateSettings.FogOfWarExploreTexture = FogOfWarExploreTexture;
        UpdateSettings.FogOfWarVisibilityDistance = FogOfWarVisibilityDistance;
        MinimapBuilder->UpdateFogOfWarTexture(GetWorld(), FogOfWarTexture, DynamicTracking, UpdateSettings);
    }
}


ADungeonMinimap2D::ADungeonMinimap2D() {
    USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>("SceneRoot");
    RootComponent = SceneRoot;
    MaterialInstance = nullptr;
    
    if (!IsRunningCommandlet()) {
        // TODO: Place a default
        //ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialTemplateRef(TEXT("/DungeonArchitect/Core/Materials/MiniMap/Material/M_MMTemplate_Inst"));
        //MaterialTemplate = MaterialTemplateRef.Object;
    }
    
}

UMaterialInterface* ADungeonMinimap2D::CreateMaterialInstance() {
    if (!MaterialTemplate) return nullptr;
    return CreateMaterialInstanceFromTemplate(MaterialTemplate);
}

UMaterialInterface* ADungeonMinimap2D::CreateMaterialInstanceFromTemplate(UMaterialInterface* InMaterialTemplate) {
    UMaterialInstanceDynamic* NewMaterialInstance = UMaterialInstanceDynamic::Create(InMaterialTemplate, this);
    UpdateMaterial(NewMaterialInstance);
    return NewMaterialInstance;
}

void ADungeonMinimap2D::UpdateMaterial(UMaterialInterface* InMaterial) {
    if (UMaterialInstanceDynamic* NewMaterialInstance = Cast<UMaterialInstanceDynamic>(InMaterial)) {
        NewMaterialInstance->SetTextureParameterValue("MaskTex", MaskTexture);
        NewMaterialInstance->SetTextureParameterValue("StaticOverlayTex", StaticOverlayTexture);
        NewMaterialInstance->SetTextureParameterValue("DynamicOverlayTex", DynamicOverlayTexture);
        NewMaterialInstance->SetTextureParameterValue("FogOfWarTex", FogOfWarTexture);
    }
}

void ADungeonMinimap2D::BuildLayout(ADungeon* Dungeon) {
    if (!Dungeon) {
        UE_LOG(LogDungeonMiniMap, Error, TEXT("Cannot build minimap layout. missing dungeon actor"));
        return;
    }
    
    const UDungeonModel* DungeonModel = Dungeon->GetModel();
    const UDungeonConfig* DungeonConfig = Dungeon->GetConfig();
    const UDungeonBuilder* DungeonBuilder = Dungeon->GetBuilder();
    if (!DungeonModel || !DungeonConfig || !DungeonBuilder) {
        UE_LOG(LogDungeonMiniMap, Error, TEXT("Cannot build minimap layout. Invalid dungeon state"));
        return;
    }

    MinimapBuilder = FDungeonMinimapBuilderFactory::Create(DungeonBuilder->GetClass());
    if (!MinimapBuilder.IsValid()) {
        UE_LOG(LogDungeonMiniMap, Error, TEXT("Cannot build minimap layout. Invalid minimap builder"));
        return;
    }

    UWorld* World = GetWorld();
    FDungeonLayoutData LayoutData;
    DungeonModel->GenerateLayoutData(DungeonConfig, LayoutData);
    WorldToScreen = LayoutData.WorldToScreen;
    
    FDungeonMinimapBuilder::FLayoutBuildSettings LayoutBuildSettings;
    LayoutBuildSettings.TextureSize = TextureSize;
    LayoutBuildSettings.OutlineThickness = OutlineThickness;
    LayoutBuildSettings.DoorThickness = DoorThickness;
    LayoutBuildSettings.BlurRadius = BlurRadius;
    LayoutBuildSettings.BlurIterations = BlurIterations;
    
    // Create the layout mask texture
    MinimapBuilder->BuildLayoutTexture(World, LayoutBuildSettings, LayoutData, DungeonModel, MaskTexture);

    // Build the static texture
    {
        if (StaticOverlayTexture) {
            UKismetRenderingLibrary::ReleaseRenderTarget2D(StaticOverlayTexture);
            StaticOverlayTexture = nullptr;
        }
        StaticOverlayTexture = UKismetRenderingLibrary::CreateRenderTarget2D(World, TextureSize, TextureSize, RTF_RGBA8);
        UKismetRenderingLibrary::ClearRenderTarget2D(World, StaticOverlayTexture, FLinearColor::Transparent);
        MinimapBuilder->BuildStaticOverlayTexture(World, StaticOverlayTexture, LayoutData, OverlayIcons);
    }

    // Build the dynamic overlay texture
    if (DynamicOverlayTexture) {
        UKismetRenderingLibrary::ReleaseRenderTarget2D(DynamicOverlayTexture);
    }
    DynamicOverlayTexture = UKismetRenderingLibrary::CreateRenderTarget2D(World, TextureSize, TextureSize, RTF_RGBA8);
    MinimapBuilder->UpdateDynamicOverlayTexture(World, DynamicOverlayTexture, WorldToScreen, DynamicTracking, OverlayIcons);
    RecreateFogOfWarTexture();

    MaterialInstance = CreateMaterialInstance();
    UpdateMaterial(MaterialInstance);
}

void ADungeonMinimap2D::BeginDestroy() {
    Super::BeginDestroy();

    if (DynamicOverlayTexture) {
        UKismetRenderingLibrary::ReleaseRenderTarget2D(DynamicOverlayTexture);
        DynamicOverlayTexture = nullptr;
    }
}

void ADungeonMinimap2D::Tick(float DeltaSeconds) {
    Super::Tick(DeltaSeconds);

    if (MinimapBuilder.IsValid()) {
        MinimapBuilder->UpdateDynamicOverlayTexture(GetWorld(), DynamicOverlayTexture, WorldToScreen, DynamicTracking, OverlayIcons);
    }
    
    UpdateFogOfWarTexture();
}

float ADungeonMinimap2D::GetIconPixelSize(const FDungeonMiniMapOverlayIcon2D& OverlayData, const FVector2D& CanvasSize) const {
    if (OverlayData.ScreenSizeType == EDungeonMiniMapIconCoordinateSystem2D::Pixels) {
        return OverlayData.ScreenSize * GetAttributeScaleMultiplier();
    }
    if (OverlayData.ScreenSizeType == EDungeonMiniMapIconCoordinateSystem2D::WorldCoordinates) {
        return OverlayData.ScreenSize * WorldToScreen.GetScale3D().X * CanvasSize.X;
    }
    return 0;
}

float ADungeonMinimap2D::GetAttributeScaleMultiplier() const {
    constexpr float ReferenceTextureSize = 512;
    return TextureSize / ReferenceTextureSize;
}

void UDungeonMiniMapTrackedObject::BeginPlay() {
    Super::BeginPlay();

    // Find the Minimap component
    for (TActorIterator<ADungeonMinimapBase> It(GetWorld()); It; ++It) {
        if (ADungeonMinimapBase* MiniMap = *It) {
            FDungeonMiniMapOverlayTracking TrackingInfo;
            TrackingInfo.TrackedActor = GetOwner();
            TrackingInfo.Id = Id;
            TrackingInfo.IconName = IconName;
            TrackingInfo.bOrientToRotation = bOrientToRotation;
            MiniMap->DynamicTracking.Add(TrackingInfo);
            break;
        }
    }
}

