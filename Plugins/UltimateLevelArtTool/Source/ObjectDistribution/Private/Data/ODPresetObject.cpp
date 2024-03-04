// Copyright 2023 Leartes Studios. All Rights Reserved.

#include "ODPresetObject.h"
#include "Editor.h"
#include "ODToolSubsystem.h"

class UODToolSubsystem;

void UODPresetObject::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	UObject::PostEditChangeProperty(PropertyChangedEvent);
}

void UODPresetObject::BeginDestroy()
{
	UObject::BeginDestroy();

	OnPresetUpdated.Unbind();
}

void UODPresetObject::SetupPresets()
{
	const auto ToolSubsystem = GEditor->GetEditorSubsystem<UODToolSubsystem>();
	if(!ToolSubsystem){return;}
		
	ToolSubsystem->UpdateMixerCheckListAndResetAllCheckStatus();

	OnPresetUpdated.ExecuteIfBound();
}

void UODPresetObject::PresetExpansionStateChanged(bool& InIsItOpen)
{
	OnPresetCategoryHidden.ExecuteIfBound(InIsItOpen);
}
