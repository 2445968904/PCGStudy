// Copyright 2023 PufStudio. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "IDetailCustomization.h"

/**
 * 
 */
class TILEDLEVEL_API FAutoPaintRuleDetailCustomization : public IDetailCustomization
{
public:
    virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
    static TSharedRef<IDetailCustomization> MakeInstance();
	
private:
	static class UAutoPaintItemRule* CustomizedItemRule;

	IDetailLayoutBuilder* MyDetailLayout = nullptr;
	TSharedPtr<IPropertyHandle> Property_Extent;
	TSharedPtr<IPropertyHandle> Property_PlacedType;
	
	TOptional<int> GetExtentValue(EAxis::Type Axis) const;
	void OnExtentChanged(int InValue, EAxis::Type Axis) const;
	void OnExtentCommitted(int InValue, ETextCommit::Type CommitType, EAxis::Type Axis) const;

};
