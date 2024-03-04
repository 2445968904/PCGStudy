//$ Copyright 2015-23, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Utils/DungeonBoundingShapes.h"

#include "Components/PrimitiveComponent.h"
#include "SnapMapModuleBounds.generated.h"

USTRUCT()
struct FSnapMapModuleBoundConvexPoly {
	GENERATED_BODY()

	UPROPERTY()
	TArray<FVector2D> Points;
};

UCLASS()
class USnapMapModuleBoundsRenderComponent : public UPrimitiveComponent {
	GENERATED_BODY()
public:
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual bool IsEditorOnly() const override;
	
	//~ Begin USceneComponent Interface
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	//~ End USceneComponent Interface
};

UCLASS(hidecategories=(Input))
class DUNGEONARCHITECTRUNTIME_API ASnapMapModuleBounds : public AActor {
	GENERATED_BODY()
public:
	ASnapMapModuleBounds();
	virtual void PostLoad() override;
	
	void RegenerateBoundsData();
	const TArray<FSnapMapModuleBoundConvexPoly>& GetConvexPolys() const { return ConvexPolys; }
	void GatherShapes(FDABoundsShapeList& OutShapes) const;
	void SetDrawColor(const FLinearColor& InDrawColor);


#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditMove(bool bFinished) override;
#endif

	bool IsPolyTriangulationValid() const { return bPolyTriangulationValid; }

private:
	void GenerateConvexPolygons();

public:
	
#if WITH_EDITORONLY_DATA
	UPROPERTY()
	USnapMapModuleBoundsRenderComponent* BoundsRenderComponent;
#endif

	UPROPERTY(EditAnywhere, Category="Dungeon Architect")
	bool bDrawBounds{true};
	
	UPROPERTY(EditAnywhere, Category="Dungeon Architect")
	EDABoundsShapeType BoundsType = EDABoundsShapeType::Polygon;
	
	UPROPERTY(EditAnywhere, Category="Dungeon Architect", meta=(MakeEditWidget=true))
	TArray<FVector> PolygonPoints = {
		{ -1500, 1000, 0 },
		{ -1500, -770, 0 },
		{ 710, -940, 0 },
		{ 2140, -3100, 0 },
		{ 3340, -1880, 0 },
		{ 1510, 970, 0 }
	};

	UPROPERTY(EditAnywhere, Category="Dungeon Architect", meta=(MakeEditWidget=true))
	FVector CircleRadius = FVector(2000, 0, 0);

	UPROPERTY(EditAnywhere, Category="Dungeon Architect", meta=(MakeEditWidget=true))
	FVector BoxExtent = FVector(1200, 800, 0);

	UPROPERTY(EditAnywhere, Category="Dungeon Architect", meta=(MakeEditWidget=true))
	FVector Height = FVector(0, 0, 1500);

	UPROPERTY(EditAnywhere, Category="Dungeon Architect", meta=(MakeEditWidget=true))
	FLinearColor DrawColor = FLinearColor(1, 0, 0, 1);

private:
	UPROPERTY()
	TArray<FSnapMapModuleBoundConvexPoly> ConvexPolys;

	UPROPERTY()
	bool bPolyTriangulationValid{};
	
#if WITH_EDITORONLY_DATA
	bool bPostEditMoveGuard{};
#endif // WITH_EDITORONLY_DATA

};


UCLASS(hidecategories=(Input))
class DUNGEONARCHITECTRUNTIME_API ASnapMapModuleBoundsCollisionDebug : public AActor {
	GENERATED_BODY()
public:
	ASnapMapModuleBoundsCollisionDebug();
	
	UPROPERTY(EditAnywhere, Category=Snap)
	TArray<ASnapMapModuleBounds*> TrackedActors;
	
	UPROPERTY(EditAnywhere, Category=Snap)
	float Tolerance = 0;

	virtual void Tick(float DeltaSeconds) override;
	virtual bool ShouldTickIfViewportsOnly() const override { return true; }
    virtual bool IsLevelBoundsRelevant() const override { return false; }
};

