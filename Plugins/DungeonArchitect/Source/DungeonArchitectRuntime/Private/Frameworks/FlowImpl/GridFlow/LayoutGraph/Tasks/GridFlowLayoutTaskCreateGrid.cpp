//$ Copyright 2015-23, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/FlowImpl/GridFlow/LayoutGraph/Tasks/GridFlowLayoutTaskCreateGrid.h"

#include "Frameworks/Flow/Domains/LayoutGraph/Core/FlowAbstractGraph.h"
#include "Frameworks/Flow/ExecGraph/FlowExecTaskAttributeMacros.h"
#include "Frameworks/FlowImpl/GridFlow/LayoutGraph/GridFlowAbstractGraph.h"

UGridFlowLayoutTaskCreateGrid::UGridFlowLayoutTaskCreateGrid() {
    InputConstraint = EFlowExecTaskInputConstraint::NoInput;
}

////////////////////////////////////// UGridFlowTaskAbstract_CreateGrid //////////////////////////////////////
void UGridFlowLayoutTaskCreateGrid::Execute(const FFlowExecutionInput& Input, const FFlowTaskExecutionSettings& InExecSettings, FFlowExecutionOutput& Output) {
    check(Input.IncomingNodeOutputs.Num() == 0);
    // Build the graph object
    UGridFlowAbstractGraph* Graph = NewObject<UGridFlowAbstractGraph>();
    Graph->GridSize = GridSize;
    CreateGraph(Graph, FIntVector(GridSize.X, GridSize.Y, 1));

    // Create a new state, since this will our first node
    Output.State = MakeShareable(new FFlowExecNodeState);
    Output.State->SetStateObject(UFlowAbstractGraphBase::StateTypeID, Graph);
    
    Output.ExecutionResult = EFlowTaskExecutionResult::Success;
}

void UGridFlowLayoutTaskCreateGrid::CreateGraph(UFlowAbstractGraphBase* InGraph, const FIntVector& InGridSize) const {
    TMap<FIntVector, FGuid> Nodes;
    for (int z = 0; z < InGridSize.Z; z++) {
        for (int y = 0; y < InGridSize.Y; y++) {
            for (int x = 0; x < InGridSize.X; x++) {
                FIntVector Coord(x, y, z);
                UFlowAbstractNode* Node = InGraph->CreateNode();
                Node->Coord = FVector(x, y, z);
                Nodes.Add(Coord, Node->NodeId);

                if (x > 0) {
                    FGuid PrevNodeX = Nodes[FIntVector(x - 1, y, z)];
                    InGraph->CreateLink(PrevNodeX, Node->NodeId);
                }

                if (y > 0) {
                    FGuid PrevNodeY = Nodes[FIntVector(x, y - 1, z)];
                    InGraph->CreateLink(PrevNodeY, Node->NodeId);
                }
                if (z > 0) {
                    FGuid PrevNodeZ = Nodes[FIntVector(x, y, z - 1)];
                    InGraph->CreateLink(PrevNodeZ, Node->NodeId);
                }
            }
        }
    }
}

bool UGridFlowLayoutTaskCreateGrid::GetParameter(const FString& InParameterName, FDAAttribute& OutValue) {
    FLOWTASKATTR_GET_SIZE(GridSize);

    return false;
}

bool UGridFlowLayoutTaskCreateGrid::SetParameter(const FString& InParameterName, const FDAAttribute& InValue) {
    FLOWTASKATTR_SET_SIZEI(GridSize)

    return false;
}

bool UGridFlowLayoutTaskCreateGrid::SetParameterSerialized(const FString& InParameterName, const FString& InSerializedText) {
    FLOWTASKATTR_SET_PARSE_SIZEI(GridSize)

    return false;
}

