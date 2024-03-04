// Copyright 2022 Greg Shynkar. All Rights Reserved

#pragma once

#include "ConnectionDrawingPolicy.h"
#include "Graphs/Nodes/TransitionNodeDB.h"
#include "SGraphPin.h"
#include "SGraphNode.h"

class SGraphNodeTransition : public SGraphNode
{
public:
	SLATE_BEGIN_ARGS(SGraphNodeTransition) {}
	SLATE_END_ARGS()

	// Start SGraphNode Interface override
	void Construct(const FArguments& InArgs, UTransitionNodeDB* InNode);
	virtual void UpdateGraphNode() override;
	// End SGraphNode Interface override

	//Start SNodePanel::SNode Interface override
	virtual bool RequiresSecondPassLayout() const override;
	virtual void PerformSecondPassLayout(const TMap< UObject*, TSharedRef<SNode> >& NodeToWidgetLookup) const override;
	//End SNodePanel::SNode Interface override

	static FLinearColor StaticGetTransitionColor(UTransitionNodeDB* TransNode, bool bIsHovered);

protected:

	FSlateColor GetTransitionColor() const;
	void PositionBetweenTwoNodesWithOffset(const FGeometry& StartGeom, const FGeometry& EndGeom, int32 NodeIndex, int32 MaxNodes) const;
};
