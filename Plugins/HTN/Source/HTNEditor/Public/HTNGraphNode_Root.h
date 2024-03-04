// Copyright 2020-2024 Maksym Maisak. All Rights Reserved.

#pragma once 

#include "CoreMinimal.h"
#include "HTNGraphNode.h"

#include "HTNGraphNode_Root.generated.h"

// Root node of this HTN, specify the Blackboard asset used by the HTN here.
UCLASS()
class UHTNGraphNode_Root : public UHTNGraphNode
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "AI|HTN")
	class UBlackboardData* BlackboardAsset;

	UHTNGraphNode_Root(const FObjectInitializer& Initializer);
	
	virtual void PostPlacedNewNode() override;
	virtual void AllocateDefaultPins() override;
	virtual bool CanDuplicateNode() const override { return false; }
	virtual bool CanUserDeleteNode() const override { return false; }
	virtual bool HasErrors() const override { return false; }
	virtual bool RefreshNodeClass() override { return false; }
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditUndo() override;

	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetDescription() const override;
	virtual FText GetTooltipText() const override;

	virtual void GetNodeContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const override;

private:
	void UpdateBlackboard() const;
};