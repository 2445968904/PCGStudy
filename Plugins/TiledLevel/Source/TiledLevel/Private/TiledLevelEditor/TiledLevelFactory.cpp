﻿// Copyright 2022 PufStudio. All Rights Reserved.

#include "TiledLevelFactory.h"
#include "TiledItemSet.h"
#include "TiledLevelAsset.h"

UTiledLevelFactory::UTiledLevelFactory()
	: InitialItemSet(nullptr)
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UTiledLevelAsset::StaticClass();
}

UObject* UTiledLevelFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags,
	UObject* Context, FFeedbackContext* Warn)
{
	UTiledLevelAsset* NewTiledLevel = NewObject<UTiledLevelAsset>(InParent, InClass, InName, Flags | RF_Transactional);
	// Apply init settings
	{
		NewTiledLevel->AddNewFloor(0);
		if (InitialItemSet)
		{
			// Duplicate item set
			UTiledItemSet* DupItemSet = InitialItemSet->CloneItemSet(NewTiledLevel);
			NewTiledLevel->SetActiveItemSet(DupItemSet);
			NewTiledLevel->SetTileSize(DupItemSet->GetTileSize());
			NewTiledLevel->ConfirmTileSize();
		}
		if (InitialRule)
		{
			NewTiledLevel->SetActiveAutoPaintRule(InitialRule);
		}
	}
	return NewTiledLevel;
}
