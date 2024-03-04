//$ Copyright 2015-23, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Builders/SnapMap/Utils/SnapMapModuleDBUtils.h"

#include "Frameworks/Snap/SnapMap/SnapMapLibrary.h"
#include "Frameworks/Snap/SnapMap/SnapMapModuleBounds.h"
#include "Frameworks/Snap/SnapMap/SnapMapModuleDatabase.h"
#include "Frameworks/Snap/SnapModuleDBBuilder.h"

#include "Kismet/GameplayStatics.h"

void FSnapMapModuleDBUtils::BuildModuleDatabaseCache(USnapMapModuleDatabase* InDatabase) {
    if (!InDatabase) return;

    struct FSnapMapModulePolicy {
        FBox CalculateBounds(ULevel* Level) const {
            return SnapModuleDatabaseBuilder::FDefaultModulePolicy::CalculateBounds(Level);
        }
        void Initialize(FSnapMapModuleDatabaseItem& ModuleItem, const ULevel* Level, const UObject* InModuleDB) {}
        void PostProcess(FSnapMapModuleDatabaseItem& ModuleItem, const ULevel* Level) const {
            ModuleItem.ModuleBoundShapes = {};
            for (AActor* Actor : Level->Actors) {
                if (const ASnapMapModuleBounds* BoundsActor = Cast<ASnapMapModuleBounds>(Actor)) {
                    BoundsActor->GatherShapes(ModuleItem.ModuleBoundShapes);
                }
            }

            // If this module didn't have any custom module bounds actor, use the level bounds to create a fallback hull
            if (ModuleItem.ModuleBoundShapes.GetTotalCustomShapes() == 0) {
                FDABoundsShapeConvexPoly& FallbackHull = ModuleItem.ModuleBoundShapes.ConvexPolys.AddDefaulted_GetRef();
                FallbackHull = FSnapMapModuleDatabaseUtils::CreateFallbackModuleConvexHull(ModuleItem.ModuleBounds);
            }
        }
    };

    typedef TSnapModuleDatabaseBuilder<
        FSnapMapModuleDatabaseItem,
        FSnapMapModuleDatabaseConnectionInfo,
        FSnapMapModulePolicy,
        SnapModuleDatabaseBuilder::TDefaultConnectionPolicy<FSnapMapModuleDatabaseConnectionInfo>
    > FSnapMapDatabaseBuilder;
    
    FSnapMapDatabaseBuilder::Build(InDatabase->Modules, InDatabase);
    
    InDatabase->Modify();
}

