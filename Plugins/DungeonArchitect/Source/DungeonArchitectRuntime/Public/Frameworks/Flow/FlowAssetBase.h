//$ Copyright 2015-23, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "FlowAssetBase.generated.h"

class UEdGraph;
class UFlowExecScript;

UCLASS()
class DUNGEONARCHITECTRUNTIME_API UFlowAssetBase : public UObject {
    GENERATED_BODY()

public:
    UPROPERTY()
    UFlowExecScript* ExecScript;

#if WITH_EDITORONLY_DATA
    UPROPERTY()
    UEdGraph* ExecEdGraph;
#endif // WITH_EDITORONLY_DATA

    static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);
};

