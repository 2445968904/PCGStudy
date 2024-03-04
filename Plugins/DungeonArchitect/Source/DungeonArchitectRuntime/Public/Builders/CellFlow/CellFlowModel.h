//$ Copyright 2015-23, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/DungeonModel.h"
#include "CellFlowModel.generated.h"

class UCellFlowVoronoiGraph;
class UDAFlowCellGraph;
class UCellFlowLayoutGraph;

UCLASS()
class DUNGEONARCHITECTRUNTIME_API UCellFlowModel : public UDungeonModel {
	GENERATED_BODY()
public:
	virtual void Reset() override;

public:	
	UPROPERTY()
	TObjectPtr<UCellFlowLayoutGraph> LayoutGraph{};

	UPROPERTY()
	TObjectPtr<UDAFlowCellGraph> CellGraph{};

	UPROPERTY()
	TObjectPtr<UCellFlowVoronoiGraph> VoronoiData{};
};

