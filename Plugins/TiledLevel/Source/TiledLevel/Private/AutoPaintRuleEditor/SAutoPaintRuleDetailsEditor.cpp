// Copyright 2023 PufStudio. All Rights Reserved.

#include "SAutoPaintRuleDetailsEditor.h"
#include "SlateOptMacros.h"
#include "AutoPaintRule.h"
#include "ISinglePropertyView.h"
#include "TiledItemSet.h"
#include "TiledLevelCommands.h"
#include "TiledLevelItem.h"
#include "TiledLevelEditorLog.h"
#include "TiledLevelUtility.h"
#include "TiledLevelEditorUtility.h"
#include "TiledLevelSettings.h"
#include "TiledLevelStyle.h"
#include "Dialogs/Dialogs.h"
#include "Framework/Commands/GenericCommands.h"
#include "Kismet/KismetMathLibrary.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"

#define LOCTEXT_NAMESPACE "AutoPaintRuleEditor"

FRuleDetailViewData::FRuleDetailViewData(UAutoPaintRule* InRule, int InGroupIdx, int InRuleIdx)
	: GroupIdx(InGroupIdx), RuleIdx(InRuleIdx), Rule(InRule)
{
}

UAutoPaintItemRule* FRuleDetailViewData::GetItemRule() const
{
	if (!EAutoPaintEditorSelectionState::Rule || RuleIdx == INDEX_NONE) return nullptr;
	if (Rule->Groups.IsValidIndex(GroupIdx) && Rule->Groups[GroupIdx].Rules.IsValidIndex(RuleIdx))
		return Rule->Groups[GroupIdx].Rules[RuleIdx];
	return nullptr;
}

FAutoPaintRuleGroup* FRuleDetailViewData::GetGroup() const
{
	if (!Rule) return nullptr;
	return &Rule->Groups[GroupIdx];
}

bool FRuleDetailViewData::IsGroup() const
{
	return RuleIdx == INDEX_NONE;
}

void FAutoPaintRuleDragDropOp::SetValidTarget(bool IsValidTarget)
{
	if (IsValidTarget)
	{
		CurrentHoverText = LOCTEXT("PlaceRuleHere", "Place rule here");
		CurrentIconBrush = FAppStyle::GetBrush("Graph.ConnectorFeedback.OK");
	}
	else
	{
		CurrentHoverText = LOCTEXT("CannotPlaceHere", "Cannot place rule here");
		CurrentIconBrush = FAppStyle::GetBrush("Graph.ConnectorFeedback.Error");
	}

}

void FAutoPaintRuleDragDropOp::OnDragged(const FDragDropEvent& DragDropEvent)
{
	MouseCursor = EMouseCursor::GrabHandClosed;
	FDecoratedDragDropOp::OnDragged(DragDropEvent);
}

TSharedRef<FAutoPaintRuleDragDropOp> FAutoPaintRuleDragDropOp::New(const TSharedPtr<FRuleDetailViewData>& InData)
{
	TSharedRef<FAutoPaintRuleDragDropOp> Operation = MakeShareable(new FAutoPaintRuleDragDropOp);
	Operation->Data = InData;
	Operation->SetValidTarget(false);
	Operation->SetupDefaults();
	Operation->Construct();
	return Operation;
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SAdjacencyRule::Construct(const FArguments& InArgs, const TSharedPtr<FRuleDetailViewData>& InData, TSharedPtr<SAutoPaintRuleDetailsEditor> InParentEditor)
{
	ParentEditor = InParentEditor;
	ButtonStyle.Normal.TintColor = FSlateColor(FLinearColor::White);
	Brush_SpawnedItemAreaHint = MakeShareable(new FSlateBrush());
	Brush_SpawnedItemAreaHint.Get()->DrawAs = ESlateBrushDrawType::RoundedBox;
	Brush_SpawnedItemAreaHint.Get()->TintColor = FLinearColor(1.0f, 0.5f, 1.0f, 1.0f); // tint color is useless here???
	Brush_SpawnedItemAreaHint.Get()->OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
	Brush_SpawnedItemAreaHint.Get()->OutlineSettings.Color = FLinearColor(1.0f, 1.0f, 0.0f, 1.0f);
	Brush_SpawnedItemAreaHint.Get()->OutlineSettings.CornerRadii = FVector4(3.f, 3.f, 3.f, 3.f);
	Brush_SpawnedItemAreaHint.Get()->OutlineSettings.Width = 4.f;
	Data = InData;
	
	// try a 3 x3 case
	SAssignNew(ButtonGrids, SUniformGridPanel)
	.SlotPadding(4.f);

	Update();
	
	ChildSlot
	[
		SNew(SBox)
		.Padding(4)
		[
			ButtonGrids.ToSharedRef()
		]
	];
}

FReply SAdjacencyRule::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (HoveredGridPoint.X < -10) return FReply::Unhandled();
	// Left mouse click on button is consumed by default, can not bind it here...
	if (MouseEvent.GetEffectingButton() == EKeys::MiddleMouseButton)
	{
		ToggleEmptyItem();
	}
	return SCompoundWidget::OnMouseButtonDown(MyGeometry, MouseEvent);
}

void SAdjacencyRule::Update()
{
	ViewFloor = Data->GetItemRule()->CurrentEditImpactFloor;
	ButtonGrids->ClearChildren();
	FIntVector AdjPointOffset = {0, 0, 0};
	switch (Data->GetItemRule()->ImpactSize)
	{
	case EImpactSize::One:
		NumGrids = 1;
		break;
	case EImpactSize::Three:
		AdjPointOffset = {-1, -1, 0};
		NumGrids = 9;
		break;
	case EImpactSize::Five:
		AdjPointOffset = {-2, -2, 0};
		NumGrids = 25;
		break;
	default: ;
	}

	FIntVector Extent = Data->GetItemRule()->Extent;
	FIntVector Offset = Data->GetItemRule()->PositionOffset;
	
	ShouldShowSpawnedAreaHint = false;
	for (int i = Offset.Z; i < Offset.Z + Extent.Z; i++)
	{
		if (ViewFloor == i)
		{
			ShouldShowSpawnedAreaHint = true;
			break;
		}
	}
	
	const int RowSize = static_cast<int>(FMath::Pow(NumGrids, 0.5));
	
	for (int i = 0; i < NumGrids; i++)
	{
		const FIntVector MyPos = FIntVector(i % RowSize, i / RowSize, ViewFloor) + AdjPointOffset;
		ButtonGrids->AddSlot(i % RowSize, i / RowSize)
		[
			SNew(SBox)
			.WidthOverride(20.f)
			.HeightOverride(20.f)
			[
				SNew(SButton)
				.ButtonStyle(&ButtonStyle)
				.ButtonColorAndOpacity(this, &SAdjacencyRule::GetAdjRuleColor, MyPos)
				.OnHovered(this, &SAdjacencyRule::OnGridButtonHovered, MyPos)
				.OnUnhovered(this, &SAdjacencyRule::OnGridButtonUnHovered)
				[
					SNew(SImage)
					.Image(this, &SAdjacencyRule::GetButtonImage, MyPos)
					.ToolTipText(this, &SAdjacencyRule::GetButtonTooltip, MyPos)
					.ColorAndOpacity(this, &SAdjacencyRule::GetButtonImageColor, MyPos)
					// MakeGridButtonContent(MyPos)
				]
				.OnClicked(this, &SAdjacencyRule::TogglePositiveItem)
			]
		];
	}
}

int32 SAdjacencyRule::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	if (!ShouldShowSpawnedAreaHint || Data->GetItemRule()->AutoPaintItemSpawns.IsEmpty())
		return SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	FVector2D Extent = FVector2D(Data->GetItemRule()->Extent.X, Data->GetItemRule()->Extent.Y);
	FVector2D Offset = FVector2D(Data->GetItemRule()->PositionOffset.X, Data->GetItemRule()->PositionOffset.Y);
	static FVector2D BtnSizeWithPadding = {28.f, 28.f};
	static FVector2D BorderPadding = {4.f, 4.f};
	FVector2D Center;
	switch (Data->GetItemRule()->ImpactSize)
	{
	case EImpactSize::One:
		Center = FVector2D(0.f);
		break;
	case EImpactSize::Three:
		Center = FVector2D(1.f);
		break;
	case EImpactSize::Five:
		Center = FVector2D(2.f);
		break;
	default: ;
	}
	
	EPlacedShapeType Shape = FTiledLevelUtility::PlacedTypeToShape(Data->GetItemRule()->PlacedType);
	FVector2D TranslationMod = (Offset + Center) * BtnSizeWithPadding + BorderPadding;
	FPaintGeometry PaintGeom;
	switch (Shape) {
	case Shape3D:
	{
		PaintGeom = AllottedGeometry.ToPaintGeometry(BtnSizeWithPadding * Extent, FSlateLayoutTransform(1.0, TranslationMod));
		break;
	}
	case Shape2D:
	{
		FVector2D ShapeSize;
		if (Data->GetItemRule()->EdgeType == EEdgeType::Horizontal)
		{
			ShapeSize = FVector2D(28, 8);
			ShapeSize.X += 28.f * (Extent.X - 1);
			TranslationMod.Y -= 4;
		}
		else
		{
			ShapeSize = FVector2D(8, 28);
			ShapeSize.Y += 28.f * (Extent.X - 1);
			TranslationMod.X -= 4;
		}
		PaintGeom = AllottedGeometry.ToPaintGeometry(ShapeSize, FSlateLayoutTransform(1.0, TranslationMod));
		break;
	}
	case Shape1D:
	{
		TranslationMod -= FVector2D(4, 4);
		PaintGeom = AllottedGeometry.ToPaintGeometry(FVector2D(8, 8), FSlateLayoutTransform(1.0, TranslationMod));
		break;
	}
	default: ;
	}
	FSlateDrawElement::MakeBox(OutDrawElements, LayerId, PaintGeom, Brush_SpawnedItemAreaHint.Get() , ESlateDrawEffect::None, FLinearColor(1,1,1,0));
	return SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
}

FSlateColor SAdjacencyRule::GetAdjRuleColor(FIntVector AdjPoint) const
{
	if (!Data->GetItemRule()) return FLinearColor(0.2, 0.2, 0.2, 1);
	if (FAdjacencyRule* AdjRule = Data->GetItemRule()->AdjacencyRules.Find(AdjPoint))
	{
		if (AdjRule->AutoPaintItemName == FName("Empty") || AdjRule->AutoPaintItemName == FName("Any"))
		{
			return FLinearColor::White;
		}
		if (FAutoPaintItem* TargetItem  = Data->Rule->Items.FindByKey(AdjRule->AutoPaintItemName))
			return TargetItem->PreviewColor;
	}
	return FLinearColor(0.2, 0.2, 0.2, 1);
}

const FSlateBrush* SAdjacencyRule::GetButtonImage(FIntVector AdjPoint) const
{
	if (!Data->GetItemRule()) return nullptr;
	if (FAdjacencyRule* AdjRule = Data->GetItemRule()->AdjacencyRules.Find(AdjPoint))
	{
		if (AdjRule->bNegate)
			return FAppStyle::GetBrush("Icons.X");
	}
	return nullptr;
}

FText SAdjacencyRule::GetButtonTooltip(FIntVector AdjPoint) const
{
	if (!Data->GetItemRule()) return FText::GetEmpty();
	if (FAdjacencyRule* AdjRule = Data->GetItemRule()->AdjacencyRules.Find(AdjPoint))
	{
		if (AdjRule->AutoPaintItemName == FName("Empty") && AdjRule->bNegate)
			return FText::FromString("Anything! (except empty)");
		if (AdjRule->AutoPaintItemName == FName("Empty"))
			return FText::FromString("Empty!");
		if (AdjRule->bNegate)
			return FText::FromString("Anything except this (includes empty)");
	}
	return FText::GetEmpty();
}

FSlateColor SAdjacencyRule::GetButtonImageColor(FIntVector AdjPoint) const
{
	if (!Data->GetItemRule()) return FLinearColor::Black;
	if (FAdjacencyRule* AdjRule = Data->GetItemRule()->AdjacencyRules.Find(AdjPoint))
	{
		if (AdjRule->bNegate)
		{
			if (FAutoPaintItem* AutoItem = Data->Rule->Items.FindByKey(AdjRule->AutoPaintItemName))
				return UKismetMathLibrary::LinearColor_GetLuminance(AutoItem->PreviewColor) > 0.5f ?FLinearColor::Black : FLinearColor::White;
		}
	}
	return FLinearColor::Black;
}

void SAdjacencyRule::OnGridButtonHovered(FIntVector AdjPoint)
{
	HoveredGridPoint = AdjPoint;
}

void SAdjacencyRule::OnGridButtonUnHovered()
{
	HoveredGridPoint = FIntVector(-100, -100, -100);
}

FReply SAdjacencyRule::TogglePositiveItem()
{
	if (FSlateApplication::Get().GetModifierKeys().IsShiftDown())
	{
		ToggleNegativeItem();
		return FReply::Handled();
	}
	FScopedTransaction Transaction(LOCTEXT("AutoPaintRuleEditor_ModyfyAdjacencyRule", "Modify adjacency rule"));
	Data->GetItemRule()->Modify();
	Data->Rule->Modify();
	if (FAdjacencyRule* AdjRule = Data->GetItemRule()->AdjacencyRules.Find(HoveredGridPoint))
	{
		if (AdjRule->AutoPaintItemName == Data->Rule->ActiveItemName && !AdjRule->bNegate)
		{
			Data->GetItemRule()->AdjacencyRules.Remove(HoveredGridPoint);
		}
		else
		{
			AdjRule->AutoPaintItemName = Data->Rule->ActiveItemName;
			AdjRule->bNegate = false;
		}
	}
	else
	{
		Data->GetItemRule()->AdjacencyRules.Add(HoveredGridPoint, FAdjacencyRule(Data->Rule->ActiveItemName, false));
	}
	Data->Rule->RefreshSpawnedInstance_Delegate.Execute();
	if (ParentEditor->GetSelectedAutoPaintRule() == Data->GetItemRule())
	    Data->Rule->RequestForUpdateMatchHint.Execute(Data->GetItemRule());
	return FReply::Handled();
}

void SAdjacencyRule::ToggleNegativeItem()
{
	FScopedTransaction Transaction(LOCTEXT("AutoPaintRuleEditor_ModyfyAdjacencyRule", "Modify adjacency rule"));
	Data->GetItemRule()->Modify();
	Data->Rule->Modify();
	if (FAdjacencyRule* AdjRule = Data->GetItemRule()->AdjacencyRules.Find(HoveredGridPoint))
	{
		if (AdjRule->AutoPaintItemName == Data->Rule->ActiveItemName && AdjRule->bNegate)
		{
			Data->GetItemRule()->AdjacencyRules.Remove(HoveredGridPoint);
		}
		else
		{
			AdjRule->AutoPaintItemName = Data->Rule->ActiveItemName;
			AdjRule->bNegate = true;
		}
	}
	else
	{
		Data->GetItemRule()->AdjacencyRules.Add(HoveredGridPoint, FAdjacencyRule(Data->Rule->ActiveItemName, true));
	}
	Data->Rule->RefreshSpawnedInstance_Delegate.Execute();
	if (ParentEditor->GetSelectedAutoPaintRule() == Data->GetItemRule())
	    Data->Rule->RequestForUpdateMatchHint.Execute(Data->GetItemRule());
}

void SAdjacencyRule::ToggleEmptyItem()
{
	FScopedTransaction Transaction(LOCTEXT("AutoPaintRuleEditor_ModyfyAdjacencyRule", "Modify adjacency rule"));
	Data->GetItemRule()->Modify();
	Data->Rule->Modify();
	if (FAdjacencyRule* AdjRule = Data->GetItemRule()->AdjacencyRules.Find(HoveredGridPoint))
	{
		if (AdjRule->AutoPaintItemName == FName("Empty"))
		{
			Data->GetItemRule()->AdjacencyRules.Remove(HoveredGridPoint);
		}
		else
		{
			AdjRule->AutoPaintItemName = FName("Empty");
			AdjRule->bNegate = false;
		}
	}
	else
	{
		Data->GetItemRule()->AdjacencyRules.Add(HoveredGridPoint, FAdjacencyRule(FName("Empty"), false));
	}
	Data->Rule->RefreshSpawnedInstance_Delegate.Execute();
	if (ParentEditor->GetSelectedAutoPaintRule() == Data->GetItemRule())
		Data->Rule->RequestForUpdateMatchHint.Execute(Data->GetItemRule());
}

void SImpactFloorSelect::Construct(const FArguments& InArgs, const TSharedPtr<FRuleDetailViewData>& InData,
	const TSharedPtr<SAdjacencyRule>& InControllingWidget )
{
	Data = InData;
	ControllingWidget = InControllingWidget;

	Content = SNew(SVerticalBox);

	Update();
	
	ChildSlot
	[
		Content.ToSharedRef()
	];
}

void SImpactFloorSelect::Update()
{
	Content->ClearChildren();
	if (!Data->GetItemRule()->bIncludeZ) return;
	if (Data->GetItemRule()->ImpactSize == EImpactSize::One) return;

	// move up button
	Content->AddSlot()
	[
		SNew(SBox)
		.WidthOverride(36.f)
		.HeightOverride(18.f)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "ToggleButton")
			.ContentPadding(FMargin(-5.f))
			.Content()
			[
				SNew(SImage)
				.Image(FAppStyle::GetBrush("Icons.ChevronUp"))
			]
			.OnClicked_Lambda([=]()
			{
				if (Data->GetItemRule()->CurrentEditImpactFloor == 2 && Data->GetItemRule()->ImpactSize == EImpactSize::Five)
					return FReply::Unhandled();
				if (Data->GetItemRule()->CurrentEditImpactFloor == 1 && Data->GetItemRule()->ImpactSize == EImpactSize::Three)
					return FReply::Unhandled();
				Data->GetItemRule()->CurrentEditImpactFloor += 1;
				ControllingWidget->Update();
				UpdateFloorsWidget();
				return FReply::Handled();
			})
		]
	];

	Content->AddSlot()
	[
		SAssignNew(FloorsWidget, SVerticalBox)
	];

	UpdateFloorsWidget();

	// add move down button
	Content->AddSlot()
	[
		SNew(SBox)
		.WidthOverride(36.f)
		.HeightOverride(18.f)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "ToggleButton")
			.ContentPadding(FMargin(-5.f))
			.Content()
			[
				SNew(SImage)
				.Image(FAppStyle::GetBrush("Icons.ChevronDown"))
			]
			.OnClicked_Lambda([=]()
			{
				if (Data->GetItemRule()->CurrentEditImpactFloor == -2 && Data->GetItemRule()->ImpactSize == EImpactSize::Five)
					return FReply::Unhandled();
				if (Data->GetItemRule()->CurrentEditImpactFloor == -1 && Data->GetItemRule()->ImpactSize == EImpactSize::Three)
					return FReply::Unhandled();
				Data->GetItemRule()->CurrentEditImpactFloor -= 1;
				ControllingWidget->Update();
				UpdateFloorsWidget();
				return FReply::Handled();
			})
		]
	];
	
}

void SImpactFloorSelect::UpdateFloorsWidget()
{
	FloorsWidget->ClearChildren();
	const int NumFloors = Data->GetItemRule()->ImpactSize == EImpactSize::Three ? 3 : 5;
	// add floor icons
	const int mod = Data->GetItemRule()->ImpactSize == EImpactSize::Three ? 1 : 2;
	for (int i = 0; i < NumFloors; i++)
	{
		FSlateRenderTransform RT;
		if (i - mod == Data->GetItemRule()->CurrentEditImpactFloor * -1) RT = FSlateRenderTransform(TScale2<float>(1.5, 2.0));
		FloorsWidget->AddSlot()
		[
			SNew(SBox)
			.WidthOverride(14.f)
			.HeightOverride(8.f)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SImage)
				.Image(FAppStyle::GetBrush("Icons.Minus"))
				.RenderTransform(RT)
				.RenderTransformPivot(FVector2D(0.5, 0.5))
			]
		];
	}
}

void SSpawnRuleWidget::Construct(const FArguments& InArgs, const TSharedPtr<FRuleDetailViewData>& InData,
	const TSharedPtr<SAdjacencyRule>& InAdjacencyRuleWidget)
{
	Data = InData;
	AdjacencyRuleWidget = InAdjacencyRuleWidget;
	ThumbnailPool = MakeShareable(new FAssetThumbnailPool(1024));
	Brush.DrawAs = ESlateBrushDrawType::Image;
	Brush.TintColor = FLinearColor(0.01f, 0.01f, 0.01f, 1.f);

	TooltipContent = SNew(SVerticalBox);
	ToSpawnItemsPreview = SNew(SUniformGridPanel)
		.MinDesiredSlotWidth(64.f)
		.MinDesiredSlotHeight(64.f)
		.SlotPadding(FMargin(0, 0, 0, 1));
							
	UpdateToSpawnItemsPreview();

	ChildSlot
	[
		SNew(SBox)
		.WidthOverride(132)
		.HeightOverride(84)
		[
			SNew(SButton)
			.ContentPadding(FMargin(2.f))
			.ToolTip(
				SNew(SToolTip)
				.TextMargin(1)
				.BorderImage(FAppStyle::GetBrush("ContentBrowser.TileViewTooltip.ToolTipBorder"))
				[
					SNew(SBorder)
					.Padding(3.f)
					.BorderImage(FAppStyle::GetBrush("ContentBrowser.TileViewTooltip.NonContentBorder"))
					[TooltipContent.ToSharedRef()]
				]
			)
			.OnClicked(this, &SSpawnRuleWidget::OnEditSpawnDetailClicked)
			.Content()
			[
				SNew(SScrollBox)
				.Orientation(Orient_Horizontal)
				.ScrollBarThickness(FVector2D(2.f, 2.f))
				+ SScrollBox::Slot()
				[
					ToSpawnItemsPreview.ToSharedRef()
				]
			]
		]
	];
}

void SSpawnRuleWidget::UpdateTooltipContent() const
{
	UAutoPaintItemRule* ItemRule = Data->GetItemRule();
	if (ItemRule->AutoPaintItemSpawns.IsEmpty())
	{
		TooltipContent->ClearChildren();
		TooltipContent->AddSlot()
		[
			SNew(STextBlock)
			.Text(FText::FromString("click to edit what to spawn!"))
		];
		return;
	}
	TSharedPtr<SVerticalBox> HeaderContent = SNew(SVerticalBox)
	+SVerticalBox::Slot()
	[
		SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("PlacedTypeHeading", "Placed Type: "))
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
		]
		+SHorizontalBox::Slot()
		[
			SNew(STextBlock)
			.Text_Lambda([this]()
			{
				switch (Data->GetItemRule()->PlacedType) {
					case EPlacedType::Block:  return FText::FromString("Block");
					case EPlacedType::Floor:  return FText::FromString("Floor");
					case EPlacedType::Wall:   return FText::FromString("Wall");
					case EPlacedType::Edge:   return FText::FromString("Edge");
					case EPlacedType::Pillar: return FText::FromString("Pillar");
					case EPlacedType::Point:  return FText::FromString("Point");
					default: return FText::GetEmpty();
				}
			})
		]
	]
	+SVerticalBox::Slot()
	[
		SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("StructureTypeHeading", "Structure Type: "))
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
		]
		+SHorizontalBox::Slot()
		[
			SNew(STextBlock)
			.Text_Lambda([this](){
				return Data->GetItemRule()->StructureType == ETLStructureType::Structure? FText::FromString("Structure") : FText::FromString("Prop");}
				)
		]
	]
	+SVerticalBox::Slot()
	[
		SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("ExtentHeading", "Extent: "))
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
		]
		+SHorizontalBox::Slot()
		[
			SNew(STextBlock)
			.Text_Lambda([this](){
				const UAutoPaintItemRule* Rule = Data->GetItemRule();
				return FText::FromString(FString::Printf(TEXT("%d x %d x %d"), Rule->Extent.X, Rule->Extent.Y, Rule->Extent.Z));}
				)
		]
	]
	+SVerticalBox::Slot()
	[
		SNew(SHorizontalBox)
		.Visibility_Lambda([=](){ return ItemRule->PositionOffset == FIntVector(0)? EVisibility::Collapsed: EVisibility::Visible;})
		+SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("PositionOffsetHeading", "PositionOffset: "))
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
		]
		+SHorizontalBox::Slot()
		[
			SNew(STextBlock)
			.Text_Lambda([this](){
				const UAutoPaintItemRule* Rule = Data->GetItemRule();
				return FText::FromString(FString::Printf(TEXT("%d x %d x %d"), Rule->PositionOffset.X, Rule->PositionOffset.Y, Rule->PositionOffset.Z));}
				)
		]
	]
	+SVerticalBox::Slot()
	[
		SNew(SHorizontalBox)
		.Visibility_Lambda([=](){return ItemRule->StepSize == FIntVector(1)? EVisibility::Collapsed: EVisibility::Visible;})
		+SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("StepSizeHeading", "Step Size: "))
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
		]
		+SHorizontalBox::Slot()
		[
			SNew(STextBlock)
			.Text_Lambda([this](){
				const UAutoPaintItemRule* Rule = Data->GetItemRule();
				return FText::FromString(FString::Printf(TEXT("%d x %d x %d"), Rule->StepSize.X, Rule->StepSize.Y, Rule->StepSize.Z));}
				)
		]
	]
	+SVerticalBox::Slot()
	[
		SNew(SHorizontalBox)
		.Visibility_Lambda([=](){return ItemRule->StepOffset == FIntVector(0)? EVisibility::Collapsed: EVisibility::Visible;})
		+SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("StepOffsetHeading", "Step Offset: "))
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
		]
		+SHorizontalBox::Slot()
		[
			SNew(STextBlock)
			.Text_Lambda([this](){
				const UAutoPaintItemRule* Rule = Data->GetItemRule();
				return FText::FromString(FString::Printf(TEXT("%d x %d x %d"), Rule->StepOffset.X, Rule->StepOffset.Y, Rule->StepOffset.Z));}
				)
		]
	];
	
	TSharedPtr<SUniformGridPanel> DetailContent =
	SNew(SUniformGridPanel)
	+ SUniformGridPanel::Slot(1, 0)
	.HAlign(HAlign_Fill)
	.VAlign(VAlign_Fill)
	[
		 SNew(STextBlock)
		 .Text(FText::FromString("Rand Coef."))
		 .Justification(ETextJustify::Right)
		 .ColorAndOpacity(FSlateColor::UseSubduedForeground())
	]
	+SUniformGridPanel::Slot(2, 0)
	.VAlign(VAlign_Fill)
	.HAlign(HAlign_Fill)
	[
		 SNew(STextBlock)
		 .Text(FText::FromString("Rotation"))
		 .Justification(ETextJustify::Right)
		 .ColorAndOpacity(FSlateColor::UseSubduedForeground())
	]
	+SUniformGridPanel::Slot(3, 0)
	.VAlign(VAlign_Fill)
	.HAlign(HAlign_Fill)
	[
		 SNew(STextBlock)
		 .Text(FText::FromString("Mirror"))
		 .Justification(ETextJustify::Right)
		 .ColorAndOpacity(FSlateColor::UseSubduedForeground())
	];

	int n = 1;
	for (auto it = ItemRule->AutoPaintItemSpawns.CreateConstIterator(); it; ++it)
	{
		FText ItemName = FText::GetEmpty();
		if (const UTiledLevelItem* Item = Data->Rule->GetItemSet()->GetItem(FGuid(it.Key().TiledItemID)))
			ItemName = FText::FromString(Item->GetItemName());
		DetailContent->AddSlot(0, n)
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Fill)
		[
			SNew(SBox)
			.MaxDesiredWidth(96.f)
			[
			  SNew(STextBlock)
			  .Text(ItemName)
			  .Justification(ETextJustify::Left)
			  .Clipping(EWidgetClipping::ClipToBounds)
			  .OverflowPolicy(ETextOverflowPolicy::Ellipsis)
			]
		];
		DetailContent->AddSlot(1, n)
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Fill)
		[
			  SNew(STextBlock)
			  .Text_Lambda([=]()
			  {
			  	  if (ItemRule->AutoPaintItemSpawns.IsEmpty())
			  	  	return FText::GetEmpty();
				  return FText::FromString(FString::Printf(TEXT("%.2f"), it.Value().RandomCoefficient));
			  })
			  .Justification(ETextJustify::Right)
		];
		DetailContent->AddSlot(2, n)
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Fill)
		[
			  SNew(STextBlock)
			  .Text_Lambda([=]()
			  {
			  	  if (ItemRule->AutoPaintItemSpawns.IsEmpty())
			  	  	return FText::GetEmpty();
				  return FText::FromString(FString::FromInt(it.Value().RotationTimes));
			  })
			  .Justification(ETextJustify::Right)
		];
		 DetailContent->AddSlot(3, n)
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Fill)
		[
			  SNew(STextBlock)
			  .Text_Lambda([=]()
			  {
			  	  if (ItemRule->AutoPaintItemSpawns.IsEmpty())
			  	  	return FText::GetEmpty();
				  if (!it.Value().MirrorX && !it.Value().MirrorY && !it.Value().MirrorZ)
					  return FText::FromString("-");
				  FString Out = TEXT("");
				  if (it.Value().MirrorX) Out += TEXT("X");
				  if (it.Value().MirrorY) Out += TEXT("Y");
				  if (it.Value().MirrorZ) Out += TEXT("Z");
				  if (Out.Len() == 2)
					  Out.InsertAt(1, TEXT(", "));
				  else if (Out.Len() == 3)
				  {
					  Out.InsertAt(1, TEXT(", "));
					  Out.InsertAt(4, TEXT(", "));
				  }
				  return FText::FromString(Out);
			  })
			  .Justification(ETextJustify::Right)
		];
		n++;
	}
	TooltipContent->ClearChildren();
	TooltipContent->AddSlot()
	.AutoHeight()
	[
	    SNew(SBorder)
	    .Padding(FMargin(6.f))
	    .HAlign(HAlign_Left)
	    .BorderImage(FAppStyle::GetBrush("ContentBrowser.TileViewTooltip.ContentBorder"))
		[
			HeaderContent.ToSharedRef()
		]
	];
	TooltipContent->AddSlot()
	.AutoHeight()
	.Padding(FMargin(0.f, 3.f, 0.f, 0.f))
	[
		SNew(SBorder)
		.Padding(6.f)
		.BorderImage(FAppStyle::GetBrush("ContentBrowser.TileViewTooltip.ContentBorder"))
		[
			DetailContent.ToSharedRef()
		]
	];
}

void SSpawnRuleWidget::UpdateToSpawnItemsPreview() const
{
	ToSpawnItemsPreview->ClearChildren();
	if (Data->GetItemRule()->AutoPaintItemSpawns.IsEmpty())
	{
		ToSpawnItemsPreview->AddSlot(0, 0)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("AddSpawnItemHint_Label", "click to edit what to spawn!"))
		];
		TooltipContent->ClearChildren();
		TooltipContent->AddSlot()
		[
			SNew(STextBlock)
			.Text(FText::FromString("click to edit what to spawn!"))
		];
		return;
	}
	int i = 0;
	for (auto [ItemObj, adj] : Data->GetItemRule()->AutoPaintItemSpawns)
	{
		UTiledLevelItem* Item = Data->Rule->GetItemSet()->GetItem(FGuid(ItemObj.TiledItemID));
		FAssetData AssetData = FAssetData(Item);
		TSharedPtr<FAssetThumbnail> Thumbnail = MakeShareable(
			new FAssetThumbnail(AssetData, 64, 64, ThumbnailPool));
		FAssetThumbnailConfig Config;
		FTiledLevelEditorUtility::ConfigThumbnailAssetColor(Config, Item);
		ToSpawnItemsPreview->AddSlot(i, 0)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			Thumbnail->MakeThumbnailWidget(Config)
		];
		i++;
	}
	UpdateTooltipContent();
}

FReply SSpawnRuleWidget::OnEditSpawnDetailClicked()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::Get().GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FDetailsViewArgs DetailsViewArgs;
	{
		DetailsViewArgs.bAllowSearch = false;
		DetailsViewArgs.bCustomFilterAreaLocation = true;
		DetailsViewArgs.bCustomNameAreaLocation = true;
		DetailsViewArgs.bHideSelectionTip = true;
		DetailsViewArgs.bLockable = false;
		DetailsViewArgs.bSearchInitialKeyFocus = true;
		DetailsViewArgs.bUpdatesFromSelection = false;
		DetailsViewArgs.bShowOptions = false;
		DetailsViewArgs.bShowModifiedPropertiesOption = false;
		DetailsViewArgs.bShowScrollBar = false;
	}

	TSharedPtr<IDetailsView> DetailsView = PropertyModule.CreateDetailView(DetailsViewArgs);
	DetailsView->SetObject(Data->GetItemRule());
	
	SGenericDialogWidget::FArguments DialogArgs;
	DialogArgs.ScrollBoxMaxHeight(600.f);
	DialogArgs.OnOkPressed_Lambda([this]()
	{
		UpdateToSpawnItemsPreview();
		AdjacencyRuleWidget->Update();
		
	});
	SGenericDialogWidget::OpenDialog(
		LOCTEXT("EditSpawnRuleTitle", "Edit Spawn Rule"),
		SNew(SBox)
			.MinDesiredWidth(720)
			[
				DetailsView.ToSharedRef()
			],
		DialogArgs,
		false);
	return FReply::Handled();
}


// SRuleGroup
void SRuleRow::Construct(const FArguments& InArgs, TSharedPtr<FRuleDetailViewData> InItem, TSharedPtr<SAutoPaintRuleDetailsEditor> InParentEditor,
	const TSharedRef<STableViewBase>& InOwnerTableView)
{
	Data = InItem;
	ParentEditor = InParentEditor;
	ButtonStyle.Normal.TintColor = FSlateColor(FLinearColor(0.05f, 0.05f, 0.05f, 1.f));
	ButtonStyle.Hovered.TintColor = FSlateColor(FLinearColor(0.1f, 0.1f, 0.1f, 1.f));
	ButtonStyle.Pressed.TintColor = FSlateColor(FLinearColor(0.2f, 0.2f, 0.2f, 1.f));
	DisabledBrush.DrawAs = ESlateBrushDrawType::Image;
	DisabledBrush.TintColor = FSlateColor(FLinearColor(0.05f, 0.05f, 0.05f, 1.f));

	DropHintBrush.DrawAs = ESlateBrushDrawType::Image;
	DropHintBrush.TintColor= FLinearColor(0.6f, 0.6f, 1.f,1.f);
	
	if (Data->IsGroup())
	{
		STableRow<TSharedPtr<FRuleDetailViewData>>::Construct(
			STableRow<TSharedPtr<FRuleDetailViewData>>::FArguments()
			.ShowSelection(true)
			.Padding(4)
			.Content()
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				[
					 MakeGroupWidget()
				]
				+ SOverlay::Slot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Top)
				[
					SNew(SBox)
					.Visibility(EVisibility::Collapsed)
					.HeightOverride(4.f)
					[
						SNew(SImage)
						.Image(&DropHintBrush)
					]
				]
			],
			InOwnerTableView
		);
	}
	else
	{
		STableRow<TSharedPtr<FRuleDetailViewData>>::Construct(
			STableRow<TSharedPtr<FRuleDetailViewData>>::FArguments()
			.Padding(4)
			.ToolTipText_Lambda([this](){ return Data->GetItemRule()->Note; })
			.Content()
			[
				SNew(SOverlay)
				+SOverlay::Slot()
				[
					MakeRuleDetailWidget()
				]
				+ SOverlay::Slot()
				.VAlign(VAlign_Top)
				[
					SNew(SBox)
					.Visibility_Lambda([this](){return ShouldShowDropHint? EVisibility::Visible: EVisibility::Collapsed; })
					.HeightOverride(4.f)
					[
						SNew(SImage)
						.Image(&DropHintBrush)
					]
				]
			],
			InOwnerTableView
		);
	}
}

FReply SRuleRow::OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
	{
		return FReply::Handled().BeginDragDrop(FAutoPaintRuleDragDropOp::New(Data));
	}
	return FReply::Unhandled();
}

void SRuleRow::OnDragEnter(FGeometry const& MyGeometry, FDragDropEvent const& DragDropEvent)
{
	TSharedPtr<FAutoPaintRuleDragDropOp> DragDropOp = DragDropEvent.GetOperationAs<FAutoPaintRuleDragDropOp>();
	if (DragDropOp.IsValid())
	{
		if (*DragDropOp->Data.Get() == *Data.Get() || (DragDropOp->Data->IsGroup() && !Data->IsGroup()))
		{
			DragDropOp->SetValidTarget(false);
		}
		else
		{
			DragDropOp->SetValidTarget(true);
			ShouldShowDropHint = true;
		}
	}
}

void SRuleRow::OnDragLeave(FDragDropEvent const& DragDropEvent)
{
	
	TSharedPtr<FAutoPaintRuleDragDropOp> DragDropOp = DragDropEvent.GetOperationAs<FAutoPaintRuleDragDropOp>();
	if (DragDropOp.IsValid())
	{
		DragDropOp->SetValidTarget(false);
	}
	ShouldShowDropHint = false;
}

FReply SRuleRow::OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	TSharedPtr<FAutoPaintRuleDragDropOp> DragDropOp = DragDropEvent.GetOperationAs<FAutoPaintRuleDragDropOp>();
	if (!DragDropOp.IsValid()) return FReply::Unhandled();
	if (DragDropOp->Data->IsGroup() && Data.Get()->IsGroup()) // swap group
	{
		FScopedTransaction Transaction(LOCTEXT("AutoPaintRuleEditor_MoveGroup", "Move group"));
		Data.Get()->Rule->Modify();
		Data.Get()->Rule->Groups.Swap(Data.Get()->GroupIdx, DragDropOp->Data.Get()->GroupIdx);
	}
	else if (!DragDropOp->Data->IsGroup() && Data.Get()->IsGroup()) // drag item directly on above group
	{
		FScopedTransaction Transaction(LOCTEXT("AutoPaintRuleEditor_MoveRuleToGroup", "Move rule to group"));
		Data.Get()->Rule->Modify();
		Data->GetGroup()->Rules.Add(DragDropOp->Data->GetItemRule());
		DragDropOp->Data->GetGroup()->Rules.RemoveAt(DragDropOp->Data->RuleIdx);
	}
	else // drag item on item
	{
		if (Data->GetItemRule() == DragDropOp->Data->GetItemRule()) return FReply::Unhandled();
		FScopedTransaction Transaction(LOCTEXT("AutoPaintRuleEditor_ReorderRule", "Reorder rule"));
		Data.Get()->Rule->Modify();
		FGuid ToRemove = DragDropOp->Data->GetItemRule()->RuleId;
		UAutoPaintItemRule* NewRule = NewObject<UAutoPaintItemRule>(Data->Rule, NAME_None, RF_Transactional);
		NewRule->DeepCopy(*DragDropOp->Data->GetItemRule());
		Data->GetGroup()->Rules.Insert(NewRule, Data->RuleIdx);
		DragDropOp->Data->GetGroup()->Rules.RemoveAll([=](UAutoPaintItemRule* D)
		{
			return D->RuleId == ToRemove;
		});
		
		 // These are swap logic... maybe one day I'll reuse them?
		// (DragDropOp->Data.Get()->GetItemRule());
		// if (Data->GroupIdx == DragDropOp->Data->GroupIdx)
		// {
		// 	// Data.Get()->GetGroup()->Rules.Swap(Data.Get()->RuleIdx, DragDropOp->Data.Get()->RuleIdx);
		// 	// const int ToRemoveIdx = DragDropOp->Data.Get()->RuleIdx > Data.Get()->RuleIdx? Data->RuleIdx + 1 : Data->RuleIdx;
		// 	Data.Get()->GetGroup()->Rules.Remove(DragDropOp->Data->GetItemRule());
		// }
		// else
		// {
		// 	Data.Get()->GetGroup()->Rules.Insert(DragDropOp->Data.Get()->GetItemRule(), Data.Get()->RuleIdx);
		// 	// DragDropOp->Data->GetGroup()->Rules.Insert(Data->GetGroup()->Rules[Data->RuleIdx + 1], DragDropOp->Data->RuleIdx);
		// 	// Data.Get()->GetGroup()->Rules.RemoveAt(Data.Get()->RuleIdx + 1);
		// }
	}
	Data.Get()->Rule->RefreshRuleEditor_Delegate.Execute();
	Data.Get()->Rule->RefreshSpawnedInstance_Delegate.Execute();
	return FReply::Handled();
	
}


TSharedRef<SWidget> SRuleRow::MakeGroupWidget()
{
	FLinearColor TextColor = Data->GetGroup()->bEnabled? FLinearColor::White : FLinearColor::Gray;
	
	return SNew(SHorizontalBox)
	.ToolTipText_Lambda([this](){ return Data->GetGroup()->Note; })
	+ SHorizontalBox::Slot()
	.AutoWidth()
	.HAlign(HAlign_Left)
	.VAlign(VAlign_Center)
	.Padding(16, 0, 4, 0)
	[
		SNew(SCheckBox)
		.ToolTipText(LOCTEXT("GroupEnabledTooltip", "Should enable this rule group?"))
		.IsChecked(this, &SRuleRow::IsGroupEnabled)
		.OnCheckStateChanged(this, &SRuleRow::OnGroupEnabledChanged)
		.Style(&FTiledLevelStyle::Get().GetWidgetStyle<FCheckBoxStyle>("TiledLevel.EnableRuleCheckbox"))
	]
	+ SHorizontalBox::Slot()
	.HAlign(HAlign_Fill)
	.VAlign(VAlign_Center)
	.Padding(2, 0)
	.FillWidth(1.f)
	[
		SNew(STextBlock)
		.Text(FText::FromName(Data->GetGroup()->GroupName))
		.Font(FCoreStyle::GetDefaultFontStyle("Bold", 18))
		.ColorAndOpacity(TextColor)
	]
	+ SHorizontalBox::Slot()
	.AutoWidth()
	.HAlign(HAlign_Right)
	.VAlign(VAlign_Center)
	.Padding(2.f, 0.f)
	[
		SNew(SBox)
		.WidthOverride(BtnSize[0])
		.HeightOverride(BtnSize[1])
		[
			SNew(SButton)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.ToolTipText(LOCTEXT("AddRuleToGroup_Tooltip", "Create new rule to this group"))
			.ButtonStyle(&ButtonStyle)
			[
				SNew(SImage)
				.Image(FAppStyle::GetBrush("Icons.Plus"))
			]
			.OnClicked(this, &SRuleRow::OnAddRuleClicked)
		]
	];
}

ECheckBoxState SRuleRow::IsGroupEnabled() const
{
	return Data->GetGroup()->bEnabled? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SRuleRow::OnGroupEnabledChanged(ECheckBoxState CheckBoxState)
{
	CheckBoxState == ECheckBoxState::Checked? Data->GetGroup()->bEnabled = true : Data->GetGroup()->bEnabled = false;
	if (!Data->Rule)
	{
		ERROR_LOG("Ptr to rule is missing..., when group enabled changed...")
		return;
	}
	Data->Rule->RefreshSpawnedInstance_Delegate.Execute();
}

FReply SRuleRow::OnAddRuleClicked()
{
	FScopedTransaction Transaction(LOCTEXT("AddRule", "Add Rule"));
	Data->Rule->Modify();
	Data->GetGroup()->Rules.Add(NewObject<UAutoPaintItemRule>(Data->Rule, NAME_None, RF_Transactional));
	if (!Data->Rule)
	{
		ERROR_LOG("Ptr to rule is missing..., when group enabled changed...")
		return FReply::Unhandled();
	}
	
	// TODO:
	ParentEditor->MakeTreeItemExpand(Data);
	Data->Rule->RefreshRuleEditor_Delegate.Execute();
	return FReply::Handled();
}

FReply SRuleRow::OnEditStepClicked()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::Get().GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	UAutoPaintItemRule* Rule = Data->GetItemRule();
	FSinglePropertyParams InitParams;
	TSharedPtr<ISinglePropertyView> StepSizePropertyView = PropertyModule.CreateSingleProperty(
		Rule,
		GET_MEMBER_NAME_CHECKED(UAutoPaintItemRule, StepSize), InitParams);
	TSharedPtr<ISinglePropertyView> StepOffsetPropertyView = PropertyModule.CreateSingleProperty(
		Rule,
		GET_MEMBER_NAME_CHECKED(UAutoPaintItemRule, StepOffset), InitParams);

// TODO: duplicated instance should never appear... based on my core rules... however, props types will still...
	
	SGenericDialogWidget::FArguments DialogArgs;
	SGenericDialogWidget::OpenDialog(
		LOCTEXT("EditSpawnRuleTitle", "Edit Step Size"),
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(12.f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("StepSizeHelpText", "Chage step size to further restrict positions where this rule would apply. \nBy default, the rule checking process"
				" will scan all positions in your tiled level. \nIf you set it to 2x2x2, this rule will only be scaned "
				"on every 2X, 2Y, 2Z positions in your level.\n\n You must set it carefully if the extent size for your item is not 1x1x1!\n"))
			.WrapTextAt(320.f)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			StepSizePropertyView.ToSharedRef()
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			StepOffsetPropertyView.ToSharedRef()
		],
		DialogArgs,
		false);
	return FReply::Handled();
}

EVisibility SRuleRow::GetVisibilityByEnabledState(bool bVisibleWhenEnabled) const
{
	if (bVisibleWhenEnabled)
	{
		return Data->GetItemRule()->bEnabled && Data->GetGroup()->bEnabled? EVisibility::Visible : EVisibility::Collapsed;
	}
	return Data->GetItemRule()->bEnabled && Data->GetGroup()->bEnabled? EVisibility::Collapsed : EVisibility::Visible;
}

TSharedRef<SWidget> SRuleRow::MakeRuleDetailWidget()
{
	return SNew(SHorizontalBox)
	.ToolTipText_Lambda([this]()
	{
		if (Data->GetItemRule() == nullptr) return FText::GetEmpty();
		return Data->GetItemRule()->Note;
	})
	+ SHorizontalBox::Slot()
	.AutoWidth()
	.HAlign(HAlign_Left)
	.VAlign(VAlign_Center)
	.Padding(2.f, 0.f)
	[
		// enabled
		SNew(SCheckBox)
		.Style(&FTiledLevelStyle::Get().GetWidgetStyle<FCheckBoxStyle>("TiledLevel.EnableRuleCheckbox"))
		.IsChecked_Lambda([=]()
		{
			return Data->GetItemRule()->bEnabled? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
		})
		.OnCheckStateChanged_Lambda([=](ECheckBoxState NewState)
		{
			FScopedTransaction Transaction(LOCTEXT("ToggleEnableRule", "Toggle enable rule"));
			Data->Rule->Modify();
			Data->GetItemRule()->bEnabled = NewState == ECheckBoxState::Checked;
			Data->Rule->RefreshSpawnedInstance_Delegate.Execute();
			if (ParentEditor->GetSelectedAutoPaintRule() == Data->GetItemRule())
				Data->Rule->RequestForUpdateMatchHint.Execute(nullptr);
		})
	]
	+ SHorizontalBox::Slot()
	[
		SNew(SHorizontalBox)
		.Visibility(this, &SRuleRow::GetVisibilityByEnabledState, true)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		.Padding(2.f, 0.f)
		[
			// stop or not
			SNew(SCheckBox)
			.ToolTipText(LOCTEXT("StopHere_Tooltip", "When this rule is met, stop the below rules"))
			.Style(&FTiledLevelStyle::Get().GetWidgetStyle<FCheckBoxStyle>("TiledLevel.StopHereCheckBox"))
			.IsChecked_Lambda([=]()
			{
				return Data->GetItemRule()->bStopOnMet? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
			})
			.OnCheckStateChanged_Lambda([=](ECheckBoxState NewState)
			{
				FScopedTransaction Transaction(LOCTEXT("ToggleRuleStop", "Toggle rule stop here"));
				Data->Rule->Modify();
				Data->GetItemRule()->bStopOnMet = NewState == ECheckBoxState::Checked;
				Data->Rule->RefreshSpawnedInstance_Delegate.Execute();
				if (ParentEditor->GetSelectedAutoPaintRule() == Data->GetItemRule())
					Data->Rule->RequestForUpdateMatchHint.Execute(nullptr);
			})
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		.Padding(2.f, 0.f)
		[
			SNew(SSpinBox<float>)
			.EnableSlider(true)
			.MinDesiredWidth(32.f)
			.MinValue(0.f)
			.MaxValue(1.f)
			.ToolTipText(LOCTEXT("ApplyProbability_Tooltip", "Rule apply probability"))
			.Value_Lambda([=]()
			{
				return Data->GetItemRule()->Probability;
			})
			.OnValueChanged_Lambda([=](float NewValue)
			{
				FScopedTransaction Transaction(LOCTEXT("ChangeApplyProbability", "Change apply probability"));
				Data->Rule->Modify();
				Data->GetItemRule()->Probability = NewValue;
			})
			.OnValueCommitted_Lambda([=](float NewValue, ETextCommit::Type CommitType)
			{
				FScopedTransaction Transaction(LOCTEXT("ChangeApplyProbability", "Change apply probability"));
				Data->Rule->Modify();
				Data->GetItemRule()->Probability = NewValue;
				Data->Rule->RefreshSpawnedInstance_Delegate.Execute();
			})
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		.Padding(8, 0, 0, 0)
		[
			SAssignNew(AdjacencyRuleWidget, SAdjacencyRule, Data, ParentEditor)
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		.Padding(0, 0, 8, 0)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.VAlign(VAlign_Bottom)
			.HAlign(HAlign_Center)
			[
				SNew(SCheckBox)
				.ToolTipText(LOCTEXT("ToggleIncludeZ_Tooltip", "Toggle whether to include Z axis for impact size. ex: 3 x 3 -> 3 x 3 x 3"))
				.Style(&FTiledLevelStyle::Get().GetWidgetStyle<FCheckBoxStyle>("TiledLevel.IncludeZCheckBox"))
				.Visibility_Lambda([=]()
				{
					return Data->GetItemRule()->ImpactSize == EImpactSize::One? EVisibility::Collapsed : EVisibility::Visible;
				})
				.IsChecked_Lambda([=]()
				{
					return Data->GetItemRule()->bIncludeZ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
				})
				.OnCheckStateChanged_Lambda([=](ECheckBoxState NewState)
				{
					FScopedTransaction Transaction(LOCTEXT("ToggleRule_IncludeZ", "Toggle rule: include z"));
					Data->Rule->Modify();
					Data->GetItemRule()->bIncludeZ = NewState == ECheckBoxState::Checked;
					Data->Rule->RefreshSpawnedInstance_Delegate.Execute();
					if (ParentEditor->GetSelectedAutoPaintRule() == Data->GetItemRule())
						Data->Rule->RequestForUpdateMatchHint.Execute(Data->GetItemRule());
					if (NewState == ECheckBoxState::Unchecked)
						Data->GetItemRule()->CurrentEditImpactFloor = 0;
					AdjacencyRuleWidget->Update();
					FloorSelectWidget->Update();
				})
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SAssignNew(FloorSelectWidget, SImpactFloorSelect, Data, AdjacencyRuleWidget)
			]
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		[
			SNew(SUniformGridPanel)
			.SlotPadding(2.f)
			+ SUniformGridPanel::Slot(0, 0)
			[
				// size select
				SNew(SComboButton)
				.ToolTipText(LOCTEXT("SetImpactSize_Tooltip", "Set the impact size of this rule"))
				.HasDownArrow(true)
				.ContentPadding(FMargin(1, 0))
				.OnGetMenuContent_Lambda([this]()
				{
					FMenuBuilder MenuBuilder(true, nullptr);
					for(EImpactSize s : TEnumRange<EImpactSize>())
					{
						FExecuteAction Action = FExecuteAction::CreateLambda([=]()
						{
							if (Data->GetItemRule()->ImpactSize == s)
								return;
							FScopedTransaction Transaction(LOCTEXT("ChangeImpactSize", "Change impact size"));
							Data->Rule->Modify();
							Data->GetItemRule()->UpdateImpactSize(s);
							Data->GetItemRule()->CurrentEditImpactFloor = 0;
							AdjacencyRuleWidget->Update();
							FloorSelectWidget->Update();
						});
						MenuBuilder.AddMenuEntry(UEnum::GetDisplayValueAsText(s), UEnum::GetDisplayValueAsText(s), FSlateIcon(), FUIAction(Action));
					}
					return MenuBuilder.MakeWidget();
				})
				.ButtonContent()
				[
					SNew(STextBlock)
					.Text_Lambda([this](){ return UEnum::GetDisplayValueAsText(Data->GetItemRule()->ImpactSize); })
				]
			]
			+ SUniformGridPanel::Slot(1, 0)
			[
				// boundary rule
				SNew(SBox)
				.WidthOverride(96.f)
				[
					 SNew(SComboButton)
					 .ToolTipText(LOCTEXT("SetOutOfBoundaryRule_Tooltip", "Set rule for impact size out of boundary of the level"))
					 .HasDownArrow(true)
					 .ContentPadding(FMargin(1, 0))
					 .OnGetMenuContent_Lambda([this]()
					 {
						 FMenuBuilder MenuBuilder(true, nullptr);
						 TArray<FName> OutOfBoundaryRuleOptions;
						 TArray<FText> OutOfBoundaryRuleTooltips;
						 for (auto [K, V] : Data->Rule->Items)
						 {
							  OutOfBoundaryRuleOptions.Add(K);
							  OutOfBoundaryRuleTooltips.Add(FText::Format(LOCTEXT("Item_Tooltip", "treat impact size out of boundary as {0}"), FText::FromName(K)));
						 }
						  OutOfBoundaryRuleOptions.Add("Not Applicable");
						  OutOfBoundaryRuleTooltips.Add(LOCTEXT("Item_Tooltip", "This rule will not apply on border"));
						 for (int32 i = 0; i < OutOfBoundaryRuleOptions.Num(); i++)
						 {
						 	FExecuteAction Action = FExecuteAction::CreateLambda([=]()
						 	{
						 		if (Data->GetItemRule()->OutOfBoundaryRule == OutOfBoundaryRuleOptions[i])
						 	        return;
							  	FScopedTransaction Transaction(LOCTEXT("ChangeOutOfBoundaryRule", "Change out of boundary rule"));
						 		Data->Rule->Modify();
						 		Data->GetItemRule()->OutOfBoundaryRule = OutOfBoundaryRuleOptions[i];
						 		Data->Rule->RefreshSpawnedInstance_Delegate.Execute();
						 		if (ParentEditor->GetSelectedAutoPaintRule() == Data->GetItemRule())
						 		    Data->Rule->RequestForUpdateMatchHint.Execute(Data->GetItemRule());
						 	});
						 	MenuBuilder.AddMenuEntry(FText::FromName(OutOfBoundaryRuleOptions[i]), OutOfBoundaryRuleTooltips[i], FSlateIcon(), FUIAction(Action));
						 }
						 return MenuBuilder.MakeWidget();
					 })
					 .ButtonContent()
					 [
						 SNew(STextBlock)
						 .Clipping(EWidgetClipping::ClipToBounds)
						 .OverflowPolicy(ETextOverflowPolicy::Ellipsis)
						 .Text_Lambda([=]()
						 {
						 	return FText::FromName(Data->GetItemRule()->OutOfBoundaryRule);
						 })
					 ]
				]
			]
			+ SUniformGridPanel::Slot(0, 1)
			[
				// duplication select
				SNew(SComboButton)
				.ToolTipText(LOCTEXT("AutoDuplication_Tooltip", "Automatically add extra duplicated mirrored or rotated versions of adjacency rule"))
				.HasDownArrow(true)
				.ContentPadding(FMargin(1, 0))
				.OnGetMenuContent_Lambda([this]()
				{
					const UEnum* EC_RuleDuplication = StaticEnum<ERuleDuplication>();
					FMenuBuilder MenuBuilder(true, nullptr);
					for(const ERuleDuplication D : TEnumRange<ERuleDuplication>())
					{
						FExecuteAction Action = FExecuteAction::CreateLambda([=]()
						{
							if (Data->GetItemRule()->RuleDuplication == D)
								return;
							FScopedTransaction Transaction(LOCTEXT("ChangeRuleDuplicationMode", "Change auto rule duplication"));
							Data->Rule->Modify();
							Data->GetItemRule()->RuleDuplication = D;
							Data->Rule->RefreshSpawnedInstance_Delegate.Execute();
							if (ParentEditor->GetSelectedAutoPaintRule() == Data->GetItemRule())
								Data->Rule->RequestForUpdateMatchHint.Execute(Data->GetItemRule());
						});
						MenuBuilder.AddMenuEntry(UEnum::GetDisplayValueAsText(D), EC_RuleDuplication->GetToolTipTextByIndex((int64)D), FSlateIcon(), FUIAction(Action));
					}
					return MenuBuilder.MakeWidget();
				})
				.ButtonContent()
				[
					SNew(STextBlock)
					.Text_Lambda([this](){ return UEnum::GetDisplayValueAsText(Data->GetItemRule()->RuleDuplication); })
				]
			]
			+ SUniformGridPanel::Slot(1, 1)
			[
				SNew(SButton)
				.ToolTipText(FText::FromString("Edit step size to apply this rule"))
				.OnClicked(this, &SRuleRow::OnEditStepClicked)
				.Content()
				[
					SNew(STextBlock)
					.Text_Lambda([this]()
					{
						UAutoPaintItemRule* Rule = Data->GetItemRule();
						return FText::Format(LOCTEXT("StepSizeBtn_Label", "{0}x{1}x{2}"),
							FText::AsNumber(Rule->StepSize.X), 
							FText::AsNumber(Rule->StepSize.Y), 
							FText::AsNumber(Rule->StepSize.Z) 
							);
					})
					.ColorAndOpacity_Lambda([this]()
					{
						if (Data->GetItemRule()->StepSize == FIntVector(1))
							return FLinearColor::Gray;
						return FLinearColor::White;
					})
					
				]
			]
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1.f)
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Center)
		[
			SAssignNew(TargetItemsSelectWidget, SSpawnRuleWidget, Data, AdjacencyRuleWidget)
		]
	]
	+SHorizontalBox::Slot()
	[
		SNew(SBorder)
		.Visibility(this, &SRuleRow::GetVisibilityByEnabledState, false)
		.BorderImage(&DisabledBrush)
		.Padding(12, 4)
		.Content()
		[
			SNew(STextBlock)
			.Font(FCoreStyle::GetDefaultFontStyle("Normal", 12))
			.Text(LOCTEXT("DisabledText", "Disabled..."))
			.ColorAndOpacity(FLinearColor(0.1, 0.1, 0.1, 1))
		]
	];

}

// End of SRuleGroup


void SAutoPaintRuleDetailsEditor::Construct(const FArguments& InArgs, UAutoPaintRule* InRule, TSharedPtr<class FUICommandList> InParentCommandList)
{
	Rule = InRule;
	Rule->RefreshRuleEditor_Delegate.BindSP(this, &SAutoPaintRuleDetailsEditor::UpdateEditor);
	ParentCommandList = InParentCommandList;
	
	ChildSlot
	[
		SAssignNew(RuleTreeWidget, STreeView<TSharedPtr<FRuleDetailViewData>>)
		.TreeItemsSource(&ViewData)
		.SelectionMode(ESelectionMode::Single)
		.OnGenerateRow(this, &SAutoPaintRuleDetailsEditor::GenerateItemRow)
		.OnGetChildren(this, &SAutoPaintRuleDetailsEditor::OnGetChildren)
		.OnSelectionChanged(this, &SAutoPaintRuleDetailsEditor::OnSelectionChanged)
		.OnContextMenuOpening(this, &SAutoPaintRuleDetailsEditor::ConstructContextMenu)
	];
	UpdateEditor();
	for (FName GroupName : Rule->OpenedGroups)
	{
		for (auto Data : ViewData)
		{
			if (Data->IsGroup() && Data->GetGroup()->GroupName == GroupName)
				RuleTreeWidget->SetItemExpansion(Data, true);
		}
	}
}

bool SAutoPaintRuleDetailsEditor::IsAnySelected() const
{
	return RuleTreeWidget->GetNumItemsSelected() > 0;
}

void SAutoPaintRuleDetailsEditor::UpdateEditor()
{
	// Cache previous selection
	FGuid PreviousSelectedItemId;
	if (RuleTreeWidget->GetNumItemsSelected() > 0)
	{
		if	(UAutoPaintItemRule* SelectedItemRule = RuleTreeWidget->GetSelectedItems()[0]->GetItemRule())
		{
			PreviousSelectedItemId = SelectedItemRule->RuleId;
		}
	}
	ViewData.Empty();
	// // handle NONE
	Rule->Groups.RemoveAll([](const FAutoPaintRuleGroup& G)
	{
		return G.GroupName == FName("None");
	});
	
	// build tree items
	for (int i = 0; i < Rule->Groups.Num(); i++)
	{
		ViewData.Add(MakeShareable(new FRuleDetailViewData(Rule, i, INDEX_NONE)));
	}
	
	TSet<TSharedPtr<FRuleDetailViewData>> OldExpansionState;
	RuleTreeWidget->GetExpandedItems(OldExpansionState);

	RuleTreeWidget->RequestTreeRefresh();
	
	for (const auto& NewItem : ViewData)
	{
		for (const auto& OldItem : OldExpansionState)
		{
			if (OldItem->IsGroup() && NewItem->IsGroup() && OldItem->GroupIdx == NewItem->GroupIdx)
			{
				RuleTreeWidget->SetItemExpansion(NewItem, true);
				break;
			}
		}
	}
	// revert previous selection
	for (auto Data : ViewData)
	{
		if (Data->GetItemRule() && Data->GetItemRule()->RuleId == PreviousSelectedItemId)
		{
			RuleTreeWidget->SetSelection(Data);
			break;
		}
	}
}

UAutoPaintItemRule* SAutoPaintRuleDetailsEditor::GetSelectedAutoPaintRule() const
{
	if (IsAnySelected())
	{
		return RuleTreeWidget->GetSelectedItems()[0]->GetItemRule();
	}
	return nullptr;
}

FAutoPaintRuleGroup* SAutoPaintRuleDetailsEditor::GetSelectedAutoPaintGroup() const
{
	if (IsAnySelected())
	{
		return RuleTreeWidget->GetSelectedItems()[0]->GetGroup();
	}
	return nullptr;
}

void SAutoPaintRuleDetailsEditor::GetSelectedItemRuleInfo(FName& ParentGroupName, int& RuleIdxInGroup) const
{
	if (!IsAnySelected()) return;
	ParentGroupName = RuleTreeWidget->GetSelectedItems()[0]->GetGroup()->GroupName;
	RuleIdxInGroup = RuleTreeWidget->GetSelectedItems()[0]->RuleIdx;
}

TArray<FName> SAutoPaintRuleDetailsEditor::GetOpenedGroups()
{
	TArray<FName> Out;
	TSet<TSharedPtr<FRuleDetailViewData>> ExpansionState;
	RuleTreeWidget->GetExpandedItems(ExpansionState);
	for (auto Data : ExpansionState)
	{
		if (Data->IsGroup()) Out.Add(Data->GetGroup()->GroupName);
	}
	return Out;
}

void SAutoPaintRuleDetailsEditor::MakeTreeItemExpand(TSharedPtr<FRuleDetailViewData> Data)
{
	RuleTreeWidget->SetItemExpansion(Data, true);
}


TSharedRef<ITableRow> SAutoPaintRuleDetailsEditor::GenerateItemRow(TSharedPtr<FRuleDetailViewData> InData, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SRuleRow, InData, SharedThis(this), OwnerTable);
}

TSharedPtr<SWidget> SAutoPaintRuleDetailsEditor::ConstructContextMenu() const
{
	const FGenericCommands& GenericCommands = FGenericCommands::Get();
	const FTiledLevelCommands& TLCommands = FTiledLevelCommands::Get();
	FMenuBuilder MenuBuilder(true, ParentCommandList);
	MenuBuilder.BeginSection("BasicOperations", LOCTEXT("BasicActionsHeader", "Basic actions"));
	{
		MenuBuilder.AddMenuEntry(GenericCommands.Delete);
		MenuBuilder.AddMenuEntry(GenericCommands.Copy);
		MenuBuilder.AddMenuEntry(GenericCommands.Paste);
		MenuBuilder.AddMenuEntry(GenericCommands.Duplicate);
	}
	MenuBuilder.EndSection();
	MenuBuilder.BeginSection("Create", LOCTEXT("CreateActionHeader", "Create"));
	{
		MenuBuilder.AddMenuEntry(
			TLCommands.CreateNewRuleGroup,
			NAME_None,
			TAttribute<FText>(), 
			TAttribute<FText>(),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Plus")
			);
	}
	MenuBuilder.EndSection();
	MenuBuilder.BeginSection("Note", LOCTEXT("NoteHeader", "Note"));
	{
		MenuBuilder.AddWidget(
			SNew(SMultiLineEditableTextBox)
			.HintText(FText::FromString("Your note..."))
			.Text_Lambda([this]()
			{
				if (!IsAnySelected()) return FText::GetEmpty();
				if (RuleTreeWidget->GetSelectedItems()[0]->IsGroup())
					return RuleTreeWidget->GetSelectedItems()[0]->GetGroup()->Note;
				return RuleTreeWidget->GetSelectedItems()[0]->GetItemRule()->Note;
			})
			.OnTextCommitted_Lambda([this](const FText& NewText, ETextCommit::Type CommitType)
			{
				if (RuleTreeWidget->GetSelectedItems()[0]->IsGroup())
				{
					RuleTreeWidget->GetSelectedItems()[0]->GetGroup()->Note = NewText;
				}
				else
				{
					 RuleTreeWidget->GetSelectedItems()[0]->GetItemRule()->Note = NewText;
				}
			})
			.OnTextChanged_Lambda([this](const FText& NewText)
			{
				if (RuleTreeWidget->GetSelectedItems()[0]->IsGroup())
				{
					RuleTreeWidget->GetSelectedItems()[0]->GetGroup()->Note = NewText;
				}
				else
				{
					 RuleTreeWidget->GetSelectedItems()[0]->GetItemRule()->Note = NewText;
				}
			})
			,
			FText::GetEmpty()
		);
	}
	MenuBuilder.EndSection();
	return MenuBuilder.MakeWidget();
}

void SAutoPaintRuleDetailsEditor::OnGetChildren(TSharedPtr<FRuleDetailViewData> InData, TArray<TSharedPtr<FRuleDetailViewData>>& OutData)
{
	if (InData->IsGroup())
	{
		for (int i = 0 ; i < Rule->Groups[InData->GroupIdx].Rules.Num(); i++)
		{
			OutData.Add(MakeShareable(new FRuleDetailViewData(Rule, InData->GroupIdx, i)));
		}
	}
}


void SAutoPaintRuleDetailsEditor::OnSelectionChanged(TSharedPtr<FRuleDetailViewData> NewSelection, ESelectInfo::Type Arg)
{
	if (IsAnySelected())
	{
		if (NewSelection->IsGroup())
		{
			Rule->SelectionState = EAutoPaintEditorSelectionState::Group;
			Rule->RequestForUpdateMatchHint.Execute(nullptr);
		}
		else
		{
			Rule->SelectionState = EAutoPaintEditorSelectionState::Rule;
			Rule->RequestForUpdateMatchHint.Execute(NewSelection->GetItemRule());
		}
	}
	else
	{
		 Rule->SelectionState = EAutoPaintEditorSelectionState::None;
		 Rule->RequestForUpdateMatchHint.Execute(nullptr);
	}
}

FReply SAutoPaintRuleDetailsEditor::OnAddGroupClicked()
{
	SGenericDialogWidget::FArguments DialogArgs;
	DialogArgs.OnOkPressed(this, &SAutoPaintRuleDetailsEditor::OnAddGroupConfirmed);
	SGenericDialogWidget::OpenDialog(LOCTEXT("AddNewGroupTitle", "Add New Group"), MakeAddGroupModal(), DialogArgs, false);
	return FReply::Handled();
}

void SAutoPaintRuleDetailsEditor::OnAddGroupConfirmed()
{
	if (CandidateNewGroupName.IsEmpty())
	{
		return;
	}
	if (Rule->Groups.ContainsByPredicate([this](const FAutoPaintRuleGroup& Group)
	{
		return Group.GroupName == FName(CandidateNewGroupName.ToString());
	}))
	{
		return;
	}
	FScopedTransaction Transaction(LOCTEXT("AutoPaintRuleEditor_ModyfyAdjacencyRule", "Modift adjacency rule"));
	Rule->Modify();
	FAutoPaintRuleGroup NewGroup;
	NewGroup.GroupName = FName(CandidateNewGroupName.ToString());
	Rule->Groups.Add(NewGroup);
	CandidateNewGroupName = FText::GetEmpty();
	UpdateEditor();
}

FText SAutoPaintRuleDetailsEditor::GetAddNewGroupErrorMessage() const
{
	return NewGroupNameErrorMessage;
}

TSharedRef<SWidget> SAutoPaintRuleDetailsEditor::MakeAddGroupModal()
{
	return SNew(SVerticalBox)
	+ SVerticalBox::Slot()
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.VAlign(VAlign_Center)
		.AutoWidth()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("NewGroupName_Label", "New Group Name: "))
		]
		+ SHorizontalBox::Slot()
		.HAlign(HAlign_Left)
		.AutoWidth()
		[
			SNew(SBox)
			.HeightOverride(32)
			.WidthOverride(128)
			[
				SNew(SEditableTextBox)
				.Text(CandidateNewGroupName)
				.OnTextChanged_Lambda([this](const FText& NewText)
				{
					if (NewText.IsEmpty())
					{
						NewGroupNameErrorMessage = FText::FromString("Invalid! New Group is empty.");
						return ;
					}
					if (Rule->Groups.ContainsByPredicate([=](const FAutoPaintRuleGroup& Group)
					{
						return Group.GroupName == FName(NewText.ToString());
					}))
					{
						NewGroupNameErrorMessage = FText::FromString("Invalid! New Group already exists.");
						return;
					}
					NewGroupNameErrorMessage = FText::GetEmpty();
				})
				.OnTextCommitted_Lambda([this](const FText& Text, ETextCommit::Type Arg)
				{
					CandidateNewGroupName = Text;
				})	
			]
		]

	]
	+ SVerticalBox::Slot()
	.Padding(0, 4)
	[
		SNew(STextBlock)
		.Text(this, &SAutoPaintRuleDetailsEditor::GetAddNewGroupErrorMessage)
		.ColorAndOpacity(FLinearColor::Red)
	];
}


END_SLATE_FUNCTION_BUILD_OPTIMIZATION

#undef LOCTEXT_NAMESPACE