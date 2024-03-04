// Copyright 2023 Leartes Studios. All Rights Reserved.

#include "ODSAssetBorder.h"

#include "Editor.h"
#include "ODToolSubsystem.h"
#include "Engine/StaticMesh.h"
#include "Rendering/DrawElements.h"

static FName SBorderTypeName("SODAssetBorder");

SLATE_IMPLEMENT_WIDGET(SODAssetBorder)
void SODAssetBorder::PrivateRegisterAttributes(FSlateAttributeInitializer& AttributeInitializer)
{
	SLATE_ADD_MEMBER_ATTRIBUTE_DEFINITION_WITH_NAME(AttributeInitializer, "BorderImage", BorderImageAttribute, EInvalidateWidgetReason::Paint);
	SLATE_ADD_MEMBER_ATTRIBUTE_DEFINITION_WITH_NAME(AttributeInitializer, "BorderBackgroundColor", BorderBackgroundColorAttribute, EInvalidateWidgetReason::Paint);
	SLATE_ADD_MEMBER_ATTRIBUTE_DEFINITION_WITH_NAME(AttributeInitializer, "DesiredSizeScale", DesiredSizeScaleAttribute, EInvalidateWidgetReason::Layout);
	SLATE_ADD_MEMBER_ATTRIBUTE_DEFINITION_WITH_NAME(AttributeInitializer, "ShowDisabledEffect", ShowDisabledEffectAttribute, EInvalidateWidgetReason::Paint);
}

SODAssetBorder::SODAssetBorder()
	: BorderImageAttribute(*this, FCoreStyle::Get().GetBrush( "Border" ))
	, BorderBackgroundColorAttribute(*this, FLinearColor::White)
	, DesiredSizeScaleAttribute(*this, FVector2D(1,1))
	, ShowDisabledEffectAttribute(*this, true)
	, bFlipForRightToLeftFlowDirection(false)
{
}

void SODAssetBorder::Construct( const SODAssetBorder::FArguments& InArgs )
{
	// Only do this if we're exactly an SBorder
	if ( GetType() == SBorderTypeName )
	{
		SetCanTick(false);
		bCanSupportFocus = false;
	}
	
	SetContentScale(InArgs._ContentScale);
	SetColorAndOpacity(InArgs._ColorAndOpacity);
	SetDesiredSizeScale(InArgs._DesiredSizeScale);
	SetShowEffectWhenDisabled(InArgs._ShowEffectWhenDisabled);

	bFlipForRightToLeftFlowDirection = InArgs._FlipForRightToLeftFlowDirection;

	SetBorderImage(InArgs._BorderImage);
	SetBorderBackgroundColor(InArgs._BorderBackgroundColor);
	SetForegroundColor(InArgs._ForegroundColor);

	if (InArgs._OnMouseButtonDown.IsBound())
	{
		SetOnMouseButtonDown(InArgs._OnMouseButtonDown);
	}

	if (InArgs._OnMouseButtonUp.IsBound())
	{
		SetOnMouseButtonUp(InArgs._OnMouseButtonUp);
	}

	if (InArgs._OnMouseMove.IsBound())
	{
		SetOnMouseMove(InArgs._OnMouseMove);
	}

	if (InArgs._OnMouseDoubleClick.IsBound())
	{
		SetOnMouseDoubleClick(InArgs._OnMouseDoubleClick);
	}

	ChildSlot
	.HAlign(InArgs._HAlign)
	.VAlign(InArgs._VAlign)
	.Padding(InArgs._Padding)
	[
		SNew(SAssetDropTarget)
		.OnAreAssetsAcceptableForDrop(this, &SODAssetBorder::OnAreAssetsValidForDrop)
		.OnAssetsDropped(this, &SODAssetBorder::HandlePlacementDropped)
		.bSupportsMultiDrop(true)
		[
			InArgs._Content.Widget
		]
	];
}

void SODAssetBorder::SetContent( TSharedRef< SWidget > InContent )
{
	ChildSlot
	[
		SNew(SAssetDropTarget)
		.OnAreAssetsAcceptableForDrop(this, &SODAssetBorder::OnAreAssetsValidForDrop)
		.OnAssetsDropped(this, &SODAssetBorder::HandlePlacementDropped)
		.bSupportsMultiDrop(true)
		[
			InContent
		]
	];
}

const TSharedRef< SWidget >& SODAssetBorder::GetContent() const
{
	return ChildSlot.GetWidget();
}

void SODAssetBorder::ClearContent()
{
	ChildSlot.DetachWidget();
}

int32 SODAssetBorder::OnPaint( const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const
{
	const FSlateBrush* BrushResource = BorderImageAttribute.Get();
		
	const bool bEnabled = ShouldBeEnabled(bParentEnabled);

	if ( BrushResource && BrushResource->DrawAs != ESlateBrushDrawType::NoDrawType )
	{
		const bool bShowDisabledEffect = GetShowDisabledEffect();
		const ESlateDrawEffect DrawEffects = (bShowDisabledEffect && !bEnabled) ? ESlateDrawEffect::DisabledEffect : ESlateDrawEffect::None;

		if (bFlipForRightToLeftFlowDirection && GSlateFlowDirection == EFlowDirection::RightToLeft)
		{
			const FGeometry FlippedGeometry = AllottedGeometry.MakeChild(FSlateRenderTransform(FScale2D(-1, 1)));
			FSlateDrawElement::MakeBox(
				OutDrawElements,
				LayerId,
				FlippedGeometry.ToPaintGeometry(),
				BrushResource,
				DrawEffects,
				BrushResource->GetTint(InWidgetStyle) * InWidgetStyle.GetColorAndOpacityTint() * BorderBackgroundColorAttribute.Get().GetColor(InWidgetStyle)
			);
		}
		else
		{
			FSlateDrawElement::MakeBox(
				OutDrawElements,
				LayerId,
				AllottedGeometry.ToPaintGeometry(),
				BrushResource,
				DrawEffects,
				BrushResource->GetTint(InWidgetStyle) * InWidgetStyle.GetColorAndOpacityTint() * BorderBackgroundColorAttribute.Get().GetColor(InWidgetStyle)
			);
		}
	}

	return SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bEnabled );
}

FVector2D SODAssetBorder::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
	return DesiredSizeScaleAttribute.Get() * Super::ComputeDesiredSize(LayoutScaleMultiplier);
}

bool SODAssetBorder::OnAreAssetsValidForDrop(TArrayView<FAssetData> DraggedAssets) const //Static Mesh Filter
{
	if (const auto ToolSubsystem = GEditor->GetEditorSubsystem<UODToolSubsystem>())
	{
		if(ToolSubsystem->bInMixerMode)
		{
			return false;
		}
	}
	
	for (const FAssetData& AssetData : DraggedAssets)
	{
		if (AssetData.GetClass() == UStaticMesh::StaticClass())
		{
			continue;
		}
		return false;
	}
	
	return true;
}

void SODAssetBorder::HandlePlacementDropped(const FDragDropEvent& DragDropEvent, TArrayView<FAssetData> DraggedAssets) const
{
	AssetDroppedSignature.ExecuteIfBound(DraggedAssets);
		
}

void SODAssetBorder::SetBorderBackgroundColor(TAttribute<FSlateColor> InColorAndOpacity)
{
	BorderBackgroundColorAttribute.Assign(*this, InColorAndOpacity);
}

void SODAssetBorder::SetDesiredSizeScale(TAttribute<FVector2D> InDesiredSizeScale)
{
	DesiredSizeScaleAttribute.Assign(*this, InDesiredSizeScale);
}

void SODAssetBorder::SetHAlign(EHorizontalAlignment HAlign)
{
	ChildSlot.SetHorizontalAlignment(HAlign);
}

void SODAssetBorder::SetVAlign(EVerticalAlignment VAlign)
{
	ChildSlot.SetVerticalAlignment(VAlign);
}

void SODAssetBorder::SetPadding(TAttribute<FMargin> InPadding)
{
	ChildSlot.SetPadding(MoveTemp(InPadding));
}

void SODAssetBorder::SetShowEffectWhenDisabled(TAttribute<bool> InShowEffectWhenDisabled)
{
	ShowDisabledEffectAttribute.Assign(*this, InShowEffectWhenDisabled);
}

void SODAssetBorder::SetBorderImage(TAttribute<const FSlateBrush*> InBorderImage)
{
	BorderImageAttribute.Assign(*this, InBorderImage);
}
