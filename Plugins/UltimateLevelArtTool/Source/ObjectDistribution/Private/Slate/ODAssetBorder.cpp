// Copyright 2023 Leartes Studios. All Rights Reserved.

#include "ODAssetBorder.h"
#include "Slate/SlateBrushAsset.h"
#include "Materials/MaterialInterface.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "ODSAssetBorder.h"
#include "ODBorderSlot.h"
#include "ObjectEditorUtils.h"

#define LOCTEXT_NAMESPACE "UMG"


UODAssetBorder::UODAssetBorder(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer), bFlipForRightToLeftFlowDirection(false)
{
	bIsVariable = false;

	ContentColorAndOpacity = FLinearColor::White;
	BrushColor = FLinearColor::White;

	Padding = FMargin(4, 2);

	HorizontalAlignment = HAlign_Fill;
	VerticalAlignment = VAlign_Fill;

	DesiredSizeScale = FVector2D(1, 1);

	bShowEffectWhenDisabled = true;
}

void UODAssetBorder::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	MyBorder.Reset();
}

TSharedRef<SWidget> UODAssetBorder::RebuildWidget()
{
	MyBorder = SNew(SODAssetBorder)
		.FlipForRightToLeftFlowDirection(bFlipForRightToLeftFlowDirection);
	
	if ( GetChildrenCount() > 0 )
	{
		Cast<UODBorderSlot>(GetContentSlot())->BuildSlot(MyBorder.ToSharedRef());
	}

	return MyBorder.ToSharedRef();
}

void UODAssetBorder::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	
	TAttribute<FLinearColor> ContentColorAndOpacityBinding = PROPERTY_BINDING(FLinearColor, ContentColorAndOpacity);
	TAttribute<FSlateColor> BrushColorBinding = OPTIONAL_BINDING_CONVERT(FLinearColor, BrushColor, FSlateColor, ConvertLinearColorToSlateColor);
	TAttribute<const FSlateBrush*> ImageBinding = OPTIONAL_BINDING_CONVERT(FSlateBrush, Background, const FSlateBrush*, ConvertImage);
	
	MyBorder->SetPadding(Padding);
	MyBorder->SetBorderBackgroundColor(BrushColorBinding);
	MyBorder->SetColorAndOpacity(ContentColorAndOpacityBinding);

	MyBorder->SetBorderImage(ImageBinding);
	
	MyBorder->SetDesiredSizeScale(DesiredSizeScale);
	MyBorder->SetShowEffectWhenDisabled(bShowEffectWhenDisabled != 0);
	
	MyBorder->SetOnMouseButtonDown(BIND_UOBJECT_DELEGATE(FPointerEventHandler, HandleMouseButtonDown));
	MyBorder->SetOnMouseButtonUp(BIND_UOBJECT_DELEGATE(FPointerEventHandler, HandleMouseButtonUp));
	MyBorder->SetOnMouseMove(BIND_UOBJECT_DELEGATE(FPointerEventHandler, HandleMouseMove));
	MyBorder->SetOnMouseDoubleClick(BIND_UOBJECT_DELEGATE(FPointerEventHandler, HandleMouseDoubleClick));
}

UClass* UODAssetBorder::GetSlotClass() const
{
	return UODBorderSlot::StaticClass();
}

void UODAssetBorder::OnSlotAdded(UPanelSlot* InSlot)
{
	// Copy the content properties into the new slot so that it matches what has been setup
	// so far by the user.
	UODBorderSlot* BorderSlot = CastChecked<UODBorderSlot>(InSlot);
	BorderSlot->Padding = Padding;
	BorderSlot->HorizontalAlignment = HorizontalAlignment;
	BorderSlot->VerticalAlignment = VerticalAlignment;

	// Add the child to the live slot if it already exists
	if ( MyBorder.IsValid() )
	{
		// Construct the underlying slot.
		BorderSlot->BuildSlot(MyBorder.ToSharedRef());
	}
}

void UODAssetBorder::OnSlotRemoved(UPanelSlot* InSlot)
{
	// Remove the widget from the live slot if it exists.
	if ( MyBorder.IsValid() )
	{
		MyBorder->SetContent(SNullWidget::NullWidget);
	}
}

void UODAssetBorder::SetContentColorAndOpacity(FLinearColor Color)
{
	ContentColorAndOpacity = Color;
	if ( MyBorder.IsValid() )
	{
		MyBorder->SetColorAndOpacity(Color);
	}
}

void UODAssetBorder::SetPadding(FMargin InPadding)
{
	Padding = InPadding;
	if ( MyBorder.IsValid() )
	{
		MyBorder->SetPadding(InPadding);
	}
}

void UODAssetBorder::SetHorizontalAlignment(EHorizontalAlignment InHorizontalAlignment)
{
	HorizontalAlignment = InHorizontalAlignment;
	if ( MyBorder.IsValid() )
	{
		MyBorder->SetHAlign(InHorizontalAlignment);
	}
}

void UODAssetBorder::SetVerticalAlignment(EVerticalAlignment InVerticalAlignment)
{
	VerticalAlignment = InVerticalAlignment;
	if ( MyBorder.IsValid() )
	{
		MyBorder->SetVAlign(InVerticalAlignment);
	}
}

void UODAssetBorder::SetBrushColor(FLinearColor Color)
{
	BrushColor = Color;
	if ( MyBorder.IsValid() )
	{
		MyBorder->SetBorderBackgroundColor(Color);
	}
}

FReply UODAssetBorder::HandleMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& MouseEvent)
{
	if ( OnMouseButtonDownEvent.IsBound() )
	{
		return OnMouseButtonDownEvent.Execute(Geometry, MouseEvent).NativeReply;
	}

	return FReply::Unhandled();
}

FReply UODAssetBorder::HandleMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& MouseEvent)
{
	if ( OnMouseButtonUpEvent.IsBound() )
	{
		return OnMouseButtonUpEvent.Execute(Geometry, MouseEvent).NativeReply;
	}

	return FReply::Unhandled();
}

FReply UODAssetBorder::HandleMouseMove(const FGeometry& Geometry, const FPointerEvent& MouseEvent)
{
	if ( OnMouseMoveEvent.IsBound() )
	{
		return OnMouseMoveEvent.Execute(Geometry, MouseEvent).NativeReply;
	}

	return FReply::Unhandled();
}

FReply UODAssetBorder::HandleMouseDoubleClick(const FGeometry& Geometry, const FPointerEvent& MouseEvent)
{
	if ( OnMouseDoubleClickEvent.IsBound() )
	{
		return OnMouseDoubleClickEvent.Execute(Geometry, MouseEvent).NativeReply;
	}

	return FReply::Unhandled();
}

void UODAssetBorder::SetBrush(const FSlateBrush& Brush)
{
	Background = Brush;

	if ( MyBorder.IsValid() )
	{
		MyBorder->SetBorderImage(&Background);
	}
}

void UODAssetBorder::SetBrushFromAsset(USlateBrushAsset* Asset)
{
	Background = Asset ? Asset->Brush : FSlateBrush();

	if ( MyBorder.IsValid() )
	{
		MyBorder->SetBorderImage(&Background);
	}
}

void UODAssetBorder::SetBrushFromTexture(UTexture2D* Texture)
{
	Background.SetResourceObject(Texture);

	if ( MyBorder.IsValid() )
	{
		MyBorder->SetBorderImage(&Background);
	}
}

void UODAssetBorder::SetBrushFromMaterial(UMaterialInterface* Material)
{
	if (!Material)
	{
		UE_LOG(LogSlate, Log, TEXT("UBorder::SetBrushFromMaterial.  Incoming material is null"));
	}

	Background.SetResourceObject(Material);
	
	if ( MyBorder.IsValid() )
	{
		MyBorder->SetBorderImage(&Background);
	}
}

UMaterialInstanceDynamic* UODAssetBorder::GetDynamicMaterial()
{
	UMaterialInterface* Material = nullptr;

	UObject* Resource = Background.GetResourceObject();
	Material = Cast<UMaterialInterface>(Resource);

	if ( Material )
	{
		UMaterialInstanceDynamic* DynamicMaterial = Cast<UMaterialInstanceDynamic>(Material);

		if ( !DynamicMaterial )
		{
			DynamicMaterial = UMaterialInstanceDynamic::Create(Material, this);
			Background.SetResourceObject(DynamicMaterial);

			if ( MyBorder.IsValid() )
			{
				MyBorder->SetBorderImage(&Background);
			}
		}

		return DynamicMaterial;
	}
	return nullptr;
}

void UODAssetBorder::SetDesiredSizeScale(FVector2D InScale)
{
	DesiredSizeScale = InScale;
	if (MyBorder.IsValid())
	{
		MyBorder->SetDesiredSizeScale(InScale);
	}
}

const FSlateBrush* UODAssetBorder::ConvertImage(TAttribute<FSlateBrush> InImageAsset) const
{
	UODAssetBorder* MutableThis = const_cast<UODAssetBorder*>( this );
	MutableThis->Background = InImageAsset.Get();

	return &Background;
}

void UODAssetBorder::PostLoad()
{
	Super::PostLoad();
	
	if ( GetChildrenCount() > 0 )
	{
		if ( UPanelSlot* PanelSlot = GetContentSlot() )
		{
			UODBorderSlot* BorderSlot = Cast<UODBorderSlot>(PanelSlot);
			if ( BorderSlot == NULL )
			{
				BorderSlot = NewObject<UODBorderSlot>(this);
				BorderSlot->Content = GetContentSlot()->Content;
				BorderSlot->Content->Slot = BorderSlot;
				Slots[0] = BorderSlot;
			}
		}
	}
}

#if WITH_EDITOR

void UODAssetBorder::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	static bool IsReentrant = false;

	if ( !IsReentrant )
	{
		IsReentrant = true;

		if ( PropertyChangedEvent.Property )
		{
			static const FName PaddingName("Padding");
			static const FName HorizontalAlignmentName("HorizontalAlignment");
			static const FName VerticalAlignmentName("VerticalAlignment");

			FName PropertyName = PropertyChangedEvent.Property->GetFName();

			if ( UODBorderSlot* BorderSlot = Cast<UODBorderSlot>(GetContentSlot()) )
			{
				if (PropertyName == PaddingName)
				{
					FObjectEditorUtils::MigratePropertyValue(this, PaddingName, BorderSlot, PaddingName);
				}
				else if (PropertyName == HorizontalAlignmentName)
				{
					FObjectEditorUtils::MigratePropertyValue(this, HorizontalAlignmentName, BorderSlot, HorizontalAlignmentName);
				}
				else if (PropertyName == VerticalAlignmentName)
				{
					FObjectEditorUtils::MigratePropertyValue(this, VerticalAlignmentName, BorderSlot, VerticalAlignmentName);
				}
			}
		}

		IsReentrant = false;
	}
}

const FText UODAssetBorder::GetPaletteCategory()
{
	return LOCTEXT("Common", "Common");
}

#endif

#undef LOCTEXT_NAMESPACE


