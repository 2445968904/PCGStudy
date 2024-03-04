//$ Copyright 2015-23, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Snap/SnapMap/SnapMapModuleBounds.h"

#include "Components/BillboardComponent.h"
#include "GeomTools.h"

ASnapMapModuleBounds::ASnapMapModuleBounds() {
	RootComponent = CreateDefaultSubobject<USceneComponent>("SceneRoot");
	UBillboardComponent* BillboardComponent = CreateDefaultSubobject<UBillboardComponent>("Sprite");
	BillboardComponent->SetupAttachment(RootComponent);

#if WITH_EDITORONLY_DATA
	BoundsRenderComponent = CreateDefaultSubobject<USnapMapModuleBoundsRenderComponent>("Bounds");
	BoundsRenderComponent->SetupAttachment(RootComponent);
	BoundsRenderComponent->SetHiddenInGame(true);
#endif // WITH_EDITORONLY_DATA

	if (BoundsType == EDABoundsShapeType::Polygon && PolygonPoints.Num() >= 3 && ConvexPolys.Num() == 0) {
		RegenerateBoundsData();
	}
}

void ASnapMapModuleBounds::PostLoad() {
	Super::PostLoad();

}

#if WITH_EDITOR
void ASnapMapModuleBounds::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) {
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName Name = (PropertyChangedEvent.MemberProperty != nullptr)
		? PropertyChangedEvent.MemberProperty->GetFName()
		: NAME_None;

	bool bRedraw{};
	if (Name == GET_MEMBER_NAME_CHECKED(ASnapMapModuleBounds, Height)) {
		Height.X = 0;
		Height.Y = 0;
		bRedraw = true;
	}
	else if (Name == GET_MEMBER_NAME_CHECKED(ASnapMapModuleBounds, BoxExtent)) {
		BoxExtent.Z = 0;
		bRedraw = true;
	}
	else if (Name == GET_MEMBER_NAME_CHECKED(ASnapMapModuleBounds, CircleRadius)) {
		CircleRadius.Z = 0;
		bRedraw = true;
	}
	else if (Name == GET_MEMBER_NAME_CHECKED(ASnapMapModuleBounds, PolygonPoints)) {
		for (FVector& ConvexPoint : PolygonPoints) {
			ConvexPoint.Z = 0;
		}
		bRedraw = true;
	}
	else if (Name == GET_MEMBER_NAME_CHECKED(ASnapMapModuleBounds, DrawColor)
		|| Name == GET_MEMBER_NAME_CHECKED(ASnapMapModuleBounds, BoundsType))
	{
		bRedraw = true;
	}
	
	if (bRedraw) {
		RegenerateBoundsData();
	}
}

void ASnapMapModuleBounds::PostEditMove(bool bFinished) {
	Super::PostEditMove(bFinished);

	if (!bPostEditMoveGuard) {
		bool bFixTransform{};
		const FVector RotationAlongAxes = GetActorRotation().Euler();
		if (!FMath::IsNearlyZero(RotationAlongAxes.X) || !FMath::IsNearlyZero(RotationAlongAxes.Y) || !GetActorScale3D().IsUnit()) {
			bFixTransform = true;
		}
	
		if (bFixTransform) {
			bPostEditMoveGuard = true;
			SetActorTransform(FTransform(FRotator::MakeFromEuler(FVector(0, 0, RotationAlongAxes.Z)), GetActorLocation()));
			bPostEditMoveGuard = false;
		}
	}
}

#endif //WITH_EDITOR


void ASnapMapModuleBounds::RegenerateBoundsData() {
	if (BoundsType == EDABoundsShapeType::Polygon) {
		GenerateConvexPolygons();
	}
	MarkComponentsRenderStateDirty();
}

void ASnapMapModuleBounds::GatherShapes(FDABoundsShapeList& OutShapes) const {
	if (BoundsType == EDABoundsShapeType::Box) {
		// Treat the box as a convex hull
		FDABoundsShapeConvexPoly& Poly = OutShapes.ConvexPolys.AddDefaulted_GetRef();
		Poly.Height = Height.Z;
		Poly.Transform = GetActorTransform();
		const float X = BoxExtent.X;
		const float Y = BoxExtent.Y;
		Poly.Points.Add(FVector2D(-X, -Y));
		Poly.Points.Add(FVector2D(-X, Y));
		Poly.Points.Add(FVector2D(X, Y));
		Poly.Points.Add(FVector2D(X, -Y));
	}
	else if (BoundsType == EDABoundsShapeType::Circle) {
		FDABoundsShapeCircle& Circle = OutShapes.Circles.AddDefaulted_GetRef();
		Circle.Height = Height.Z;
		Circle.Transform = GetActorTransform();
		Circle.Radius = CircleRadius.Size2D();
	}
	else if (BoundsType == EDABoundsShapeType::Polygon) {
		for (const FSnapMapModuleBoundConvexPoly& ConvexPoly : GetConvexPolys()) {
			FDABoundsShapeConvexPoly& HullEntry = OutShapes.ConvexPolys.AddDefaulted_GetRef();
			HullEntry.Height = Height.Z;
			HullEntry.Transform = GetActorTransform();
			HullEntry.Points = ConvexPoly.Points;
		} 
	}
}

void ASnapMapModuleBounds::SetDrawColor(const FLinearColor& InDrawColor) {
	if (DrawColor != InDrawColor) {
		DrawColor = InDrawColor;
		MarkComponentsRenderStateDirty();
	}
}

void ASnapMapModuleBounds::GenerateConvexPolygons() {
	ConvexPolys.Reset();
	if (PolygonPoints.Num() < 3) {
		return;
	}

	TArray<FVector2D> PolygonPoints2D;
	PolygonPoints2D.Reserve(PolygonPoints.Num());
	for (const FVector& PolygonPoint3D : PolygonPoints) {
		PolygonPoints2D.Add(FVector2D(PolygonPoint3D));
	}
	
	TArray<FVector2D> TriangleSoup;
	bPolyTriangulationValid = FGeomTools2D::TriangulatePoly(TriangleSoup, PolygonPoints2D);
	if (bPolyTriangulationValid) {
		TArray<TArray<FVector2D>> GeneratedConvexPolys;
		FGeomTools2D::GenerateConvexPolygonsFromTriangles(GeneratedConvexPolys, TriangleSoup);

		ConvexPolys.Reserve(GeneratedConvexPolys.Num());
		for (const TArray<FVector2D>& GeneratedConvexPoly : GeneratedConvexPolys) {
			FSnapMapModuleBoundConvexPoly& CachedConvexPoly = ConvexPolys.AddDefaulted_GetRef();
			CachedConvexPoly.Points = GeneratedConvexPoly;
		}
	}
}

FPrimitiveSceneProxy* USnapMapModuleBoundsRenderComponent::CreateSceneProxy() {
	class FSnapMapModuleBoundsRenderProxy : public FPrimitiveSceneProxy {
	public:
		FSnapMapModuleBoundsRenderProxy(const USnapMapModuleBoundsRenderComponent* InComponent)
			: FPrimitiveSceneProxy(InComponent)
		{
			if (const ASnapMapModuleBounds* BoundsActor = Cast<ASnapMapModuleBounds>(InComponent->GetOwner())) {
				bDrawBounds = BoundsActor->bDrawBounds;
				Height = BoundsActor->Height.Z;
				BoundsType = BoundsActor->BoundsType;
				CircleRadius = BoundsActor->CircleRadius.Size2D();
				BoxExtent2D = FVector2D(BoundsActor->BoxExtent);
				DrawColor = BoundsActor->DrawColor;
				ConvexPolys = BoundsActor->GetConvexPolys();
				
				bPolyTriangulationValid = BoundsActor->IsPolyTriangulationValid();
				PolyOutline.Reserve(BoundsActor->PolygonPoints.Num());
				for (const FVector& Point3D : BoundsActor->PolygonPoints) {
					PolyOutline.Add(FVector2D(Point3D));
				}
			}
			else {
				bDrawBounds = false;
			}
		}

		virtual SIZE_T GetTypeHash() const override {
			static size_t UniquePointer;
			return reinterpret_cast<size_t>(&UniquePointer);
		}
		
		virtual uint32 GetMemoryFootprint() const override {
			return(sizeof(*this) + GetAllocatedSize());
		}

		static void DADrawDashedLine(FPrimitiveDrawInterface* PDI, const FVector& Start, const FVector& End, const FLinearColor& Color, double DashSize, uint8 DepthPriority, float LineThickness, float DepthBias, bool bScreenSpace) {
			FVector LineDir = End - Start;
			double LineLeft = (End - Start).Size();
			if (LineLeft)
			{
				LineDir /= LineLeft;
			}

			const int32 nLines = FMath::CeilToInt32(LineLeft / (DashSize*2));
			PDI->AddReserveLines(DepthPriority, nLines, DepthBias != 0);

			const FVector Dash = (DashSize * LineDir);

			FVector DrawStart = Start;
			while (LineLeft > DashSize)
			{
				const FVector DrawEnd = DrawStart + Dash;

				PDI->DrawLine(DrawStart, DrawEnd, Color, DepthPriority, LineThickness, DepthBias, bScreenSpace);

				LineLeft -= 2*DashSize;
				DrawStart = DrawEnd + Dash;
			}
			if (LineLeft > 0.0f)
			{
				const FVector DrawEnd = End;

				PDI->DrawLine(DrawStart, DrawEnd, Color, DepthPriority, LineThickness, DepthBias, bScreenSpace);
			}
		}

		virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override {
			if (!bDrawBounds) {
				return;
			}

			float LineThickness = 0; //IsSelected() ? 2 : 1;
			FLinearColor LineColor = DrawColor;
			constexpr float DashSize = 100;
			const FMatrix Matrix = GetLocalToWorld();
			const FVector AxisX = Matrix.GetUnitAxis(EAxis::X);
			const FVector AxisY = Matrix.GetUnitAxis(EAxis::Y);
			const FVector AxisZ = Matrix.GetUnitAxis(EAxis::Z);
			constexpr float DepthBias = 0;
			constexpr bool bDrawScreenSpace = false;
			constexpr uint8 DepthPriorityGroup = SDPG_Foreground;

			if (!IsSelected()) {
				LineThickness = 5;
				LineColor.A = 0.5f;
			}
			
			for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++) {
				if (VisibilityMap & (1 << ViewIndex)) {
					FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);
					if (BoundsType == EDABoundsShapeType::Box) {
						const float HalfHeight = Height * 0.5f;
						const FVector BoxCenter = GetLocalToWorld().TransformPosition(FVector(0, 0, HalfHeight));
						const FVector BoxExtent = FVector(BoxExtent2D, HalfHeight);
						
						DrawOrientedWireBox(PDI, BoxCenter, AxisX, AxisY, AxisZ,
							BoxExtent, LineColor, DepthPriorityGroup, LineThickness, DepthBias, bDrawScreenSpace);
					}
					else if (BoundsType == EDABoundsShapeType::Circle) {
						constexpr int32 NumSides = 32;
						FVector BasePoint = GetLocalToWorld().TransformPosition(FVector::Zero());
						FVector TopPoint = GetLocalToWorld().TransformPosition(FVector(0, 0, Height));
						
						DrawCircle(PDI, BasePoint, AxisX, AxisY, LineColor, CircleRadius, NumSides, DepthPriorityGroup, LineThickness, DepthBias, bDrawScreenSpace);
						DrawCircle(PDI, TopPoint, AxisX, AxisY, LineColor, CircleRadius, NumSides, DepthPriorityGroup, LineThickness, DepthBias, bDrawScreenSpace);

						constexpr float AngleDelta = PI * 2.0f / NumSides; 
						for (int i = 0; i < NumSides; i++) {
							const float AngleRad = i * AngleDelta;
							FVector Lower(FMath::Cos(AngleRad) * CircleRadius, FMath::Sin(AngleRad) * CircleRadius, 0);
							FVector Upper(Lower.X, Lower.Y, Height);
							Lower = GetLocalToWorld().TransformPosition(Lower);
							Upper = GetLocalToWorld().TransformPosition(Upper);
							DADrawDashedLine(PDI, Lower, Upper, LineColor, DashSize, DepthPriorityGroup, LineThickness, DepthBias, bDrawScreenSpace);
						}
					}
					else if (BoundsType == EDABoundsShapeType::Polygon) {
						if (bPolyTriangulationValid) {
							/*
							const FLinearColor InnerLineColor = LineColor * 0.5f;
							for (const FSnapMapModuleBoundConvexPoly& ConvexPoly : ConvexPolys) {
								TArray<FVector2D> Points = ConvexPoly.Points;
								if (Points.Num() >= 3) {
									for (int i = 0; i < Points.Num(); i++) {
										FVector2D PolyPoint0 = Points[i];
										FVector2D PolyPoint1 = Points[(i + 1) % Points.Num()];
										FVector Lo0 = GetLocalToWorld().TransformPosition(FVector(PolyPoint0, 0));
										FVector Lo1 = GetLocalToWorld().TransformPosition(FVector(PolyPoint1, 0));
										PDI->DrawLine(Lo0, Lo1, InnerLineColor, DepthPriorityGroup, LineThickness, DepthBias, bDrawScreenSpace);
									}
								}
							}
							*/
							for (int i = 0; i < PolyOutline.Num(); i++) {
								FVector2D PolyPoint0 = PolyOutline[i];
								FVector2D PolyPoint1 = PolyOutline[(i + 1) % PolyOutline.Num()];

								FVector Lo0 = GetLocalToWorld().TransformPosition(FVector(PolyPoint0, 0));
								FVector Hi0 = GetLocalToWorld().TransformPosition(FVector(PolyPoint0, Height));
								FVector Lo1 = GetLocalToWorld().TransformPosition(FVector(PolyPoint1, 0));
								FVector Hi1 = GetLocalToWorld().TransformPosition(FVector(PolyPoint1, Height));

								PDI->DrawLine(Lo0, Lo1, LineColor, DepthPriorityGroup, LineThickness, DepthBias, bDrawScreenSpace);
								PDI->DrawLine(Hi0, Hi1, LineColor, DepthPriorityGroup, LineThickness, DepthBias, bDrawScreenSpace);
								DADrawDashedLine(PDI, Lo0, Hi0, LineColor, DashSize, DepthPriorityGroup, LineThickness, DepthBias, bDrawScreenSpace);
							} 
						}
						else if (PolyOutline.Num() >= 3) {
							// The triangulation is invalid.  Draw the outline indicating it
							for (int i = 0; i < PolyOutline.Num(); i++) {
								FVector2D PolyPoint0 = PolyOutline[i];
								FVector2D PolyPoint1 = PolyOutline[(i + 1) % PolyOutline.Num()];
								FVector Lo0 = GetLocalToWorld().TransformPosition(FVector(PolyPoint0, 0));
								FVector Lo1 = GetLocalToWorld().TransformPosition(FVector(PolyPoint1, 0));
								DADrawDashedLine(PDI, Lo0, Lo1, FLinearColor(1, 0.25f, 0, 1), DashSize, DepthPriorityGroup, 0, DepthBias, true);
							} 
						}
					}
				}
			}			
		}		
		
		virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override
		{
			FPrimitiveViewRelevance Result;
			Result.bDrawRelevance = IsShown(View);
			Result.bDynamicRelevance = true;
			Result.bShadowRelevance = IsShadowCast(View);
			Result.bEditorPrimitiveRelevance = UseEditorCompositing(View);
			return Result;
		}

	private:
		bool bDrawBounds{};
		EDABoundsShapeType BoundsType{};
		TArray<FSnapMapModuleBoundConvexPoly> ConvexPolys;
		TArray<FVector2D> PolyOutline;
		bool bPolyTriangulationValid;
		FVector2D BoxExtent2D;
		float CircleRadius;
		float Height{};
		FLinearColor DrawColor;
	};

	return new FSnapMapModuleBoundsRenderProxy(this);
}

bool USnapMapModuleBoundsRenderComponent::IsEditorOnly() const {
	return true;
}

FBoxSphereBounds USnapMapModuleBoundsRenderComponent::CalcBounds(const FTransform& LocalToWorld) const {
	if (const ASnapMapModuleBounds* BoundsActor = Cast<ASnapMapModuleBounds>(GetOwner())) {
		FBox LocalBounds(ForceInit);
		const float HalfHeight = BoundsActor->Height.Z * 0.5;
		if (BoundsActor->BoundsType == EDABoundsShapeType::Box) {
			FVector Extent = BoundsActor->BoxExtent;
			Extent.Z = HalfHeight;
			const FVector Center(0, 0, HalfHeight);
			LocalBounds = FBox(Center - Extent, Center + Extent);
		}
		else if (BoundsActor->BoundsType == EDABoundsShapeType::Circle) {
			const float ExtentDistance = BoundsActor->CircleRadius.Size2D() * 1.4142135f;
			const FVector Extent(ExtentDistance, ExtentDistance, HalfHeight);
			const FVector Center(0, 0, HalfHeight);
			LocalBounds = FBox(Center - Extent, Center + Extent);
		}
		else if (BoundsActor->BoundsType == EDABoundsShapeType::Polygon) {
			for (const FVector& Point : BoundsActor->PolygonPoints) {
				LocalBounds += Point;
			}
			LocalBounds += FVector(0, 0, BoundsActor->Height.Z);
		}
		else {
			return Super::CalcBounds(LocalToWorld);	
		}

		const FBox WorldBox = LocalBounds.TransformBy(GetComponentTransform());
		return FBoxSphereBounds(WorldBox);
	}
	else {
		return Super::CalcBounds(LocalToWorld);
	}
}


//////////////////////// ASnapMapModuleBoundsCollisionDebug ////////////////////////
ASnapMapModuleBoundsCollisionDebug::ASnapMapModuleBoundsCollisionDebug() {
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void ASnapMapModuleBoundsCollisionDebug::Tick(float DeltaSeconds) {
	Super::Tick(DeltaSeconds);

	static FLinearColor LineColorNormal(1, 0, 0, 1);
	static FLinearColor LineColorIntersecting(0, 0, 1, 1); 

	TSet<ASnapMapModuleBounds*> IntersectingSet;
	for (int i = 0; i < TrackedActors.Num(); i++) {
		for (int j = i + 1; j < TrackedActors.Num(); j++) {
			ASnapMapModuleBounds* A = TrackedActors[i];
			ASnapMapModuleBounds* B = TrackedActors[j];
			if (!A || !B) continue;

			FDABoundsShapeList ShapesA;
			A->GatherShapes(ShapesA);

			FDABoundsShapeList ShapesB;
			B->GatherShapes(ShapesB);

			if (FDABoundsShapeCollision::Intersects(ShapesA, ShapesB, Tolerance)) {
				IntersectingSet.Add(A);
				IntersectingSet.Add(B);
			}
		}
	}

	for (ASnapMapModuleBounds* Actor : TrackedActors) {
		const bool bIntersects = IntersectingSet.Contains(Actor);
		const FLinearColor LineColor = bIntersects ? LineColorIntersecting : LineColorNormal;
		Actor->SetDrawColor(LineColor);
	}
}


