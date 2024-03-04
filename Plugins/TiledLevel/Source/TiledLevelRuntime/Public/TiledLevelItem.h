// Copyright 2022 PufStudio. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "TiledLevelTypes.h"
#include "Engine/EngineTypes.h"
#include "PhysicsEngine/BodyInstance.h"
#include "Components/PrimitiveComponent.h"
#include "UObject/Object.h"
#include "TiledLevelItem.generated.h"

DECLARE_DELEGATE(FRequestForRefreshPalette)
DECLARE_DELEGATE(FUpdatePreviewMesh)
DECLARE_DELEGATE(FUpdatePreviewActor)
DECLARE_DELEGATE(FUpdateHISM)

class UTiledLevelAsset;

UCLASS(BlueprintType)
class TILEDLEVELRUNTIME_API UTiledLevelItem : public UObject
{
	GENERATED_UCLASS_BODY()

	// Use this ID to query this item in gametime	
	UPROPERTY(VisibleAnywhere, Category="TiledItem")
	FGuid ItemID = FGuid::NewGuid();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="TiledItem")
	const FName GetID();

	/*
	 * Different placed type items will never interfere with each other.
	 * A way to make you place items in the most reasonable manner.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TiledItem")
	EPlacedType PlacedType = EPlacedType::Block;

	/*
	 * Determine the behavior of replace or overlap during item placement
	 * Struct item can not overlap, thus replace previous ones
	 * Prop item can overlap everywhere.
	 * PS: no overlap between same items, it will replace previous one.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TiledItem")
	ETLStructureType StructureType = ETLStructureType::Structure;

	// how much does this item occupy the tile space?
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TiledItem")
	FVector Extent{1, 1, 1};

	// Both Actor or StaticMesh types are supported!
	UPROPERTY(BlueprintReadOnly, Category="TiledItem")
	ETLSourceType SourceType = ETLSourceType::Mesh;

	// Source mesh for Mesh type item
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mesh", meta = (NoResetToDefault))
	TObjectPtr<class UStaticMesh> TiledMesh; // make reset not possible

	// Material override for tiled item
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mesh")
	TArray<TObjectPtr<class UMaterialInterface>> OverrideMaterials; 

	// Source actor for Actor type item
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Actor", meta = (NoResetToDefault))
	TObjectPtr<UObject> TiledActor;

	// PLACEMENT SETTINGS
	
	// Automatically setup mesh origin and scale to meet tile item boundary as possible
	UPROPERTY(EditAnywhere, Category="Placement")
	bool bAutoPlacement = false;

	// Set mesh origin to different position
	UPROPERTY(EditAnywhere, Category="Placement")
	EPivotPosition PivotPosition = EPivotPosition::Bottom;

	// should override height? (floor only)
	UPROPERTY(EditAnywhere, DisplayName="Should Override Height?", Category="Placement")
	bool bHeightOverride = false;

	// New overriden height 
	UPROPERTY(EditAnywhere, meta = (EditCondition = "bHeightOverride", UIMin = 10, ClampMin = 10, ClampMax=500), Category="Placement") // min max value limited by tile size???
	float NewHeight = 10;

	// should override thickness (wall only)
	UPROPERTY(EditAnywhere, DisplayName="Should Override Thinkness?", Category="Placement")
	bool bThicknessOverride = false;

	// New overriden thickness
	UPROPERTY(EditAnywhere, meta = (EditCondition = "bThicknessOverride", UIMin = 10, ClampMin = 10, ClampMax=500), Category="Placement")
	float NewThickness = 10;

	// Can ever snap to floor? (you can quickly toggle on/off during paint)
	UPROPERTY(EditAnywhere, Category="Placement")
	bool bSnapToFloor = false;
	
	// Can ever snap to wall if exists? (Only for wall props. You can quickly toggle on/off during paint)
	UPROPERTY(EditAnywhere, Category="Placement")
	bool bSnapToWall = false;

	// final transformation adjustment
	UPROPERTY(EditAnywhere, Category="Placement")
	FTransform TransformAdjustment;

	// INSTANCE SETTINGS (copy from FoliageType.h)

	// TODO: expose this property later if proper way to fix reset component location to 0 when it is set as static....
	/** Mobility property to apply to tiled item components */
	// UPROPERTY(Category = InstanceSettings, EditAnywhere, BlueprintReadOnly)
	// TEnumAsByte<EComponentMobility::Type> Mobility;
	
	/**
	 * The distance where instances will begin to fade out if using a PerInstanceFadeAmount material node. 0 disables.
	 * When the entire cluster is beyond this distance, the cluster is completely culled and not rendered at all.
	 */
	UPROPERTY(EditAnywhere, Category=InstanceSettings, meta=(UIMin=0))
	FInt32Interval CullDistance = {0, 0};

	/** Controls whether the item should cast a shadow or not. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=InstanceSettings)
	bool CastShadow = true;

	/** Controls whether this item should inject light into the Light Propagation Volume.  This flag is only used if CastShadow is true. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=InstanceSettings, meta=(EditCondition="CastShadow"))
	bool bAffectDynamicIndirectLighting;

	/** Controls whether the primitive should affect dynamic distance field lighting methods.  This flag is only used if CastShadow is true. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=InstanceSettings, meta=(EditCondition="CastShadow"))
	bool bAffectDistanceFieldLighting;

	/** Controls whether the tiled item should cast shadows in the case of non precomputed shadowing.  This flag is only used if CastShadow is true. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=InstanceSettings, meta=(EditCondition="CastShadow"))
	bool bCastDynamicShadow = true;

	/** Whether the tiled item should cast a static shadow from shadow casting lights.  This flag is only used if CastShadow is true. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=InstanceSettings, meta=(EditCondition="CastShadow"))
	bool bCastStaticShadow = true;

	/** Whether the object should cast contact shadows. This flag is only used if CastShadow is true. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=InstanceSettings, meta=(EditCondition = "CastShadow"))
	bool bCastContactShadow = true;

	/** Whether this tiled item should cast dynamic shadows as if it were a two sided material. */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadOnly, Category=InstanceSettings, meta=(EditCondition="bCastDynamicShadow"))
	bool bCastShadowAsTwoSided;

	/** Whether the tiled item receives decals. */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadOnly, Category=InstanceSettings)
	bool bReceivesDecals;

	/** Whether to override the lightmap resolution defined in the static mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=InstanceSettings, meta=(InlineEditConditionToggle))
	bool bOverrideLightMapRes;

	/** Overrides the lightmap resolution defined in the static mesh */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=InstanceSettings, meta=(DisplayName="Light Map Resolution", EditCondition="bOverrideLightMapRes"))
	int32 OverriddenLightMapRes;

	/** Controls the type of lightmap used for this component. */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadOnly, Category=InstanceSettings)
	ELightmapType LightmapType;

	/**
	 * If enabled, tiled item will render a pre-pass which allows it to occlude other primitives, and also allows 
	 * it to correctly receive DBuffer decals. Enabling this setting may have a negative performance impact.
	 */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadOnly, Category = InstanceSettings)
	bool bUseAsOccluder;

	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadOnly, Category = InstanceSettings)
	bool bVisibleInRayTracing = true;

	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadOnly, Category = InstanceSettings)
	bool bEvaluateWorldPositionOffset = true;

	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadOnly, Category = InstanceSettings, meta = (ClampMin=0))
	int32 WorldPositionOffsetDisableDistance;

	/** Custom collision for tiled item */
	UPROPERTY(EditAnywhere, Category=InstanceSettings, meta=(HideObjectType=true))
	FBodyInstance BodyInstance;

	/** Force navmesh */
	UPROPERTY(EditAnywhere, Category=InstanceSettings, meta=(HideObjectType=true))
	TEnumAsByte<EHasCustomNavigableGeometry::Type> CustomNavigableGeometry = EHasCustomNavigableGeometry::Yes;

	/**
	 * Lighting channels that placed tiled item will be assigned. Lights with matching channels will affect the item.
	 * These channels only apply to opaque materials, direct lighting, and dynamic lighting and shadowing.
	 */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadOnly, Category=InstanceSettings)
	FLightingChannels LightingChannels;

	/** If true, the tiled item will be rendered in the CustomDepth pass (usually used for outlines) */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadOnly, Category=InstanceSettings, meta=(DisplayName = "Render CustomDepth Pass"))
	bool bRenderCustomDepth;

	/** Mask used for stencil buffer writes. */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadOnly, Category=InstanceSettings, meta = (editcondition = "bRenderCustomDepth"))
	ERendererStencilMask CustomDepthStencilWriteMask;

	/** Optionally write this 0-255 value to the stencil buffer in CustomDepth pass (Requires project setting or r.CustomDepth == 3) */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadOnly, Category=InstanceSettings,  meta=(UIMin = "0", UIMax = "255", editcondition = "bRenderCustomDepth", DisplayName = "CustomDepth Stencil Value"))
	int32 CustomDepthStencilValue;

	/**
	 * Translucent objects with a lower sort priority draw behind objects with a higher priority.
	 * Translucent objects with the same priority are rendered from back-to-front based on their bounds origin.
	 * This setting is also used to sort objects being drawn into a runtime virtual texture.
	 *
	 * Ignored if the object is not translucent.  The default priority is zero.
	 * Warning: This should never be set to a non-default value unless you know what you are doing, as it will prevent the renderer from sorting correctly.
	 * It is especially problematic on dynamic gameplay effects.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=InstanceSettings)
	int32 TranslucencySortPriority;

	// Enable for detail meshes that don't really affect the game. Disable for anything important.
	// Typically, this will be enabled for small meshes without collision (e.g. grass) and disabled for large meshes with collision (e.g. trees)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=InstanceSettings)
	bool bEnableDensityScaling;
	
	// VIRTUAL TEXTURE

	/** 
	 * Array of runtime virtual textures into which we draw the instances. 
	 * The mesh material also needs to be set up to output to a virtual texture. 
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=VirtualTexture, meta = (DisplayName = "Draw in Virtual Textures"))
	TArray<TObjectPtr<URuntimeVirtualTexture>> RuntimeVirtualTextures;

	/**
	 * Number of lower mips in the runtime virtual texture to skip for rendering this primitive.
	 * Larger values reduce the effective draw distance in the runtime virtual texture.
	 * This culling method doesn't take into account primitive size or virtual texture size.
	 */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadOnly, Category = VirtualTexture, meta = (DisplayName = "Virtual Texture Skip Mips", UIMin = "0", UIMax = "7"))
	int32 VirtualTextureCullMips = 0;

	/** Controls if this component draws in the main pass as well as in the virtual texture. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = VirtualTexture, meta = (DisplayName = "Draw in Main Pass"))
	ERuntimeVirtualTextureMainPassType VirtualTextureRenderPassType = ERuntimeVirtualTextureMainPassType::Exclusive;

	// HLOD

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=HLOD)
	uint32 bIncludeInHLOD:1;
#endif // WITH_EDITORONLY_DATA
	
	// CONTROL parameters
	
	UPROPERTY()
	bool bIsEraserAllowed = true;

	// fill tool vars
	UPROPERTY()
	float FillCoefficient = 1.0f;

	UPROPERTY()
	bool bAllowRandomRotation = false;
	
	UPROPERTY()
	bool bAllowOverlay = false;

	virtual FString GetItemName() const;
	virtual FString GetItemInfo() const;
	virtual FString GetItemNameWithInfo() const;

	bool IsShape2D() const
	{
		return PlacedType == EPlacedType::Edge || PlacedType == EPlacedType::Wall;
	}

	virtual bool IsSpecialItem()
	{
		return false;
	}
	
	bool operator==(const UTiledLevelItem& Other) const { return ItemID == Other.ItemID; }

	FRequestForRefreshPalette RequestForRefreshPalette;
	FUpdatePreviewMesh UpdatePreviewMesh;
	FUpdatePreviewActor UpdatePreviewActor;
	FUpdateHISM UpdateHISM;

	// no need now... the dependency is no more needed...
	// UPROPERTY()
	// TArray<TObjectPtr<class UTiledLevelAsset>> AssetsUsingThisItem;

	// hide some properties when not in set editor...
	bool IsEditInSet = false;
	
	
#if WITH_EDITOR
	virtual void PreEditChange(FProperty* PropertyAboutToChange) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

// private:
// 	UPROPERTY()
// 	TObjectPtr<UObject> PreviousTiledActorObject;
	
};

DECLARE_DELEGATE(FUpdatePreviewRestriction)

UCLASS()
class TILEDLEVELRUNTIME_API UTiledLevelRestrictionItem : public UTiledLevelItem
{
	GENERATED_UCLASS_BODY()
    
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="RestrictionItem|Visual")
	FLinearColor BorderColor = {0.8f, 0.2f, 0.2f, 0.2f};
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="RestrictionItem|Visual", meta=(ClampMax="0.5", ClampMin="0.0", UIMax="0.5", UIMin="0.0"))
	float BorderWidth = 0.5f;

	// Set false when you want to see the visuals in game time
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="RestrictionItem|Debug")
	bool bHiddenInGame = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="RestrictionItem|Rule")
	ERestrictionType RestrictionType = ERestrictionType::DisallowBuilding;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="RestrictionItem|Rule")
	bool bTargetAllItems = true;

	UPROPERTY(BlueprintReadOnly, Category="RestrictionItem|Rule")
	TArray<FGuid> TargetItems;

	FString GetItemName() const override;
	FString GetItemNameWithInfo() const override;
	
	FUpdatePreviewRestriction UpdatePreviewRestriction;

	virtual bool IsSpecialItem() override
	{
		return true;
	}
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	
};


USTRUCT(BlueprintType)
struct FTemplateAsset
{
    GENERATED_USTRUCT_BODY()
    
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Asset")
	TObjectPtr<UObject> TemplateAsset;
};

// The template item that can reuse existing tiled levels...
UCLASS(BlueprintType)
class TILEDLEVELRUNTIME_API UTiledLevelTemplateItem : public UTiledLevelItem
{
	GENERATED_UCLASS_BODY()

    // must have the same tile size
    UPROPERTY(EditAnywhere, Category="TemplateItem")
    FTemplateAsset TemplateAsset;

    UTiledLevelAsset* GetAsset() const;

    FString GetItemName() const override;
    FString GetItemNameWithInfo() const override;
	
	virtual bool IsSpecialItem() override
	{
		return true;
	}

};


USTRUCT(BlueprintType)
struct FTiledItemObj
{
	GENERATED_BODY()

	FTiledItemObj() = default;
		
	FTiledItemObj(FGuid InitID)
		: TiledItemID(InitID.ToString())
	{}

	UPROPERTY(EditAnywhere, DisplayName="Tiled Item", Category="Item")
	FString TiledItemID;

	bool operator==(const FTiledItemObj& Other) const
	{
		return TiledItemID == Other.TiledItemID;
	}

	friend uint32 GetTypeHash(const FTiledItemObj& Other)
	{
		return GetTypeHash(Other.TiledItemID);
	}

};
