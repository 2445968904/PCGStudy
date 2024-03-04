//$ Copyright 2015-23, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "DungeonMinimap2D.generated.h"

class UTextureRenderTarget2D;
class UDungeonModel;
class UDungeonConfig;
class FDungeonLayoutData;
class UTexture;
class UTexture2D;
typedef TSharedPtr<class FDungeonMinimapBuilder> FDungeonMinimapBuilderPtr;

//////////////////////// Object Tracking ////////////////////////
USTRUCT(BlueprintType)
struct DUNGEONARCHITECTRUNTIME_API FDungeonMiniMapOverlayTracking {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MiniMap")
    TWeakObjectPtr<AActor> TrackedActor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MiniMap")
    FName Id;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MiniMap")
    FName IconName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MiniMap")
    bool bOrientToRotation = false;
};

UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent))
class DUNGEONARCHITECTRUNTIME_API UDungeonMiniMapTrackedObject : public UActorComponent {
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MiniMap")
    FName Id;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MiniMap")
    FName IconName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MiniMap")
    bool bOrientToRotation = false;

public:
    virtual void BeginPlay() override;
};


//////////////////////// 2D Minimap ////////////////////////

UENUM()
enum class EDungeonMiniMapIconCoordinateSystem2D : uint8 {
    Pixels = 0 UMETA(DisplayName = "Pixels"),
    WorldCoordinates UMETA(DisplayName = "World Coordinates"),
};

USTRUCT()
struct DUNGEONARCHITECTRUNTIME_API FDungeonMiniMapOverlayIcon2D {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "MiniMap")
    FName Name;

    UPROPERTY(EditAnywhere, Category = "MiniMap")
    UTexture2D* Icon = nullptr;

    UPROPERTY(EditAnywhere, Category = "MiniMap")
    float ScreenSize = 32.0f;

    UPROPERTY(EditAnywhere, Category = "MiniMap")
    EDungeonMiniMapIconCoordinateSystem2D ScreenSizeType = EDungeonMiniMapIconCoordinateSystem2D::Pixels;

    UPROPERTY(EditAnywhere, Category = "MiniMap")
    FLinearColor Tint = FLinearColor::White;

    UPROPERTY(EditAnywhere, Category = "MiniMap")
    float Rotation = 0.0f;
};

UCLASS(Abstract, HideDropdown)
class DUNGEONARCHITECTRUNTIME_API ADungeonMinimapBase : public AActor {
    GENERATED_BODY()
public:
    ADungeonMinimapBase();
    
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MiniMap")
    TArray<FDungeonMiniMapOverlayTracking> DynamicTracking;

    UPROPERTY(BlueprintReadOnly, Category = "MiniMap")
    FTransform WorldToScreen;
    
    UPROPERTY(EditAnywhere, Category = "Fog Of War")
    bool bEnableFogOfWar = false;

    UPROPERTY(EditAnywhere, Category = "Fog Of War", meta = (EditCondition = bEnableFogOfWar))
    int32 FogOfWarTextureSize = 512;

    UPROPERTY(EditAnywhere, Category = "Fog Of War", meta = (EditCondition = bEnableFogOfWar))
    FName FogOfWarTrackingItem = "Player";

    UPROPERTY(EditAnywhere, Category = "Fog Of War", meta = (EditCondition = bEnableFogOfWar))
    UTexture2D* FogOfWarExploreTexture;

    UPROPERTY(EditAnywhere, Category = "Fog Of War", meta = (EditCondition = bEnableFogOfWar))
    float FogOfWarVisibilityDistance = 5000.0f;

    UPROPERTY(Transient)
    UTextureRenderTarget2D* FogOfWarTexture;
    
protected:
    FDungeonMinimapBuilderPtr MinimapBuilder;
    
protected:
    void UpdateFogOfWarTexture() const;
    void RecreateFogOfWarTexture();
};

UCLASS()
class DUNGEONARCHITECTRUNTIME_API ADungeonMinimap2D : public ADungeonMinimapBase {
    GENERATED_BODY()

public:
    ADungeonMinimap2D();

    UPROPERTY(EditAnywhere, Category = "MiniMap")
    int32 TextureSize = 512;

    UPROPERTY(EditAnywhere, Category = "MiniMap")
    float OutlineThickness = 4.0f;

    UPROPERTY(EditAnywhere, Category = "MiniMap")
    float DoorThickness = 8.0f;

    UPROPERTY(EditAnywhere, Category = "MiniMap")
    TArray<FDungeonMiniMapOverlayIcon2D> OverlayIcons;

    UPROPERTY(EditAnywhere, Category = "MiniMap")
    UMaterialInterface* MaterialTemplate;

    UPROPERTY(EditAnywhere, Category = "MiniMap Blur Layer")
    float BlurRadius = 5;

    UPROPERTY(EditAnywhere, Category = "MiniMap Blur Layer")
    int32 BlurIterations = 3;

    
    UPROPERTY(Transient)
    UTexture* MaskTexture;

    UPROPERTY(Transient)
    UTextureRenderTarget2D* StaticOverlayTexture;

    UPROPERTY(Transient)
    UTextureRenderTarget2D* DynamicOverlayTexture;

    UPROPERTY(Transient, BlueprintReadOnly, Category="MiniMap")
    UMaterialInterface* MaterialInstance;
    
public:
    UFUNCTION(BlueprintCallable, Category = Dungeon)
    UMaterialInterface* CreateMaterialInstance();

    UFUNCTION(BlueprintCallable, Category = Dungeon)
    UMaterialInterface* CreateMaterialInstanceFromTemplate(UMaterialInterface* InMaterialTemplate);

    UFUNCTION(BlueprintCallable, Category = Dungeon)
    void UpdateMaterial(UMaterialInterface* InMaterial);

    UFUNCTION(BlueprintCallable, Category = Dungeon)
    virtual void BuildLayout(ADungeon* Dungeon);

    //// Begin AActor Interface ////
    virtual void BeginDestroy() override;
    virtual void Tick(float DeltaSeconds) override;
    //// End AActor Interface ////

protected:
    float GetAttributeScaleMultiplier() const;
    float GetIconPixelSize(const FDungeonMiniMapOverlayIcon2D& OverlayData, const FVector2D& CanvasSize) const;

};

