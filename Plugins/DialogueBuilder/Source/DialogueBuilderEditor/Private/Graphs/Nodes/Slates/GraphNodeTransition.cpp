// Copyright 2022 Greg Shynkar. All Rights Reserved


#include "Graphs/Nodes/Slates/GraphNodeTransition.h"


void SGraphNodeTransition::Construct(const FArguments& InArgs, UTransitionNodeDB* InNode)
{
	this->GraphNode = InNode;
	this->UpdateGraphNode();
}


void SGraphNodeTransition::UpdateGraphNode()
{
	InputPins.Empty();
	OutputPins.Empty();

	// Reset variables that are going to be exposed, in case we are refreshing an already setup node.
	RightNodeBox.Reset();
	LeftNodeBox.Reset();
	
	this->SetVisibility(EVisibility::Visible);
	this->ContentScale.Bind(this, &SGraphNode::GetContentScale);
	this->GetOrAddSlot(ENodeZone::Center)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
		[
			SNew(SImage)
			.Image(FAppStyle::GetBrush("Graph.TransitionNode.ColorSpill"))
		.ColorAndOpacity(this, &SGraphNodeTransition::GetTransitionColor)
		]
	+ SOverlay::Slot()
		[
			SNew(SImage)
			.Image(FAppStyle::GetBrush("Graph.TransitionNode.Icon_Inertialization"))
		]
		];
}

FSlateColor SGraphNodeTransition::GetTransitionColor() const
{
	UTransitionNodeDB* TransNode = CastChecked<UTransitionNodeDB>(GraphNode);
	return StaticGetTransitionColor(TransNode, IsHovered());
}


bool SGraphNodeTransition::RequiresSecondPassLayout() const
{
	return true;
}


void SGraphNodeTransition::PerformSecondPassLayout(const TMap< UObject*, TSharedRef<SNode> >& NodeToWidgetLookup) const
{
	UTransitionNodeDB* TransNode = CastChecked<UTransitionNodeDB>(GraphNode);

	// Find the geometry of the state nodes we're connecting
	FGeometry StartGeom;
	FGeometry EndGeom;

	int32 TransIndex = 0;
	int32 NumOfTrans = 1;

	UEdGraphNode* PrevState = TransNode->GetPreviousState();
	UEdGraphNode* NextState = TransNode->GetNextState();
	if ((PrevState != NULL) && (NextState != NULL))
	{
		const TSharedRef<SNode>* pPrevNodeWidget = NodeToWidgetLookup.Find(PrevState);
		const TSharedRef<SNode>* pNextNodeWidget = NodeToWidgetLookup.Find(NextState);
		if ((pPrevNodeWidget != NULL) && (pNextNodeWidget != NULL))
		{
			const TSharedRef<SNode>& PrevNodeWidget = *pPrevNodeWidget;
			const TSharedRef<SNode>& NextNodeWidget = *pNextNodeWidget;

			StartGeom = FGeometry(FVector2D(PrevState->NodePosX, PrevState->NodePosY), FVector2D::ZeroVector, PrevNodeWidget->GetDesiredSize(), 1.0f);
			EndGeom = FGeometry(FVector2D(NextState->NodePosX, NextState->NodePosY), FVector2D::ZeroVector, NextNodeWidget->GetDesiredSize(), 1.0f);
		}
	}

	//Position Node
	PositionBetweenTwoNodesWithOffset(StartGeom, EndGeom, TransIndex, NumOfTrans);
}

void SGraphNodeTransition::PositionBetweenTwoNodesWithOffset(const FGeometry& StartGeom, const FGeometry& EndGeom, int32 NodeIndex, int32 MaxNodes) const
{
	// Get a reasonable seed point (halfway between the boxes)
	const FVector2D StartCenter = FGeometryHelper::CenterOf(StartGeom);
	const FVector2D EndCenter = FGeometryHelper::CenterOf(EndGeom);
	const FVector2D SeedPoint = (StartCenter + EndCenter) * 0.5f;

	// Find the (approximate) closest points between the two boxes
	const FVector2D StartAnchorPoint = FGeometryHelper::FindClosestPointOnGeom(StartGeom, SeedPoint);
	const FVector2D EndAnchorPoint = FGeometryHelper::FindClosestPointOnGeom(EndGeom, SeedPoint);

	// Position ourselves halfway along the connecting line between the nodes, elevated away perpendicular to the direction of the line
	const float Height = 30.0f;

	const FVector2D DesiredNodeSize = GetDesiredSize();

	FVector2D DeltaPos(EndAnchorPoint - StartAnchorPoint);

	if (DeltaPos.IsNearlyZero())
	{
		DeltaPos = FVector2D(10.0f, 0.0f);
	}

	const FVector2D Normal = FVector2D(DeltaPos.Y, -DeltaPos.X).GetSafeNormal();

	const FVector2D NewCenter = StartAnchorPoint + (0.5f * DeltaPos) + (Height * Normal);

	FVector2D DeltaNormal = DeltaPos.GetSafeNormal();

	// Calculate node offset in the case of multiple transitions between the same two nodes
	// MultiNodeOffset: the offset where 0 is the centre of the transition, -1 is 1 <size of node>
	// towards the PrevStateNode and +1 is 1 <size of node> towards the NextStateNode.

	const float MutliNodeSpace = 0.2f; // Space between multiple transition nodes (in units of <size of node> )
	const float MultiNodeStep = (1.f + MutliNodeSpace); //Step between node centres (Size of node + size of node spacer)

	const float MultiNodeStart = -((MaxNodes - 1) * MultiNodeStep) / 2.f;
	const float MultiNodeOffset = MultiNodeStart + (NodeIndex * MultiNodeStep);

	// Now we need to adjust the new center by the node size, zoom factor and multi node offset
	const FVector2D NewCorner = NewCenter - (0.5f * DesiredNodeSize) + (DeltaNormal * MultiNodeOffset * DesiredNodeSize.Size());

	GraphNode->NodePosX = NewCorner.X;
	GraphNode->NodePosY = NewCorner.Y;
}

FLinearColor SGraphNodeTransition::StaticGetTransitionColor(UTransitionNodeDB* TransNode, bool bIsHovered)
{
	FLinearColor HoverColor(0.724f, 0.256f, 0.0f, 1.0f);
	FLinearColor BaseColor(0.9f, 0.9f, 0.9f, 1.0f);

	if (bIsHovered)
	{
		return HoverColor;
	}

	return BaseColor;
}
