//$ Copyright 2015-23, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Flow/Domains/LayoutGraph/Tasks/BaseFlowLayoutTaskCreateMainPath.h"
#include "Frameworks/FlowImpl/CellFlow/Lib/CellFlowStructs.h"
#include "CellFlowLayoutTaskCreateMainPath.generated.h"

UCLASS(Meta = (AbstractTask, Title = "Create Main Path", Tooltip = "Create a main path with spawn and goal", MenuPriority = 1100))
class UCellFlowLayoutTaskCreateMainPath : public UBaseFlowLayoutTaskCreateMainPath {
	GENERATED_BODY()

};

