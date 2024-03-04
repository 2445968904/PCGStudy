// Copyright 2023 PufStudio. All Rights Reserved.


#include "AutoPaintRuleDetailCustomization.h"

#include "AutoPaintRule.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "TiledItemDetailCustomization.h"
#include "TiledLevelUtility.h"
#include "Widgets/Input/SVectorInputBox.h"

#define LOCTEXT_NAMESPACE "AutoPaintRuleDetailCustomization"

UAutoPaintItemRule* FAutoPaintRuleDetailCustomization::CustomizedItemRule;

void FAutoPaintRuleDetailCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	DetailBuilder.HideCategory("Rule");
	DetailBuilder.HideCategory("StepSize");

	TArray<TWeakObjectPtr<UObject>> Temp;
	DetailBuilder.GetObjectsBeingCustomized(Temp);
	if (!Temp.IsValidIndex(0)) return;
	CustomizedItemRule = Cast<UAutoPaintItemRule>(Temp[0].Get());
	if (!CustomizedItemRule) return;

	MyDetailLayout = &DetailBuilder;
	IDetailCategoryBuilder& SpawnCategory = DetailBuilder.EditCategory("Spawn");
	Property_Extent = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UAutoPaintItemRule, Extent));
	Property_PlacedType = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UAutoPaintItemRule, PlacedType));
	if (FTiledLevelUtility::PlacedTypeToShape(CustomizedItemRule->PlacedType) != EPlacedShapeType::Shape2D)
		DetailBuilder.HideProperty(DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UAutoPaintItemRule, EdgeType)));

	// set properties display order
	SpawnCategory.AddProperty(Property_PlacedType);
	IDetailPropertyRow& ExtentRow = SpawnCategory.AddProperty(Property_Extent);

	TSharedRef<SWidget> X_Label = SNumericEntryBox<int>::BuildLabel(FText::FromString("X"), FLinearColor::White,
																	SNumericEntryBox<int>::RedLabelBackgroundColor);
	TSharedRef<SWidget> Y_Label = SNumericEntryBox<int>::BuildLabel(FText::FromString("Y"), FLinearColor::White,
																	SNumericEntryBox<int>::GreenLabelBackgroundColor);
	TSharedRef<SWidget> Z_Label = SNumericEntryBox<int>::BuildLabel(FText::FromString("Z"), FLinearColor::White,
																	SNumericEntryBox<int>::BlueLabelBackgroundColor);

	TSharedRef<SNumericEntryBox<int>> Extent_XInput = SNew(SNumericEntryBox<int>)
		.AllowSpin(true)
		.MinValue(0)
		.MaxValue(16)
		.MinSliderValue(1)
		.MaxSliderValue(16)
		.UndeterminedString(LOCTEXT("MultipleValues", "Multiple Values"))
		.Value(this, &FAutoPaintRuleDetailCustomization::GetExtentValue, EAxis::X)
		.OnValueChanged(this, &FAutoPaintRuleDetailCustomization::OnExtentChanged, EAxis::X)
		.OnValueCommitted(this, &FAutoPaintRuleDetailCustomization::OnExtentCommitted, EAxis::X)
		.Label()[X_Label]
		.LabelPadding(0);

	TSharedRef<SNumericEntryBox<int>> Extent_YInput = SNew(SNumericEntryBox<int>)
		.AllowSpin(true)
		.MinValue(0)
		.MaxValue(16)
		.MinSliderValue(1)
		.MaxSliderValue(16)
		.UndeterminedString(LOCTEXT("MultipleValues", "Multiple Values"))
		.Value(this, &FAutoPaintRuleDetailCustomization::GetExtentValue, EAxis::Y)
		.OnValueChanged(this, &FAutoPaintRuleDetailCustomization::OnExtentChanged, EAxis::Y)
		.OnValueCommitted(this, &FAutoPaintRuleDetailCustomization::OnExtentCommitted, EAxis::Y)
		.Label()[Y_Label]
		.LabelPadding(0);
	
	TSharedRef<SNumericEntryBox<int>> Extent_ZInput = SNew(SNumericEntryBox<int>)
		.AllowSpin(true)
		.MinValue(0)
		.MaxValue(16)
		.MinSliderValue(1)
		.MaxSliderValue(16)
		.UndeterminedString(LOCTEXT("MultipleValues", "Multiple Values"))
		.Value(this, &FAutoPaintRuleDetailCustomization::GetExtentValue, EAxis::Z)
		.OnValueChanged(this, &FAutoPaintRuleDetailCustomization::OnExtentChanged, EAxis::Z)
		.OnValueCommitted(this, &FAutoPaintRuleDetailCustomization::OnExtentCommitted, EAxis::Z)
		.Label()[Z_Label]
		.LabelPadding(0);
	
	TSharedRef<SHorizontalBox> ExtentInputs = SNew(SHorizontalBox);
	
	switch (CustomizedItemRule->PlacedType)
	{
	case EPlacedType::Block:
		ExtentInputs->AddSlot()
		.VAlign(VAlign_Center)
		.FillWidth(1.0f)
		.Padding(0.0f, 1.0f, 4.0f, 1.0f)
		[
			Extent_XInput
		];
		ExtentInputs->AddSlot()
		.VAlign(VAlign_Center)
		.FillWidth(1.0f)
		.Padding(0.0f, 1.0f, 4.0f, 1.0f)
		[
			Extent_YInput
		];
		ExtentInputs->AddSlot()
		.VAlign(VAlign_Center)
		.FillWidth(1.0f)
		.Padding(0.0f, 1.0f, 4.0f, 1.0f)
		[
			Extent_ZInput
		];
		break;
		
	case EPlacedType::Floor:
		ExtentInputs->AddSlot()
		.VAlign(VAlign_Center)
		.FillWidth(1.0f)
		.Padding(0.0f, 1.0f, 4.0f, 1.0f)
		[
			Extent_XInput
		];
		ExtentInputs->AddSlot()
		.VAlign(VAlign_Center)
		.FillWidth(1.0f)
		.Padding(0.0f, 1.0f, 4.0f, 1.0f)
		[
			Extent_YInput
		];
		break;
	case EPlacedType::Wall:
		ExtentInputs->AddSlot()
		.VAlign(VAlign_Center)
		.FillWidth(1.0f)
		.Padding(0.0f, 1.0f, 4.0f, 1.0f)
		[
			Extent_XInput
		];

		ExtentInputs->AddSlot()
		.VAlign(VAlign_Center)
		.FillWidth(1.0f)
		.Padding(0.0f, 1.0f, 4.0f, 1.0f)
		[
			Extent_ZInput
		];
		break;
	case EPlacedType::Pillar:
		ExtentInputs->AddSlot()
		.VAlign(VAlign_Center)
		.FillWidth(1.0f)
		.Padding(0.0f, 1.0f, 4.0f, 1.0f)
		[
			Extent_ZInput
		];
		break;
	case EPlacedType::Edge:
		ExtentInputs->AddSlot()
		.VAlign(VAlign_Center)
		.FillWidth(1.0f)
		.Padding(0.0f, 1.0f, 4.0f, 1.0f)
		[
			Extent_XInput
		];
		break;
	default: ;
	}
	
	ExtentRow.CustomWidget()
			 .NameContent()
		[
			Property_Extent->CreatePropertyNameWidget()
		]
		.ValueContent()
		.MinDesiredWidth(400.0f)
		.MinDesiredWidth(400.0f)
		[
			ExtentInputs
		];

	// register on property changed
	Property_PlacedType->SetOnPropertyValueChanged(FSimpleDelegate::CreateLambda([this]()
	{
		MyDetailLayout->ForceRefreshDetails();	
	}));
}

TSharedRef<IDetailCustomization> FAutoPaintRuleDetailCustomization::MakeInstance()
{
	return MakeShareable(new FAutoPaintRuleDetailCustomization());
}

TOptional<int> FAutoPaintRuleDetailCustomization::GetExtentValue(EAxis::Type Axis) const
{
	TOptional<int> Unset;
	if (!Property_Extent.IsValid()) return Unset;
	FIntVector Extent;
	Property_Extent->GetChildHandle(0)->GetValue(Extent.X);
	Property_Extent->GetChildHandle(1)->GetValue(Extent.Y);
	Property_Extent->GetChildHandle(2)->GetValue(Extent.Z);
	switch (Axis)
	{
	case EAxis::X:
		if (Extent.X < 1.0f || Extent.X > 16.0f)
		{
			return Unset;
		}
		return Extent.X;
	case EAxis::Y:
		if (Extent.Y < 1.0f || Extent.Y > 16.0f)
		{
			return Unset;
		}
		return Extent.Y;
	case EAxis::Z:
		if (Extent.Z < 1.0f || Extent.Z > 16.0f)
		{
			return Unset;
		}
		return Extent.Z;
	default: 
		return Unset;
	}
}

void FAutoPaintRuleDetailCustomization::OnExtentChanged(int InValue, EAxis::Type Axis) const
{
	if (InValue < 1) InValue = 1;
	if (InValue > 16) InValue = 16;
	uint8 PlacedTypeIndex;
	Property_PlacedType->GetValue(PlacedTypeIndex);
	FIntVector Extent;
	Property_Extent->GetChildHandle(0)->GetValue(Extent.X);
	Property_Extent->GetChildHandle(1)->GetValue(Extent.Y);
	Property_Extent->GetChildHandle(2)->GetValue(Extent.Z);
	switch (Axis)
	{
	case EAxis::X:
		Extent.X = InValue;
		switch (static_cast<EPlacedType>(PlacedTypeIndex)) {
			case EPlacedType::Wall:
				Extent.Y = InValue;
				break;
			case EPlacedType::Edge:
				Extent.Y = InValue;
				break;
			default: ;
		}
		Property_Extent->GetChildHandle(0)->SetValue(Extent.X);
		Property_Extent->GetChildHandle(1)->SetValue(Extent.Y);
		break;
	case EAxis::Y:
		Extent.Y = InValue;
		Property_Extent->GetChildHandle(1)->SetValue(Extent.Y);
		break;
	case EAxis::Z:
		Extent.Z = InValue;
		Property_Extent->GetChildHandle(2)->SetValue(Extent.Z);
		break;
	default: ;
	}
}

void FAutoPaintRuleDetailCustomization::OnExtentCommitted(int InValue, ETextCommit::Type CommitType,
	EAxis::Type Axis) const
{
	if (InValue < 1) InValue = 1;
	if (InValue > 16) InValue = 16;
	uint8 PlacedTypeIndex;
	Property_PlacedType->GetValue(PlacedTypeIndex);
	FIntVector Extent;
	Property_Extent->GetChildHandle(0)->GetValue(Extent.X);
	Property_Extent->GetChildHandle(1)->GetValue(Extent.Y);
	Property_Extent->GetChildHandle(2)->GetValue(Extent.Z);
	switch (Axis)
	{
	case EAxis::X:
		Extent.X = InValue;
		switch (static_cast<EPlacedType>(PlacedTypeIndex)) {
			case EPlacedType::Wall:
				Extent.Y = InValue;
				break;
			case EPlacedType::Edge:
				Extent.Y = InValue;
				break;
			default: ;
		}
		Property_Extent->GetChildHandle(0)->SetValue(Extent.X);
		Property_Extent->GetChildHandle(1)->SetValue(Extent.Y);
		break;
	case EAxis::Y:
		Extent.Y = InValue;
		Property_Extent->GetChildHandle(1)->SetValue(Extent.Y);
		break;
	case EAxis::Z:
		Extent.Z = InValue;
		Property_Extent->GetChildHandle(2)->SetValue(Extent.Z);
		break;
	default: ;
	}
}


#undef LOCTEXT_NAMESPACE
