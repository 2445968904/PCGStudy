// Copyright 2022 PufStudio. All Rights Reserved.

#include "AutoPaintRule.h"
#include "TiledLevelAsset.h"
#include "TiledLevelUtility.h"

#define LOCTEXT_NAMESPACE "AutoPaintRuleEditor"

UAutoPaintItemRule::UAutoPaintItemRule(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
}

void UAutoPaintItemRule::UpdateImpactSize(EImpactSize NewImpactSize)
{
	if (NewImpactSize < ImpactSize)
	{
		TArray<FIntVector> AdjPos;
		AdjacencyRules.GenerateKeyArray(AdjPos);
		const int ToReserve = NewImpactSize == EImpactSize::One? 1 : 2;
		AdjPos.RemoveAll([=](const FIntVector& Pos)
		{
			return FMath::Abs(Pos.X) < ToReserve && FMath::Abs(Pos.Y) < ToReserve && FMath::Abs(Pos.Z) < ToReserve;
		});
		for (FIntVector Pos : AdjPos)
		{
			AdjacencyRules.Remove(Pos);
		}
	}
	ImpactSize = NewImpactSize;
}

FTiledItemObj UAutoPaintItemRule::PickWhatToSpawn() const
{
	TArray<float> TempWeights;
	for (auto [k, v] : AutoPaintItemSpawns)
	{
		
		TempWeights.Add(v.RandomCoefficient);
	}
	TArray<float> Weights = FTiledLevelUtility::GetWeightedCoefficient(TempWeights);
	const float RandValue = FMath::FRand();
	int i = 0;
	for (auto [k, v] : AutoPaintItemSpawns)
	{
		if (Weights[i] > RandValue)
		{
			return k;
		}
		i++;
	}
	return FTiledItemObj();
}

TArray<EDuplicationType> UAutoPaintItemRule::GetDuplicateCases() const
{
	TArray<EDuplicationType> Out;
	Out.Add(EDuplicationType::No);
	if (RuleDuplication == ERuleDuplication::Mirrored)
		Out.Add(EDuplicationType::R2);
	else if (RuleDuplication == ERuleDuplication::Rotated)
	{
		Out.Add(EDuplicationType::R1);
		Out.Add(EDuplicationType::R2);
		Out.Add(EDuplicationType::R3);
	}
	return Out;
}


#if WITH_EDITOR
void UAutoPaintItemRule::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	UObject::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property == nullptr) return;
	if (PropertyChangedEvent.Property->GetFName() == "PlacedType")
	{
		AutoPaintItemSpawns.Empty();
	}
	if (PropertyChangedEvent.Property->GetFName() == "Extent" || (PropertyChangedEvent.MemberProperty != nullptr
		&& PropertyChangedEvent.MemberProperty->GetFName() == "Extent"))
	{
		AutoPaintItemSpawns.Empty();
	}
	if (PropertyChangedEvent.Property->GetFName() == "StructureType" || (PropertyChangedEvent.MemberProperty != nullptr
		&& PropertyChangedEvent.MemberProperty->GetFName() == "StructureType"))
	{
		AutoPaintItemSpawns.Empty();
	}
	

	// clamp offset to a particular range
	if (!bIncludeZ) PositionOffset.Z = 0;
	FIntVector MaxOffset = FIntVector(0);
	FIntVector MinOffset = FIntVector(0);
	const EPlacedShapeType Shape = FTiledLevelUtility::PlacedTypeToShape(PlacedType);
	switch (ImpactSize) { 
	case EImpactSize::Three:
		MaxOffset = FIntVector(1);
		MinOffset = FIntVector(-1);
		break;
	case EImpactSize::Five:
		MaxOffset = FIntVector(2);
		MinOffset = FIntVector(-2);
		break;
	default: break;
	}
	if (Shape == EPlacedShapeType::Shape2D)
	{
		Extent.Y = Extent.X;
		if (EdgeType == EEdgeType::Horizontal)
		{
			MaxOffset.Y += 1;
			MaxOffset.X -= Extent.X - 1;	
		} else
		{
			MaxOffset.X += 1;
			MaxOffset.Y -= Extent.X - 1;	
		}
	}
	else if (Shape == EPlacedShapeType::Shape1D)
	{
		Extent.X = Extent.Y = 1;
		MaxOffset.X += 1;
		MaxOffset.Y += 1;
	}
	
	// PositionOffset = FMath::Clamp(PositionOffset, MinOffset, MaxOffset); // Can not clamp FIntVector???
	if (PositionOffset.X > MaxOffset.X) PositionOffset.X = MaxOffset.X;
	if (PositionOffset.Y > MaxOffset.Y) PositionOffset.Y = MaxOffset.Y;
	if (PositionOffset.Z > MaxOffset.Z) PositionOffset.Z = MaxOffset.Z;
	if (PositionOffset.X < MinOffset.X) PositionOffset.X = MinOffset.X;
	if (PositionOffset.Y < MinOffset.Y) PositionOffset.Y = MinOffset.Y;
	if (PositionOffset.Z < MinOffset.Z) PositionOffset.Z = MinOffset.Y;

	if (StepSize.X < 1) StepSize.X = 1;
	if (StepSize.Y < 1) StepSize.Y = 1;
	if (StepSize.Z < 1) StepSize.Z = 1;
	StepOffset.X = FMath::Clamp(StepOffset.X, 0, StepSize.X - 1);
	StepOffset.Y = FMath::Clamp(StepOffset.Y, 0, StepSize.Y - 1);
	StepOffset.Z = FMath::Clamp(StepOffset.Z, 0, StepSize.Z - 1);

	// restrict rotation for edge direction
	if (Shape == EPlacedShapeType::Shape2D)
	{
		for (auto It = AutoPaintItemSpawns.CreateIterator(); It; ++It)
		{
			uint8 RawValue = It.Value().RotationTimes;
			if (RawValue <= 1)
			{
				It.Value().RotationTimes = EdgeType == EEdgeType::Horizontal? 0 : 1;
			}
			else
			{
				It.Value().RotationTimes = EdgeType == EEdgeType::Horizontal? 2 : 3;
			}
		}
	}
	
	if (UAutoPaintRule* Rule =  Cast<UAutoPaintRule>(GetOutermostObject()))
	{
		Rule->RefreshSpawnedInstance_Delegate.Execute();
	    Rule->RequestForUpdateMatchHint.Execute(this);
	}
}
#endif

TArray<UAutoPaintItemRule*> UAutoPaintRule::GetEnabledRuleList()
{
	TArray<UAutoPaintItemRule*> OutRules;
	for (auto Group : Groups)
	{
		if (!Group.bEnabled) continue;
		for (auto Rule : Group.Rules)
		{
			if (!Rule->bEnabled) continue;
			OutRules.Add(Rule);
		}
	}
	return OutRules;
}

void UAutoPaintRule::ReorderItem(const FName& ItemName1, const FName& ItemName2)
{
	const int idx_1 = Items.IndexOfByKey(ItemName1);
	const int idx_2 = Items.IndexOfByKey(ItemName2);
	Items.Swap(idx_1, idx_2);
	RefreshAutoPalette_Delegate.Execute();
}

bool UAutoPaintRule::IsRuleMet(UAutoPaintItemRule* QueryRule, EDuplicationType DupCase, FIntVector QueryPosition,
	const TMap<FIntVector, FName>& ExistingData, FIntVector MinBoundary, FIntVector MaxBoundary)
{
	// return no need to test
	if (QueryRule->AutoPaintItemSpawns.IsEmpty()) return false;
	if (QueryRule->Probability + 0.00001 < FMath::FRand()) return false;
	const FIntVector StepPosition = QueryPosition - QueryRule->StepOffset;
	if (StepPosition.X % QueryRule->StepSize.X != 0 ||
		StepPosition.Y % QueryRule->StepSize.Y != 0 ||
		StepPosition.Z % QueryRule->StepSize.Z != 0)
		return false;
	
	// rotate raw adj rule points in response to dup case...
	TMap<FIntVector, FAdjacencyRule> ToCheck;
	for (auto [k, v] : QueryRule->AdjacencyRules)
	{
		ToCheck.Add(RotatePosToDuplicated(k, DupCase), v);
	}
	// early return for OOB checking
	// place it earlier than creating padding data... this should improve performance a little bit...
	for (auto [AdjPoint, AdjInfo] : ToCheck)
	{
		FIntVector TestPoint = AdjPoint + QueryPosition;
		if (QueryRule->OutOfBoundaryRule == "Not Applicable")
		{
			if (TestPoint.X < MinBoundary.X || TestPoint.Y < MinBoundary.Y || TestPoint.Z < MinBoundary.Z)
				return false;
			if (TestPoint.X > MaxBoundary.X || TestPoint.Y > MaxBoundary.Y || TestPoint.Z > MaxBoundary.Z)
				return false;
		}
	}
	// create padding for OOB check
	TMap<FIntVector, FName> ExistingData_Padding = ExistingData;
	if (QueryRule->OutOfBoundaryRule != "Empty" && QueryRule->ImpactSize != EImpactSize::One)
	{
		const int PaddingSize = QueryRule->ImpactSize == EImpactSize::Three? 1 : 2;
		bool NeedToPad = true;
		if (QueryPosition.X > MinBoundary.X + PaddingSize - 1 && QueryPosition.X < MaxBoundary.X + PaddingSize - 2 &&
			QueryPosition.Y > MinBoundary.Y + PaddingSize - 1 && QueryPosition.Y < MaxBoundary.Y + PaddingSize - 2 &&
			QueryPosition.Z > MinBoundary.Z + PaddingSize - 1 && QueryPosition.Z < MaxBoundary.Z + PaddingSize - 2
			)
		{
			NeedToPad = false;
		}

		if (NeedToPad)
		{
			for (int x = MinBoundary.X - PaddingSize; x < MaxBoundary.X + PaddingSize; x++)
			{
				for (int y = MinBoundary.Y - PaddingSize; y < MaxBoundary.Y + PaddingSize; y++)
				{
					for (int z = MinBoundary.Z - PaddingSize; z < MaxBoundary.Z + PaddingSize; z++)
					{
						if (x >= MinBoundary.X && x < MaxBoundary.X && y >= MinBoundary.Y && y < MaxBoundary.Y && z >= MinBoundary.Z && z < MaxBoundary.Z)
							continue;
						ExistingData_Padding.Add(FIntVector(x, y, z), QueryRule->OutOfBoundaryRule);
					}
				}
			}
		}
	}
	// The actual checking algorithm...
	for (auto [AdjPoint, AdjInfo] : ToCheck)
	{
		FIntVector TestPoint = AdjPoint + QueryPosition;
		// if any not met return false
		if (ExistingData_Padding.Contains(TestPoint))
		{
			if (ExistingData_Padding[TestPoint] != AdjInfo.AutoPaintItemName && AdjInfo.bNegate)
			{
				continue;
			}
			if (ExistingData_Padding[TestPoint] == AdjInfo.AutoPaintItemName && !AdjInfo.bNegate)
			{
				continue;
			}
			return false;
		}
		if (AdjInfo.AutoPaintItemName != TEXT("Empty") && AdjInfo.bNegate)
			continue;
		if (AdjInfo.AutoPaintItemName == TEXT("Empty"))
			continue;
		return false;
	}
	return true;
}

FIntVector UAutoPaintRule::RotatePosToDuplicated(const FIntVector& InPos, EDuplicationType DupType)
{
	switch (DupType) {
	case EDuplicationType::R1:
		return FTiledLevelUtility::RotationXY(InPos, 90);
	case EDuplicationType::R2:
		return FTiledLevelUtility::RotationXY(InPos, 180);
	case EDuplicationType::R3:
		return FTiledLevelUtility::RotationXY(InPos, 270);
	case EDuplicationType::No:
		return InPos;
	default: ;
	}
	return InPos;
}

FDuplicationMod UAutoPaintRule::GetDuplicationMod(EDuplicationType DupType, EPlacedShapeType WhichShape, uint8 RotTimes,
	FIntVector Pos)
{
	if (DupType == EDuplicationType::No)
 		return FDuplicationMod(RotTimes, Pos);
 	FDuplicationMod Out;
 	uint8 RMod = RotTimes;
 	float RotateDegree = 0.f;
 	switch (DupType) {
 	case EDuplicationType::R1:
 		RotateDegree = 90.f;
 		RMod += 1;
 		break;
 	case EDuplicationType::R2:
 		RotateDegree = 180.f;
 		RMod += 2;
 		break;
 	case EDuplicationType::R3:
 		RotateDegree = 270.f;
 		RMod += 3;
 		break;
 	}
 	if (RMod > 3)
 		RMod -= 4;
 	Out.RotationTimes = RMod;
 
 	switch (WhichShape) {
 	case Shape3D:
 		Out.PosOffset = FTiledLevelUtility::RotationXY(Pos, RotateDegree);
 		break;
 	case Shape2D:
 	{

// TODO: this is still very wrong? it's working on horizontal edge now... is still wrong for vertical ones... 			
 		const FVector VOffset = FTiledLevelUtility::RotationXY(FVector(Pos) - FVector(0.5), RotateDegree) + FVector(0.5);
 		Out.PosOffset = FIntVector(FMath::RoundToInt(VOffset.X), FMath::RoundToInt(VOffset.Y), FMath::RoundToInt(VOffset.Z));
		if (RotTimes == 0 || RotTimes == 2) // horizontal edge
		{
			if (DupType == EDuplicationType::R2)
				Out.PosOffset.X -= 1;
			if (DupType == EDuplicationType::R3)
				Out.PosOffset.Y -= 1;
		}
		else
		{
			if (DupType == EDuplicationType::R1)
				Out.PosOffset.X -= 1;
			if (DupType == EDuplicationType::R2)
				Out.PosOffset.Y -= 1;
		}
 		break;
 	}
 	case Shape1D:
 	{
 		const FVector VOffset = FTiledLevelUtility::RotationXY(FVector(Pos) - FVector(0.5), RotateDegree) + FVector(0.5);
 		Out.PosOffset = FIntVector(FMath::RoundToInt(VOffset.X), FMath::RoundToInt(VOffset.Y), FMath::RoundToInt(VOffset.Z));
 		break;
 	}
 	}
 	return Out;
}

UTiledItemSet* UAutoPaintRule::GetItemSet()
{
	if (UTiledLevelAsset* TA = Cast<UTiledLevelAsset>(GetOuter()))
	{
		return TA->GetItemSetAsset();
	}
	return SourceItemSet.Get();
}

TMap<UAutoPaintItemRule*, TArray<FIntVector>> UAutoPaintRule::GetMatchedPositions()
{
	TMap<UAutoPaintItemRule*, TArray<FIntVector>> Out;
	for (auto Data : MatchData)
	{
		if (Out.Contains(Data.BaseRulePtr))
			Out[Data.BaseRulePtr].Add(Data.MetPos);
		else
		{
			Out.Add(Data.BaseRulePtr, {Data.MetPos});
		}
	}
	return Out;
}

#if WITH_EDITOR

void UAutoPaintRule::PostEditUndo()
{
	RefreshAutoPalette_Delegate.Execute();
	RefreshRuleEditor_Delegate.Execute();
	RefreshSpawnedInstance_Delegate.Execute();
	UObject::PostEditUndo();
}
#endif

#undef LOCTEXT_NAMESPACE