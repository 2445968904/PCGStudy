// Copyright 2023 Leartes Studios. All Rights Reserved.

#include "ODBorderSlot.h"
#include "Widgets/SNullWidget.h"
#include "ODSAssetBorder.h"
#include "ODAssetBorder.h"
#include "ObjectEditorUtils.h"

/////////////////////////////////////////////////////
// UBorderSlot

UODBorderSlot::UODBorderSlot(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Padding = FMargin(4, 2);

	HorizontalAlignment = HAlign_Fill;
	VerticalAlignment = VAlign_Fill;
}

void UODBorderSlot::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	Border.Reset();
}

void UODBorderSlot::BuildSlot(TSharedRef<SODAssetBorder> InBorder)
{
	Border = InBorder;
	Border.Pin()->SetPadding(Padding);
	Border.Pin()->SetHAlign(HorizontalAlignment);
	Border.Pin()->SetVAlign(VerticalAlignment);

	Border.Pin()->SetContent(Content ? Content->TakeWidget() : SNullWidget::NullWidget);
}

void UODBorderSlot::SetPadding(FMargin InPadding)
{
	CastChecked<UODAssetBorder>(Parent)->SetPadding(InPadding);
}

void UODBorderSlot::SetHorizontalAlignment(EHorizontalAlignment InHorizontalAlignment)
{
	CastChecked<UODAssetBorder>(Parent)->SetHorizontalAlignment(InHorizontalAlignment);
}

void UODBorderSlot::SetVerticalAlignment(EVerticalAlignment InVerticalAlignment)
{
	CastChecked<UODAssetBorder>(Parent)->SetVerticalAlignment(InVerticalAlignment);
}

void UODBorderSlot::SynchronizeProperties()
{
	if ( Border.IsValid() )
	{
		SetPadding(Padding);
		SetHorizontalAlignment(HorizontalAlignment);
		SetVerticalAlignment(VerticalAlignment);
	}
}

#if WITH_EDITOR

void UODBorderSlot::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
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

			if ( UODAssetBorder* ParentBorder = CastChecked<UODAssetBorder>(Parent) )
			{
				if ( PropertyName == PaddingName)
				{
					FObjectEditorUtils::MigratePropertyValue(this, PaddingName, ParentBorder, PaddingName);
				}
				else if ( PropertyName == HorizontalAlignmentName)
				{
					FObjectEditorUtils::MigratePropertyValue(this, HorizontalAlignmentName, ParentBorder, HorizontalAlignmentName);
				}
				else if ( PropertyName == VerticalAlignmentName)
				{
					FObjectEditorUtils::MigratePropertyValue(this, VerticalAlignmentName, ParentBorder, VerticalAlignmentName);
				}
			}
		}

		IsReentrant = false;
	}

	
}


#endif
