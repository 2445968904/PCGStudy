// Copyright 2023 Leartes Studios. All Rights Reserved.


#include "ObjectDistribution.h"
#include "ODDetailCustomization.h"
#include "PropertyEditorModule.h"


#define LOCTEXT_NAMESPACE "FObjectDistributionModule"

void FObjectDistributionModule::StartupModule()
{
	RegisterCustomDetailClasses();
}

void FObjectDistributionModule::ShutdownModule()
{
	UnregisterCustomDetailClasses();
}

void FObjectDistributionModule::RegisterCustomDetailClasses()
{
	auto& PropertyModule = FModuleManager::LoadModuleChecked< FPropertyEditorModule >("PropertyEditor");
	PropertyModule.RegisterCustomClassLayout("ODPresetObject",FOnGetDetailCustomizationInstance::CreateStatic(&FODPresetDetailCustomization::MakeInstance));
	PropertyModule.RegisterCustomClassLayout("ODDistributionBase",FOnGetDetailCustomizationInstance::CreateStatic(&FODDistributionDetailCustomization::MakeInstance));
	PropertyModule.NotifyCustomizationModuleChanged();
}

void FObjectDistributionModule::UnregisterCustomDetailClasses()
{
	auto& PropertyModule = FModuleManager::LoadModuleChecked< FPropertyEditorModule >("PropertyEditor");
	PropertyModule.UnregisterCustomClassLayout("ODPresetObject");
	PropertyModule.UnregisterCustomClassLayout("ODDistributionBase");
	PropertyModule.NotifyCustomizationModuleChanged();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FObjectDistributionModule, ObjectDistribution)