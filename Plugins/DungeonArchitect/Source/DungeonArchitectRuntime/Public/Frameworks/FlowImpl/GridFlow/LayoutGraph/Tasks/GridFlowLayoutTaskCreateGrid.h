//$ Copyright 2015-23, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Flow/Domains/LayoutGraph/Tasks/BaseFlowLayoutTask.h"
#include "GridFlowLayoutTaskCreateGrid.generated.h"

class UFlowAbstractGraphBase;

UCLASS(Meta = (AbstractTask, Title="Create Grid", Tooltip="Create a grid to work with", MenuPriority = 1000))
class DUNGEONARCHITECTRUNTIME_API UGridFlowLayoutTaskCreateGrid : public UBaseFlowLayoutTask {
    GENERATED_BODY()
public:
    /** 
    The size of the initial layout grid 

    Variable Name: GridSize
    */
    UPROPERTY(EditAnywhere, Category = "Create Grid")
    FIntPoint GridSize = FIntPoint(6, 5);

public:
    UGridFlowLayoutTaskCreateGrid();
    virtual void Execute(const FFlowExecutionInput& Input, const FFlowTaskExecutionSettings& InExecSettings, FFlowExecutionOutput& Output) override;
    virtual bool GetParameter(const FString& InParameterName, FDAAttribute& OutValue) override;
    virtual bool SetParameter(const FString& InParameterName, const FDAAttribute& InValue) override;
    virtual bool SetParameterSerialized(const FString& InParameterName, const FString& InSerializedText) override;
    
private:
    void CreateGraph(UFlowAbstractGraphBase* InGraph, const FIntVector& InGridSize) const;
};

