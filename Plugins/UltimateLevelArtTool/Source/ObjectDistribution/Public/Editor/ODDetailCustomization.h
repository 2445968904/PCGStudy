// Copyright 2023 Leartes Studios. All Rights Reserved.

#pragma once

#if WITH_EDITOR

#include "CoreMinimal.h"
#include "IDetailCustomization.h"

class UODDistributionBase;
struct FButtonStyle;
class SButton;
class STextBlock;

class FODPresetDetailCustomization : public IDetailCustomization
{
public:
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

	static TSharedRef< IDetailCustomization > MakeInstance();

	TSharedRef<class SWidget> GetPresetMixerMenuContent();

	TSharedPtr<class STextComboBox> PresetComboBox;
	TSharedPtr<class SComboButton> MixerComboBox;
	TSharedPtr<SButton> OpenTheMixerBtn;
	TSharedPtr<FButtonStyle> MixBtnStyle;




	void OnPresetLoaded();

	void CheckMixerBtnAvailability();
private:
	TArray<TSharedPtr<FString>> PresetNames;
};

class FODDistributionDetailCustomization : public IDetailCustomization
{
public:
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

	static TSharedRef< IDetailCustomization > MakeInstance();
};

#endif