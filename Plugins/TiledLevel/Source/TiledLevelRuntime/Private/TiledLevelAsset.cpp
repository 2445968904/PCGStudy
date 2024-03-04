// Copyright 2022 PufStudio. All Rights Reserved.

#include "TiledLevelAsset.h"
#include "AutoPaintRule.h"
#include "TiledLevel.h"
#include "TiledLevelEditorLog.h"
#include "TiledLevelItem.h"
#include "TiledLevelUtility.h"
#include "Components/StaticMeshComponent.h"
#include "Misc/MessageDialog.h"

#define LOCTEXT_NAMESPACE "TiledLevel"

/*
 *  TODO: trigger update of auto paint data here is just a temporarily solution, need to trigger it inside tiled level actor somehow...
 */

UTiledLevelAsset::UTiledLevelAsset()
{
}

void UTiledLevelAsset::ConfirmTileSize()
{
	CanEditTileSize = false;
}

void UTiledLevelAsset::ResetTileSize()
{
	bool Proceed = true;
	const int TotalPlacements = GetNumOfAllPlacements();
	if (TotalPlacements > 0)
	{
		FText Message = FText::Format(
			LOCTEXT("ResetAssetTileSize", "Reset Tile Size will clear all placements! ({0}) Are you sure?"),
			FText::AsNumber(TotalPlacements));
		Proceed = FMessageDialog::Open(EAppMsgType::OkCancel, Message) == EAppReturnType::Ok;
	}
	if (Proceed)
	{
		this->Modify();
		CanEditTileSize = true;
		EmptyAllFloors();
		if (HostLevel)
			HostLevel->ResetAllInstance();
		OnResetTileSize.ExecuteIfBound();
	}
}

void UTiledLevelAsset::SetNewViewState(bool AsAutoPaint)
{
	if (AsAutoPaint == ViewAsAutoPaint) return;
	ViewAsAutoPaint = AsAutoPaint;
	VersionNumber += 1;
	if (HostLevel)
	{
		HostLevel->ResetAllInstance();
	}
}

void UTiledLevelAsset::ClearOrphan()
{
	int N_Removed = 0;
	for (FTiledFloor& Floor : TiledFloors)
	{
		N_Removed += Floor.BlockPlacements.RemoveAll([this](const FTilePlacement& TP)
		{
			return TP.ItemSet != ActiveItemSet.Get();
		});
		N_Removed += Floor.FloorPlacements.RemoveAll([this](const FTilePlacement& TP)
		{
			return TP.ItemSet != ActiveItemSet.Get();
		});
		N_Removed += Floor.WallPlacements.RemoveAll([this](const FEdgePlacement& EP)
		{
			return EP.ItemSet != ActiveItemSet.Get();
		});
		N_Removed += Floor.EdgePlacements.RemoveAll([this](const FEdgePlacement& EP)
		{
			return EP.ItemSet != ActiveItemSet.Get();
		});
		N_Removed += Floor.PillarPlacements.RemoveAll([this](const FPointPlacement& PP)
		{
			return PP.ItemSet != ActiveItemSet.Get();
		});
		N_Removed += Floor.PointPlacements.RemoveAll([this](const FPointPlacement& PP)
		{
			return PP.ItemSet != ActiveItemSet.Get();
		});
	}
	DEV_LOGF("%d orphan placements are removed!", N_Removed)
	if (HostLevel)
		HostLevel->ResetAllInstance(true);
	HasOrphanPlacement = false;
}

UTiledLevelAsset* UTiledLevelAsset::CloneAsset(UObject* OuterForClone)
{
	// fixed! with add additional flags  Ensure condition failed: !(bDuplicateForPIE && DupObjectInfo.DuplicatedObject->HasAnyFlags(RF_Standalone))
	// why this issue not happened in paper2D which have to flags set? 
	return CastChecked<UTiledLevelAsset>(
		StaticDuplicateObject(this, OuterForClone, NAME_None, EObjectFlags::RF_Transactional));
}

#if WITH_EDITOR

void UTiledLevelAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = (PropertyChangedEvent.Property != nullptr)? PropertyChangedEvent.Property->GetFName() : NAME_None;

	const TArray<FName> AreaRelatedPropertyNames =
	{
		GET_MEMBER_NAME_CHECKED(UTiledLevelAsset, X_Num),
		GET_MEMBER_NAME_CHECKED(UTiledLevelAsset, Y_Num),
		GET_MEMBER_NAME_CHECKED(UTiledLevelAsset, TileSizeX),
		GET_MEMBER_NAME_CHECKED(UTiledLevelAsset, TileSizeY),
		GET_MEMBER_NAME_CHECKED(UTiledLevelAsset, TileSizeZ),
	};
	if (AreaRelatedPropertyNames.Contains(PropertyName))
	{
		OnTiledLevelAreaChanged.Broadcast(TileSizeX, TileSizeY, TileSizeZ, X_Num, Y_Num, TiledFloors.Num(),
		                                  FMath::Max(TiledFloors).FloorPosition);
		ClearOutOfBoundPlacements();
	}
	if (FTiledFloor* ActiveFloor = GetActiveFloor())
	{
		MoveHelperFloorGrids.ExecuteIfBound(ActiveFloor->FloorPosition);
	}
	VersionNumber += 1;
	UObject::PostEditChangeProperty(PropertyChangedEvent);
}

void UTiledLevelAsset::ClearOutOfBoundPlacements()
{
	int NumRemoved = 0;
	for (FTiledFloor& Floor : TiledFloors)
	{
		NumRemoved += Floor.BlockPlacements.RemoveAll([this](const FTilePlacement& P)
		{
			return P.GridPosition.X > X_Num - P.Extent.X || P.GridPosition.Y > Y_Num - P.Extent.Y;
		});
		NumRemoved += Floor.FloorPlacements.RemoveAll([this](const FTilePlacement& P)
		{
			return P.GridPosition.X > X_Num - P.Extent.X || P.GridPosition.Y > Y_Num - P.Extent.Y;
		});
		NumRemoved += Floor.WallPlacements.RemoveAll([this](const FEdgePlacement& P)
		{
			return !FTiledLevelUtility::IsEdgeInsideArea(P.Edge, P.GetItem()->Extent.X, FIntPoint(X_Num, Y_Num));
		});
		NumRemoved += Floor.EdgePlacements.RemoveAll([this](const FEdgePlacement& P)
		{
			return !FTiledLevelUtility::IsEdgeInsideArea(P.Edge, P.GetItem()->Extent.X, FIntPoint(X_Num, Y_Num));
		});
		NumRemoved += Floor.PillarPlacements.RemoveAll([this](const FPointPlacement& P)
		{
			return P.GridPosition.X > X_Num || P.GridPosition.Y > Y_Num;
		});
		NumRemoved += Floor.BlockPlacements.RemoveAll([this](const FTilePlacement& P)
		{
			return P.GridPosition.X > X_Num || P.GridPosition.Y > Y_Num;
		});
	}
	if (NumRemoved)
	{
		HostLevel->ResetAllInstance(true);
	}
}
#endif

void UTiledLevelAsset::SetActiveItemSet(UTiledItemSet* NewItemSet)
{
	ActiveItemSet = NewItemSet;
	HasOrphanPlacement = false;
	// change item set ptr in placement data if the GUID exist
	for (FTiledFloor& Floor : TiledFloors)
	{
		for (auto P : Floor.GetAllPlacements())
		{
			if (NewItemSet->GetItem(P->ItemID))
			{
				P->ItemSet = NewItemSet;
			}
			else
			{
				HasOrphanPlacement = true;
			}
		}
	}
	if (HostLevel)
		HostLevel->ResetAllInstance(true);
}

void UTiledLevelAsset::SetActiveAutoPaintRule(UAutoPaintRule* NewRule)
{
	ActiveAutoPaintRule = NewRule;
}


int UTiledLevelAsset::AddNewFloor(int32 InsertPosition)
{
	int ActualInsertPosition = InsertPosition;
	// the operator< overload is in reversed way... so use "min to get max", "max to get min"
	int32 MaxFloorPosition = FMath::Min(TiledFloors).FloorPosition;
	int32 MinFloorPosition = FMath::Max(TiledFloors).FloorPosition;
	if (InsertPosition > MaxFloorPosition)
	{
		ActualInsertPosition = MaxFloorPosition + 1;
	}
	else if (InsertPosition < MinFloorPosition)
	{
		ActualInsertPosition = MinFloorPosition - 1;
	}
	if (ActualInsertPosition >= 0)
	{
		for (FTiledFloor& F : TiledFloors)
		{
			if (F.FloorPosition >= ActualInsertPosition)
				F.UpdatePosition(F.FloorPosition + 1, TileSizeZ);
		}
	}
	else
	{
		for (FTiledFloor& F : TiledFloors)
		{
			if (F.FloorPosition <= ActualInsertPosition)
				F.UpdatePosition(F.FloorPosition - 1, TileSizeZ);
		}
	}
	TiledFloors.Add(FTiledFloor(ActualInsertPosition));
	TiledFloors.Sort();
	OnTiledLevelAreaChanged.Broadcast(TileSizeX, TileSizeY, TileSizeZ, X_Num, Y_Num, TiledFloors.Num(),
	                                  FMath::Max(TiledFloors).FloorPosition);
	return ActualInsertPosition;
}

// TODO: auto paint data need test
void UTiledLevelAsset::DuplicatedFloor(int SourcePosition)
{
	FTiledFloor* SourceFloor = GetFloorFromPosition(SourcePosition);
	TArray<FTilePlacement> BP = SourceFloor->BlockPlacements;
	TArray<FTilePlacement> FP = SourceFloor->FloorPlacements;
	TArray<FPointPlacement> PiP = SourceFloor->PillarPlacements;
	TArray<FEdgePlacement> WP = SourceFloor->WallPlacements;
	TArray<FEdgePlacement> BmP = SourceFloor->EdgePlacements;
	TArray<FPointPlacement> PP = SourceFloor->PointPlacements;
	for (auto& P : BP)
	{
		P.OnMoveFloor(TileSizeZ, true, 1);
	}
	for (auto& P : FP) // forget to add "&" stuck me one hour...
	{
		P.OnMoveFloor(TileSizeZ, true, 1);
	}
	for (auto& P : PiP)
	{
		P.OnMoveFloor(TileSizeZ, true, 1);
	}
	for (auto& P : WP)
	{
		P.OnMoveFloor(TileSizeZ, true, 1);
	}
	for (auto& P : BmP)
	{
		P.OnMoveFloor(TileSizeZ, true, 1);
	}
	for (auto& P : PP)
	{
		P.OnMoveFloor(TileSizeZ, true, 1);
	}
	int NewFloorPosition = AddNewFloor(SourcePosition + 1);
	FTiledFloor* TargetFloor = GetFloorFromPosition(NewFloorPosition);
	TargetFloor->BlockPlacements = BP;
	TargetFloor->FloorPlacements = FP;
	TargetFloor->PillarPlacements = PiP;
	TargetFloor->WallPlacements = WP;
	TargetFloor->EdgePlacements = BmP;
	TargetFloor->PointPlacements = PP;
	TMap<FIntVector, FName> ToAppend;
	for (auto [K, V] : AutoPaintData)
	{
		if (K.Z == SourcePosition)
		{
			ToAppend.Add(K + FIntVector(0, 0, 1), V);
		}
	}
	AutoPaintData.Append(ToAppend);
	if (ActiveAutoPaintRule) ActiveAutoPaintRule->RefreshSpawnedInstance_Delegate.Execute();
}

void UTiledLevelAsset::MoveAllFloors(bool Up)
{
	for (auto& F : TiledFloors)
	{
		if (Up)
			F.UpdatePosition(F.FloorPosition + 1, TileSizeZ);
		else
			F.UpdatePosition(F.FloorPosition - 1, TileSizeZ);
	}
	TMap<FIntVector, FName> NewData;
	for (auto& [K, V] : AutoPaintData)
	{
		FIntVector Mod = Up ? FIntVector(0, 0, 1) : FIntVector(0, 0, -1);
		NewData.Add(K + Mod, V);
	}
	AutoPaintData = NewData;

	if (ActiveAutoPaintRule) ActiveAutoPaintRule->RefreshSpawnedInstance_Delegate.Execute();
	OnTiledLevelAreaChanged.Broadcast(TileSizeX, TileSizeY, TileSizeZ, X_Num, Y_Num, TiledFloors.Num(),
	                                  FMath::Max(TiledFloors).FloorPosition);
}

// todo: auto paint?
void UTiledLevelAsset::MoveFloor(bool IsUp, int SelectedFloorPosition, int EndFloorPosition, bool ToEnd)
{
	if (ToEnd)
	{
		for (auto& F : TiledFloors)
		{
			if (F.FloorPosition == SelectedFloorPosition)
			{
				F.UpdatePosition(EndFloorPosition, TileSizeZ);
			}
			else if (F.FloorPosition > SelectedFloorPosition && IsUp)
			{
				F.UpdatePosition(F.FloorPosition - 1, TileSizeZ);
			}
			else if (F.FloorPosition < SelectedFloorPosition && !IsUp)
			{
				F.UpdatePosition(F.FloorPosition + 1, TileSizeZ);
			}
		}
		TMap<FIntVector, FName> NewData;
		for (auto& [K, V] : AutoPaintData)
		{
			if (K.Z == SelectedFloorPosition)
			{
				NewData.Add(FIntVector(K.X, K.Y, EndFloorPosition), V);
			}
			else if (K.Z > SelectedFloorPosition && IsUp)
			{
				NewData.Add(K - FIntVector(0, 0, 1), V);
			}
			else if (K.Z < SelectedFloorPosition && !IsUp)
			{
				NewData.Add(K + FIntVector(0, 0, 1), V);
			}
			else
			{
				NewData.Add(K, V);
			}
		}
		AutoPaintData = NewData;
		VersionNumber += 1;
	}
	else
	{
		const int Mod = IsUp ? 1 : -1;
		ChangeFloorOrdering(SelectedFloorPosition, SelectedFloorPosition + Mod);
	}
	TiledFloors.Sort();
	if (ActiveAutoPaintRule) ActiveAutoPaintRule->RefreshSpawnedInstance_Delegate.Execute();
}


void UTiledLevelAsset::ChangeFloorOrdering(int OldPosition, int NewPosition)
{
	if (IsFloorExists(OldPosition) && IsFloorExists(NewPosition))
	{
		for (auto& F : TiledFloors)
		{
			if (F.FloorPosition == OldPosition)
			{
				F.UpdatePosition(NewPosition, TileSizeZ);
			}
			else if (F.FloorPosition == NewPosition)
			{
				F.UpdatePosition(OldPosition, TileSizeZ);
			}
		}
		TMap<FIntVector, FName> NewData;
		for (auto& [K, V] : AutoPaintData)
		{
			if (K.Z == OldPosition)
			{
				NewData.Add(FIntVector(K.X, K.Y, NewPosition), V);
			}
			else if (K.Z == NewPosition)
			{
				NewData.Add(FIntVector(K.X, K.Y, OldPosition), V);
			}
			else
			{
				NewData.Add(K, V);
			}
		}
		AutoPaintData = NewData;
		VersionNumber += 1;
	}
}

// TODO: auto paint data ... need test
void UTiledLevelAsset::RemoveFloor(int32 DeleteIndex)
{
	TiledFloors.Remove(FTiledFloor(DeleteIndex));
	TMap<FIntVector, FName> NewData;
	for (auto& [K, V] : AutoPaintData)
	{
		if (K.Z == DeleteIndex) continue;
		if (DeleteIndex >= 0)
		{
			if (K.Z > DeleteIndex)
			{
				NewData.Add(K - FIntVector(0, 0, 1), V);
			}
			else
			{
				NewData.Add(K, V);
			}
		}
		else
		{
			if (K.Z < DeleteIndex)
			{
				NewData.Add(K + FIntVector(0, 0, 1), V);
			}
			else
			{
				NewData.Add(K, V);
			}
		}
	}
	AutoPaintData = NewData;

	// reorder
	if (DeleteIndex >= 0)
	{
		for (auto& F : TiledFloors)
		{
			if (F.FloorPosition > DeleteIndex)
				F.UpdatePosition(F.FloorPosition - 1, TileSizeZ);
		}
	}
	else
	{
		for (auto& F : TiledFloors)
			if (F.FloorPosition < DeleteIndex)
				F.UpdatePosition(F.FloorPosition + 1, TileSizeZ);
	}

	if (ActiveAutoPaintRule) ActiveAutoPaintRule->RefreshSpawnedInstance_Delegate.Execute();
	OnTiledLevelAreaChanged.Broadcast(TileSizeX, TileSizeY, TileSizeZ, X_Num, Y_Num, TiledFloors.Num(),
	                                  FMath::Max(TiledFloors).FloorPosition);
}

// TODO: auto paint data .. need test
void UTiledLevelAsset::EmptyFloor(int FloorPosition)
{
	if (FTiledFloor* F = GetFloorFromPosition(FloorPosition))
	{
		F->BlockPlacements.Empty();
		F->FloorPlacements.Empty();
		F->WallPlacements.Empty();
		F->EdgePlacements.Empty();
		F->PillarPlacements.Empty();
	}
	TArray<FIntVector> ToRemove;
	for (auto& [K, V] : AutoPaintData)
		if (K.Z == FloorPosition) ToRemove.Add(K);
	for (auto& P : ToRemove)
		AutoPaintData.Remove(P);
	if (ActiveAutoPaintRule) ActiveAutoPaintRule->RefreshSpawnedInstance_Delegate.Execute();
	VersionNumber += 100;
}

// TODO: auto paint data ... need test
void UTiledLevelAsset::EmptyAllFloors()
{
	for (FTiledFloor& F : TiledFloors)
	{
		F.BlockPlacements.Empty();
		F.FloorPlacements.Empty();
		F.WallPlacements.Empty();
		F.EdgePlacements.Empty();
		F.PillarPlacements.Empty();
	}
	AutoPaintData.Empty();
	if (ActiveAutoPaintRule) ActiveAutoPaintRule->RefreshSpawnedInstance_Delegate.Execute();
	VersionNumber += 100;
}

bool UTiledLevelAsset::IsFloorExists(int32 PositionIndex)
{
	TArray<int> ExistsFloorPosition;
	for (const FTiledFloor& F : TiledFloors)
	{
		ExistsFloorPosition.Add(F.FloorPosition);
	}
	return ExistsFloorPosition.Contains(PositionIndex);
}

void UTiledLevelAsset::RemovePlacements(const TArray<FTilePlacement>& TilesToDelete)
{
	for (FTiledFloor& F : TiledFloors)
	{
		F.BlockPlacements.RemoveAll([=](const FTilePlacement& P)
		{
			return TilesToDelete.Contains(P);
		});
		F.FloorPlacements.RemoveAll([=](const FTilePlacement& P)
		{
			return TilesToDelete.Contains(P);
		});
	}
}

void UTiledLevelAsset::RemovePlacements(const TArray<FEdgePlacement>& WallsToDelete)
{
	for (FTiledFloor& F : TiledFloors)
	{
		F.WallPlacements.RemoveAll([=](const FEdgePlacement& P)
		{
			return WallsToDelete.Contains(P);
		});
		F.EdgePlacements.RemoveAll([=](const FEdgePlacement& P)
		{
			return WallsToDelete.Contains(P);
		});
	}
}

void UTiledLevelAsset::RemovePlacements(const TArray<FPointPlacement>& PointsToDelete)
{
	for (FTiledFloor& F : TiledFloors)
	{
		F.PillarPlacements.RemoveAll([=](const FPointPlacement& P)
		{
			return PointsToDelete.Contains(P);
		});
		F.PointPlacements.RemoveAll([=](const FPointPlacement& P)
		{
			return PointsToDelete.Contains(P);
		});
	}
}

void UTiledLevelAsset::ClearItem(const FGuid& ItemID)
{
	for (FTiledFloor& F : TiledFloors)
	{
		if (F.BlockPlacements.RemoveAll([=](const FTilePlacement& P)
		{
			return P.ItemID == ItemID;
		}))
			continue;
		if (F.FloorPlacements.RemoveAll([=](const FTilePlacement& P)
		{
			return P.ItemID == ItemID;
		}))
			continue;
		if (F.WallPlacements.RemoveAll([=](const FEdgePlacement& P)
		{
			return P.ItemID == ItemID;
		}))
			continue;
		if (F.EdgePlacements.RemoveAll([=](const FEdgePlacement& P)
		{
			return P.ItemID == ItemID;
		}))
			continue;
		if (F.PillarPlacements.RemoveAll([=](const FPointPlacement& P)
		{
			return P.ItemID == ItemID;
		}))
			continue;
		F.PointPlacements.RemoveAll([=](const FPointPlacement& P)
		{
			return P.ItemID == ItemID;
		});
	}
	VersionNumber += 1;
}

void UTiledLevelAsset::ClearItemInActiveFloor(const FGuid& ItemID)
{
	GetActiveFloor()->BlockPlacements.RemoveAll([=](const FTilePlacement& P)
	{
		return P.ItemID == ItemID;
	});
	GetActiveFloor()->FloorPlacements.RemoveAll([=](const FTilePlacement& P)
	{
		return P.ItemID == ItemID;
	});
	GetActiveFloor()->WallPlacements.RemoveAll([=](const FEdgePlacement& P)
	{
		return P.ItemID == ItemID;
	});
	GetActiveFloor()->EdgePlacements.RemoveAll([=](const FEdgePlacement& P)
	{
		return P.ItemID == ItemID;
	});
	GetActiveFloor()->PillarPlacements.RemoveAll([=](const FPointPlacement& P)
	{
		return P.ItemID == ItemID;
	});
	GetActiveFloor()->PointPlacements.RemoveAll([=](const FPointPlacement& P)
	{
		return P.ItemID == ItemID;
	});
	VersionNumber += 1;
}

void UTiledLevelAsset::ClearInvalidPlacements()
{
	TArray<FGuid> InvalidIndices;
	for (FTiledFloor& F : TiledFloors)
	{
		for (FItemPlacement& P : F.GetItemPlacements())
		{
			if (!P.GetItem())
				InvalidIndices.AddUnique(P.ItemID);
		}
	}
	for (FGuid& TestID : InvalidIndices)
	{
		ClearItem(TestID);
	}
}

FTiledFloor* UTiledLevelAsset::GetActiveFloor()
{
	return TiledFloors.FindByPredicate([this](const FTiledFloor& TestFloor)
	{
		return TestFloor.FloorPosition == ActiveFloorPosition;
	});
}

FTiledFloor* UTiledLevelAsset::GetBelowActiveFloor()
{
	return GetFloorFromPosition(ActiveFloorPosition - 1);
}

FTiledFloor* UTiledLevelAsset::GetFloorFromPosition(int PositionIndex)
{
	return TiledFloors.FindByPredicate([=](const FTiledFloor& TestFloor)
	{
		return TestFloor.FloorPosition == PositionIndex;
	});
}

void UTiledLevelAsset::AddNewTilePlacement(FTilePlacement NewTile)
{
	UTiledLevelItem* Item = NewTile.GetItem();
	if (!Item) return;
	FTiledFloor* TargetFloor = GetFloorFromPosition(NewTile.GridPosition.Z);
	switch (Item->PlacedType)
	{
	case EPlacedType::Block:
		TargetFloor->BlockPlacements.Add(NewTile);
		break;
	case EPlacedType::Floor:
		TargetFloor->FloorPlacements.Add(NewTile);
		break;
	default: ;
	}
}

void UTiledLevelAsset::AddNewEdgePlacement(FEdgePlacement NewEdge)
{
	UTiledLevelItem* Item = NewEdge.GetItem();
	if (!Item) return;
	FTiledFloor* TargetFloor = GetFloorFromPosition(NewEdge.Edge.Z);
	switch (Item->PlacedType)
	{
	case EPlacedType::Wall:
		TargetFloor->WallPlacements.Add(NewEdge);
		break;
	case EPlacedType::Edge:
		TargetFloor->EdgePlacements.Add(NewEdge);
	default: ;
	}
}

void UTiledLevelAsset::AddNewPointPlacement(FPointPlacement NewPoint)
{
	UTiledLevelItem* Item = NewPoint.GetItem();
	if (!Item) return;
	FTiledFloor* TargetFloor = GetFloorFromPosition(NewPoint.GridPosition.Z);
	switch (Item->PlacedType)
	{
	case EPlacedType::Pillar:
		TargetFloor->PillarPlacements.Add(NewPoint);
		break;
	case EPlacedType::Point:
		TargetFloor->PointPlacements.Add(NewPoint);
		break;
	default: ;
	}
}

TArray<FTilePlacement> UTiledLevelAsset::GetAllBlockPlacements() const
{
	TArray<FTilePlacement> OutArray;
	for (FTiledFloor F : TiledFloors)
	{
		OutArray.Append(F.BlockPlacements);
	}
	return OutArray;
}

TArray<FTilePlacement> UTiledLevelAsset::GetAllFloorPlacements() const
{
	TArray<FTilePlacement> OutArray;
	for (FTiledFloor F : TiledFloors)
	{
		OutArray.Append(F.FloorPlacements);
	}
	return OutArray;
}

TArray<FPointPlacement> UTiledLevelAsset::GetAllPillarPlacements() const
{
	TArray<FPointPlacement> OutArray;
	for (FTiledFloor F : TiledFloors)
	{
		OutArray.Append(F.PillarPlacements);
	}
	return OutArray;
}


TArray<FEdgePlacement> UTiledLevelAsset::GetAllWallPlacements() const
{
	TArray<FEdgePlacement> OutArray;
	for (FTiledFloor F : TiledFloors)
	{
		OutArray.Append(F.WallPlacements);
	}
	return OutArray;
}

TArray<FEdgePlacement> UTiledLevelAsset::GetAllEdgePlacements() const
{
	TArray<FEdgePlacement> OutArray;
	for (FTiledFloor F : TiledFloors)
	{
		OutArray.Append(F.EdgePlacements);
	}
	return OutArray;
}

TArray<FPointPlacement> UTiledLevelAsset::GetAllPointPlacements() const
{
	TArray<FPointPlacement> OutArray;
	for (FTiledFloor F : TiledFloors)
	{
		OutArray.Append(F.PointPlacements);
	}
	return OutArray;
}

TArray<FItemPlacement> UTiledLevelAsset::GetAllItemPlacements() const
{
	TArray<FItemPlacement> OutArray;
	for (FTiledFloor F : TiledFloors)
	{
		OutArray.Append(F.GetItemPlacements());
	}
	return OutArray;
}

int UTiledLevelAsset::GetNumOfAllPlacements() const
{
	return GetAllItemPlacements().Num();
}


TSet<UTiledLevelItem*> UTiledLevelAsset::GetUsedItems() const
{
	TSet<UTiledLevelItem*> UsedItemsSet;
	for (FTiledFloor F : TiledFloors)
	{
		for (FItemPlacement& P : F.GetItemPlacements())
		{
			UsedItemsSet.Add(P.GetItem());
		}
	}
	return UsedItemsSet;
}

/*
 * it takes me so long to get the static mesh inside spawned actors...
 * Must use actors that are actually spawned to get access to them...
 * Actor->BlueprintCreatedComponents only display BP components as it shows, while
 * Actor->GetComponents can actual get both... The reason why I can't get them is because I
 * tried it from "UObject" pointer set in UTiledItem rather the the actual spawned actor pointer 
 */
TSet<UStaticMesh*> UTiledLevelAsset::GetUsedStaticMeshSet() const
{
	TSet<UStaticMesh*> MeshesSet;
	for (UTiledLevelItem* UsedItem : GetUsedItems())
	{
		if (UsedItem->TiledMesh)
		{
			MeshesSet.Add(UsedItem->TiledMesh);
		}
		else if (UsedItem->TiledActor)
		{
			for (AActor* SpawnedActor : HostLevel->SpawnedTiledActors)
			{
				for (auto C : SpawnedActor->GetComponents())
				{
					if (UStaticMeshComponent* SMC = Cast<UStaticMeshComponent>(C))
					{
						if (SMC->GetStaticMesh())
						{
							MeshesSet.Add(SMC->GetStaticMesh());
						}
					}
				}
			}
		}
	}
	return MeshesSet;
}

void UTiledLevelAsset::EmptyRegionData(const TArray<FIntVector>& Points)
{
	if (Points.Num() == 0) return;
	TArray<FTilePlacement> TileToDelete;
	TArray<FTilePlacement> TargetTilePlacements;
	TargetTilePlacements.Append(GetAllBlockPlacements());
	TargetTilePlacements.Append(GetAllFloorPlacements());
	TArray<FEdgePlacement> WallToDelete;
	TArray<FEdgePlacement> TargetWallPlacements;
	TargetWallPlacements.Append(GetAllWallPlacements());
	TargetWallPlacements.Append(GetAllEdgePlacements());
	TArray<FPointPlacement> PointToDelete;
	TArray<FPointPlacement> TargetPointPlacements;
	TargetPointPlacements.Append(GetAllPillarPlacements());
	TargetPointPlacements.Append(GetAllPointPlacements());

	bool Found = false;
	for (FTilePlacement& Placement : TargetTilePlacements)
	{
		for (FIntVector& Position : Placement.GetOccupiedTilePositions())
		{
			if (Points.Contains(Position))
			{
				TileToDelete.Add(Placement);
				break;
			}
		}
	}

	TSet<FIntVector> Region = TSet<FIntVector>(Points);

	const TArray<FTiledLevelEdge> InnerEdges = FTiledLevelUtility::GetEdgesAroundArea(Region, false);
	for (FEdgePlacement& Placement : TargetWallPlacements)
	{
		FVector Extent = Placement.GetItem()->Extent;
		for (FTiledLevelEdge& E : Placement.GetOccupiedEdges(Extent))
		{
			if (InnerEdges.Contains(Placement.Edge))
			{
				WallToDelete.Add(Placement);
				break;
			}
		}
	}

	const TSet<FIntVector> InnerPoints = FTiledLevelUtility::GetPointsAroundArea(Region, false);

	for (FPointPlacement& Placement : TargetPointPlacements)
	{
		if (InnerPoints.Contains(Placement.GridPosition))
		{
			PointToDelete.Add(Placement);
		}
	}
	RemovePlacements(TileToDelete);
	RemovePlacements(WallToDelete);
	RemovePlacements(PointToDelete);
}

void UTiledLevelAsset::EmptyEdgeRegionData(const TArray<FTiledLevelEdge>& EdgeRegions)
{
	if (EdgeRegions.Num() == 0) return;
	TArray<FEdgePlacement> WallToDelete;
	TArray<FEdgePlacement> TargetWallPlacements;
	TargetWallPlacements.Append(GetAllWallPlacements());
	TargetWallPlacements.Append(GetAllEdgePlacements());

	for (FEdgePlacement& Placement : TargetWallPlacements)
	{
		FVector EdgeExtent = Placement.GetItem()->Extent;

		for (FTiledLevelEdge& Edge : Placement.GetOccupiedEdges(EdgeExtent))
		{
			if (EdgeRegions.Contains(Edge))
			{
				WallToDelete.Add(Placement);
				break;
			}
		}
	}
	RemovePlacements(WallToDelete);
}

void UTiledLevelAsset::GetAssetRegistryTags(TArray<FAssetRegistryTag>& AssetRegistryTags) const
{
	const FString TileSizeStr = FString::Printf(TEXT("%dx%dx%d"), FMath::RoundToInt(TileSizeX),
	                                            FMath::RoundToInt(TileSizeY), FMath::RoundToInt(TileSizeZ));
	const FString TileAmountStr = FString::Printf(TEXT("%dx%dx%d"), X_Num, Y_Num, TiledFloors.Num());
	AssetRegistryTags.Add(FAssetRegistryTag("Tile Size", TileSizeStr, FAssetRegistryTag::TT_Dimensional));
	AssetRegistryTags.Add(FAssetRegistryTag("Tile Amount", TileAmountStr, FAssetRegistryTag::TT_Dimensional));
	if (TiledFloors.IsValidIndex(0))
	{
		const FString FloorsInfoStr = FString::Printf(TEXT("%s to %s"), *TiledFloors[0].FloorName.ToString(),
		                                              *TiledFloors.Last().FloorName.ToString());
		AssetRegistryTags.Add(FAssetRegistryTag("Floors", FloorsInfoStr, FAssetRegistryTag::TT_Alphabetical));
	}
	AssetRegistryTags.Add(FAssetRegistryTag("Total Placements", FString::FromInt(GetNumOfAllPlacements()),
	                                        FAssetRegistryTag::TT_Numerical));

	UObject::GetAssetRegistryTags(AssetRegistryTags);
}

bool UTiledLevelAsset::UpdateAutoPaintData(const FIntVector& NewPos, const FName& NewItemName)
{
	if (NewItemName == FName("Empty")) return false;
	if (AutoPaintData.Contains(NewPos))
		if (AutoPaintData[NewPos] == NewItemName)
			return false;
	AutoPaintData.Add(NewPos, NewItemName);
	VersionNumber += 1;
	return true;
}

bool UTiledLevelAsset::RemoveAutoPaintDataAt(const FIntVector& PosToRemove)
{
	if (AutoPaintData.Contains(PosToRemove))
	{
		AutoPaintData.Remove(PosToRemove);
		VersionNumber += 1;
		return true;
	}
	return false;
}

TArray<FIntVector> UTiledLevelAsset::GetAllTilePositions()
{
	TArray<FIntVector> OutPositions;
	int StartFloor = FMath::Min(GetBottomFloor().FloorPosition, GetTopFloor().FloorPosition);
	int EndFloor = FMath::Max(GetBottomFloor().FloorPosition, GetTopFloor().FloorPosition);
	for (int x = 0; x < X_Num; x++)
	{
		for (int y = 0; y < Y_Num; y++)
		{
			for (int z = StartFloor; z <= EndFloor; z++)
			{
				OutPositions.Add(FIntVector(x, y, z));
			}
		}
	}
	// DEV_LOGF("len out pos: %d", OutPositions.Num())
	return OutPositions;
}

/*
 * TODO: V3.2 I should just follow the different placement space rule...
 * The stop on met logic is still very wrong?
 */

void UTiledLevelAsset::CollectAutoPaintPlacements(TArray<FAutoPaintPlacement_Tile>& OutTiles,
                                              TArray<FAutoPaintPlacement_Edge>& OutEdges,
                                              TArray<FAutoPaintPlacement_Point>& OutPoints)
{
	if (!IsValid(ActiveAutoPaintRule)) return;
	ActiveAutoPaintRule->MatchData.Empty();

	bool IsRuleMet = false;
	for (FIntVector TilePosition : GetAllTilePositions())
	{
		for (UAutoPaintItemRule* R : ActiveAutoPaintRule->GetEnabledRuleList())
		{
			IsRuleMet = false;
			for (EDuplicationType D: R->GetDuplicateCases())
			{
				if (UAutoPaintRule::IsRuleMet(R, D, TilePosition, AutoPaintData, {0, 0, GetBottomFloor().FloorPosition},
								 {X_Num, Y_Num, GetTopFloor().FloorPosition + 1}))
				{
					FTiledItemObj SpawnID = R->PickWhatToSpawn();
					// may become empty when item set is changed...
					if (SpawnID.TiledItemID.IsEmpty()) continue;
					FDuplicationMod DMod = UAutoPaintRule::GetDuplicationMod(D, FTiledLevelUtility::PlacedTypeToShape(R->PlacedType),
						R->AutoPaintItemSpawns[SpawnID].RotationTimes, R->PositionOffset);
					FAutoPaintMatchData Data;
					Data.DupType = D;
					Data.MetPos = TilePosition;
					Data.ToPaint = FGuid(SpawnID.TiledItemID);
					Data.PosOffset = DMod.PosOffset;
					Data.RotateTimes = DMod.RotationTimes;
					Data.SpawnAdj = &R->AutoPaintItemSpawns[SpawnID];
					Data.BaseRulePtr = R;
					ActiveAutoPaintRule->MatchData.Add(Data);
					IsRuleMet = true;
				}
			}
			if (IsRuleMet && R->bStopOnMet)
				break;
		}
	}

	TArray<FIntVector> HasSpawnedBlockPositions;
	TArray<FIntVector> HasSpawnedFloorPositions;
	TArray<FTiledLevelEdge> HasSpawnedWallPositions;
	TArray<FTiledLevelEdge> HasSpawnedEdgePositions;
	TArray<FIntVector> HasSpawnedPillarPositions;
	TArray<FIntVector> HasSpawnedPointPositions;

	// structure type checking...
	/*
	 * reversed way... only spawn if the space is enough for structure type items...
	 */
	auto CheckSpace = [&](EPlacedType PlacedType, FItemPlacement* P)
	{
		switch (PlacedType) {
		case EPlacedType::Block:
		{
			FTilePlacement* TP = static_cast<FTilePlacement*>(P);
			for (FIntVector Pos : TP->GetOccupiedTilePositions())
			{
				if (HasSpawnedBlockPositions.Contains(Pos))
				{
					return false;
				}
			}
			HasSpawnedBlockPositions.Append(TP->GetOccupiedTilePositions());
			return true;
		}
		case EPlacedType::Floor:
		{
			FTilePlacement* TP = static_cast<FTilePlacement*>(P);
			for (FIntVector Pos : TP->GetOccupiedTilePositions())
			{
				if (HasSpawnedFloorPositions.Contains(Pos))
				{
					return false;
				}
			}
			HasSpawnedFloorPositions.Append(TP->GetOccupiedTilePositions());
			return true;
		}
		case EPlacedType::Wall:
		{
			FEdgePlacement* EP = static_cast<FEdgePlacement*>(P);
			for (FTiledLevelEdge Pos : EP->GetOccupiedEdges(EP->GetItem()->Extent))
			{
				if (HasSpawnedWallPositions.Contains(Pos))
				{
					return false;
				}
			}
			HasSpawnedWallPositions.Append(EP->GetOccupiedEdges(EP->GetItem()->Extent));
			return true;
		}
		case EPlacedType::Edge:
		{
			FEdgePlacement* EP = static_cast<FEdgePlacement*>(P);
			for (FTiledLevelEdge Pos : EP->GetOccupiedEdges(EP->GetItem()->Extent))
			{
				if (HasSpawnedEdgePositions.Contains(Pos))
				{
					return false;
				}
			}
			HasSpawnedEdgePositions.Append(EP->GetOccupiedEdges(EP->GetItem()->Extent));
			return true;
		}
		case EPlacedType::Pillar:
		{
			FPointPlacement* PP = static_cast<FPointPlacement*>(P);
			if (HasSpawnedPillarPositions.Contains(PP->GridPosition))
			{
				return false;
			}
			HasSpawnedPillarPositions.Add(PP->GridPosition);
			return true;
		}
		case EPlacedType::Point:
		{
			FPointPlacement* PP = static_cast<FPointPlacement*>(P);
			if (HasSpawnedPointPositions.Contains(PP->GridPosition))
			{
				return false;
			}
			HasSpawnedPointPositions.Add(PP->GridPosition);
			return true;
		}
		}
		return false;	
	};
	
	for (auto D : ActiveAutoPaintRule->MatchData)
	{
		UTiledLevelItem* Item = ActiveItemSet->GetItem(D.ToPaint);
		if (!Item)
		{
			ERROR_LOGF("Can not get item from current item set (item id: %s)", *D.ToPaint.ToString())
			ERROR_LOG("Changing the valid source item set may fix it!")
			continue;
		}
		FTilePlacement NewTile;
		FEdgePlacement NewEdge;
		FPointPlacement NewPoint;
		FItemPlacement* NewPlacement = nullptr;
		const EPlacedShapeType PlacementShape = FTiledLevelUtility::PlacedTypeToShape(Item->PlacedType);
		bool HasEnoughSpace = true;
		switch (PlacementShape)
		{
		case Shape3D:
			NewPlacement = &NewTile;
			break;
		case Shape2D:
			NewPlacement = &NewEdge;
			break;
		case Shape1D:
			NewPlacement = &NewPoint;
			break;
		}
		NewPlacement->ItemSet = ActiveItemSet;
		NewPlacement->ItemID = Item->ItemID;

		switch (PlacementShape)
		{
		case Shape3D:
			NewTile.GridPosition = D.MetPos + D.PosOffset;
			NewTile.Extent = FIntVector(Item->Extent);
			if (Item->StructureType == ETLStructureType::Structure)
				HasEnoughSpace = CheckSpace(Item->PlacedType, &NewTile);
			if (HasEnoughSpace)
				OutTiles.Add(FAutoPaintPlacement_Tile(NewTile, D.RotateTimes, D.SpawnAdj->TransformAdjustment,
												 D.SpawnAdj->MirrorX, D.SpawnAdj->MirrorY, D.SpawnAdj->MirrorZ));
			break;
		case Shape2D:
			NewEdge.Edge = FTiledLevelEdge(
				D.MetPos + D.PosOffset,
				D.RotateTimes % 2 == 0? EEdgeType::Horizontal: EEdgeType::Vertical);
			if (Item->StructureType == ETLStructureType::Structure)
				HasEnoughSpace = CheckSpace(Item->PlacedType, &NewEdge);
			if (HasEnoughSpace)
				OutEdges.Add(FAutoPaintPlacement_Edge(NewEdge,D.RotateTimes, D.SpawnAdj->TransformAdjustment,
					D.SpawnAdj->MirrorX, D.SpawnAdj->MirrorY, D.SpawnAdj->MirrorZ));
			break;
		case Shape1D:
			NewPoint.GridPosition = D.MetPos + D.PosOffset;
			if (Item->StructureType == ETLStructureType::Structure)
				HasEnoughSpace = CheckSpace(Item->PlacedType, &NewPoint);
			if (HasEnoughSpace)
				OutPoints.Add(FAutoPaintPlacement_Point(NewPoint,D.RotateTimes, D.SpawnAdj->TransformAdjustment,
					D.SpawnAdj->MirrorX, D.SpawnAdj->MirrorY, D.SpawnAdj->MirrorZ));
			break;
		default: break;
		}
	}
	OutTiles.Sort();
	OutEdges.Sort();
	OutPoints.Sort();
}

FAutoPaintItem UTiledLevelAsset::GetAutoPaintItemRule(const FName& QueryName)
{
	if (ActiveAutoPaintRule && ActiveAutoPaintRule->Items.Contains(QueryName))
	{
		return *ActiveAutoPaintRule->Items.FindByKey(QueryName);
	}
	FAutoPaintItem ErrorRule;
	ErrorRule.PreviewColor = FLinearColor::Black;
	return ErrorRule;
}

void UTiledLevelAsset::ClearAutoPaintDataForFloor(int InFloorPosition)
{
	bool HasRemoveAny = false;
	for (auto It = AutoPaintData.CreateIterator(); It; ++It)
	{
		if (It.Key().Z == InFloorPosition)
		{
			It.RemoveCurrent();
			HasRemoveAny = true;
		}
	}
	if (HasRemoveAny)
	{
		RequestForUpdateAutoPaintPlacements.Execute();
	}
}

int UTiledLevelAsset::GetAppliedAutoItemCount()
{
	TArray<FName> Temp;
	for (auto [k, v] : AutoPaintData)
		Temp.AddUnique(v);
	return Temp.Num();
}

void UTiledLevelAsset::UpdateAutoPaintResults(const TArray<FTilePlacement>& InGenTiles,
                                              const TArray<FEdgePlacement>& InGenEdges,
                                              const TArray<FPointPlacement>& InGenPoints)
{
	for (FTiledFloor& Floor : TiledFloors)
	{
		Floor.AutoPaintGenTiles.Empty();
		Floor.AutoPaintGenEdges.Empty();
		Floor.AutoPaintGenPoints.Empty();
	}
	for (auto P : InGenTiles)
	{
		if (FTiledFloor* TargetFloor = TiledFloors.FindByKey(P.GridPosition.Z))
		{
			TargetFloor->AutoPaintGenTiles.Add(P);
		}
	}
	for (auto P : InGenEdges)
	{
		if (FTiledFloor* TargetFloor = TiledFloors.FindByKey(P.Edge.Z))
		{
			TargetFloor->AutoPaintGenEdges.Add(P);
		}
	}
	for (auto P : InGenPoints)
	{
		if (FTiledFloor* TargetFloor = TiledFloors.FindByKey(P.GridPosition.Z))
		{
			TargetFloor->AutoPaintGenPoints.Add(P);
		}
	}
}

void UTiledLevelAsset::ConvertAutoPlacementsToNormal()
{
	// empty required regions
	TArray<FIntVector> ToRemoveTilePositions;
	TArray<FTiledLevelEdge> ToRemoveEdges;
	TArray<FIntVector> ToRemovePointPositions;
	for (FTiledFloor& Floor : TiledFloors)
	{
		for (auto P : Floor.AutoPaintGenTiles)
			ToRemoveTilePositions.Add(P.GridPosition);
		Floor.BlockPlacements.RemoveAll([=](const FTilePlacement& P)
		{
			return ToRemoveTilePositions.Contains(P.GridPosition);
		});
		Floor.FloorPlacements.RemoveAll([=](const FTilePlacement& P)
		{
			return ToRemoveTilePositions.Contains(P.GridPosition);
		});
		for (auto P : Floor.AutoPaintGenEdges)
			ToRemoveEdges.Add(P.Edge);
		Floor.WallPlacements.RemoveAll([=](const FEdgePlacement& P)
		{
			return ToRemoveEdges.Contains(P.Edge);
		});
		Floor.EdgePlacements.RemoveAll([=](const FEdgePlacement& P)
		{
			return ToRemoveEdges.Contains(P.Edge);
		});
		for (auto P : Floor.AutoPaintGenPoints)
			ToRemovePointPositions.Add(P.GridPosition);
		Floor.PillarPlacements.RemoveAll([=](const FPointPlacement& P)
		{
			return ToRemovePointPositions.Contains(P.GridPosition);
		});
		Floor.PointPlacements.RemoveAll([=](const FPointPlacement& P)
		{
			return ToRemovePointPositions.Contains(P.GridPosition);
		});
		for (FTilePlacement P : Floor.AutoPaintGenTiles)
		{
			if (P.IsBlock())
				Floor.BlockPlacements.Add(P);
			else
				Floor.FloorPlacements.Add(P);
		}
		Floor.AutoPaintGenTiles.Empty();
		for (FEdgePlacement P : Floor.AutoPaintGenEdges)
		{
			if (P.IsWall())
				Floor.WallPlacements.Add(P);
			else
			{
				Floor.EdgePlacements.Add(P);
			}
		}
		Floor.AutoPaintGenEdges.Empty();
		for (FPointPlacement P : Floor.AutoPaintGenPoints)
		{
			if (P.IsPillar())
				Floor.PillarPlacements.Add(P);
			else
				Floor.PointPlacements.Add(P);
		}
		Floor.AutoPaintGenPoints.Empty();
	}
	AutoPaintData.Empty();
}

void UTiledLevelAsset::OnAutoPaintItemChanged(const FAutoPaintItem& Old, const FAutoPaintItem& New)
{
	for (auto& [k, v] : AutoPaintData)
	{
		if (v == Old.ItemName)
			v = New.ItemName;
	}
}

void UTiledLevelAsset::OnAutoPaintItemRemoved(const FName& ToRemove)
{
	TArray<FIntVector> KeysToRemove;
	for (auto& [k, v] : AutoPaintData)
	{
		if (v == ToRemove)
			KeysToRemove.Add(k);
	}
	for (auto k : KeysToRemove)
		AutoPaintData.Remove(k);
}


#undef LOCTEXT_NAMESPACE
