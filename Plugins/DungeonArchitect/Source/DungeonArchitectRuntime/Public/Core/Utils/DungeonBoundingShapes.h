//$ Copyright 2015-23, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "DungeonBoundingShapes.generated.h"

UENUM()
enum class EDABoundsShapeType : uint8 {
	Polygon,
	Box,
	Circle
};

USTRUCT(BlueprintType)
struct DUNGEONARCHITECTRUNTIME_API FDABoundsShapeCircle {
	GENERATED_BODY()

	UPROPERTY()
	float Height{};

	UPROPERTY()
	FTransform Transform;

	UPROPERTY()
	float Radius{};
};

USTRUCT(BlueprintType)
struct DUNGEONARCHITECTRUNTIME_API FDABoundsShapeLine {
	GENERATED_BODY()

	UPROPERTY()
	float Height = 0;

	UPROPERTY()
	FTransform Transform;

	UPROPERTY()
	FVector2D LineStart = FVector2D::ZeroVector;
	
	UPROPERTY()
	FVector2D LineEnd = FVector2D::ZeroVector;
};


USTRUCT(BlueprintType)
struct DUNGEONARCHITECTRUNTIME_API FDABoundsShapeConvexPoly {
	GENERATED_BODY()

	UPROPERTY()
	float Height{};
	
	UPROPERTY()
	FTransform Transform;

	UPROPERTY()
	TArray<FVector2D> Points;
};

USTRUCT(BlueprintType)
struct DUNGEONARCHITECTRUNTIME_API FDABoundsShapeList {
	GENERATED_BODY()
	
	UPROPERTY(VisibleAnywhere, Category=DungeonArchitect)
	TArray<FDABoundsShapeConvexPoly> ConvexPolys;
	
	UPROPERTY(VisibleAnywhere, Category=DungeonArchitect)
	TArray<FDABoundsShapeCircle> Circles;

	FORCEINLINE int32 GetTotalCustomShapes() const { return ConvexPolys.Num() + Circles.Num(); }
	FDABoundsShapeList TransformBy(const FTransform& InTransform) const;
};


class DUNGEONARCHITECTRUNTIME_API FDABoundsShapeCollision {
public:
	static void ConvertBoxToConvexPoly(const FBox& InBox, FDABoundsShapeConvexPoly& OutPoly);
	static void ConvertBoxToConvexPoly(const FTransform& InTransform, const FVector& InExtent, FDABoundsShapeConvexPoly& OutPoly);
	static bool Intersects(const FDABoundsShapeConvexPoly& A, const FDABoundsShapeConvexPoly& B, float Tolerance);
	static bool Intersects(const FDABoundsShapeConvexPoly& A, const FDABoundsShapeCircle& B, float Tolerance);
	static bool Intersects(const FDABoundsShapeCircle& A, const FDABoundsShapeCircle& B, float Tolerance);
	static bool Intersects(const FDABoundsShapeList& A, const FDABoundsShapeList& B, float Tolerance);
	static bool Intersects(const FDABoundsShapeConvexPoly& Poly, const FDABoundsShapeLine& Line, float Tolerance);
	
	static bool Intersects(const FBox& A, const FDABoundsShapeConvexPoly& B, float Tolerance);
	static bool Intersects(const FBox& A, const FDABoundsShapeCircle& B, float Tolerance);
	static bool Intersects(const FBox& A, const FDABoundsShapeList& B, float Tolerance);

	static void TransformPoints(const TArray<FVector2D>& InPoints, const FTransform& InTransform, TArray<FVector2D>& OutTransformedPoints);
	
private:
	static void PopulatePolyProjectionNormals(const TArray<FVector2D>& InTransformedPoints, TSet<FVector2D>& Visited, TArray<FVector2D>& OutProjections);
	static void PopulateCircleProjectionNormals(const FVector2D& InTransformedCenter, const TArray<FVector2D>& InTransformedPolyPoints, TSet<FVector2D>& Visited, TArray<FVector2D>& OutProjections);
	
	FORCEINLINE static float GetOverlapsDistance(float MinA, float MaxA, float MinB, float MaxB) {
		return FMath::Min(MaxA, MaxB) - FMath::Max(MinA, MinB);
	}

	template<typename TA, typename TB>
	static float GetHeightOverlapDistance(const TA& A, const TB& B) {
		const float MinA = A.Transform.GetLocation().Z;
		const float MaxA = MinA + A.Height;
		const float MinB = B.Transform.GetLocation().Z;
		const float MaxB = MinB + B.Height;
		return GetOverlapsDistance(MinA, MaxA, MinB, MaxB);
	}
    
	template<typename TA, typename TB>
	static bool HeightOverlaps(const TA& A, const TB& B, float Tolerance) {
		return GetHeightOverlapDistance(A, B) - Tolerance > 0;
	}
	
	template<typename TB>
	static float GetBoxHeightOverlapDistance(const FBox& A, const TB& B) {
		const float MinA = A.Min.Z;
		const float MaxA = A.Max.Z;
		const float MinB = B.Transform.GetLocation().Z;
		const float MaxB = MinB + B.Height;
		return GetOverlapsDistance(MinA, MaxA, MinB, MaxB);
	}
	
	template<typename TB>
	static bool BoxHeightOverlaps(const FBox& A, const TB& B, float Tolerance) {
		return GetBoxHeightOverlapDistance(A, B) - Tolerance > 0;
	}
};

