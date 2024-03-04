// Copyright 2022 PufStudio. All Rights Reserved.

#include "TiledLevelPropertyCustomizations.h"

#include "AutoPaintRule.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "TiledLevelAsset.h"
#include "PropertyCustomizationHelpers.h"
#include "TiledLevelItem.h"
#include "SAssetSearchBox.h"
#include "TiledLevelEditorUtility.h"
#include "TiledLevelSettings.h"

#define LOCTEXT_NAMESPACE "TiledLevel"

void FTemplateAssetDetails::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow,
                                            IPropertyTypeCustomizationUtils& CustomizationUtils)
{
    TSharedPtr<IPropertyHandle> MyProperty = PropertyHandle->GetChildHandle(TEXT("TemplateAsset"));
    TArray<UObject*> Owners;
    PropertyHandle->GetOuterObjects(Owners);
    if (Owners.IsValidIndex(0))
    {
        Owner = Cast<UTiledLevelTemplateItem>(Owners[0]);
    }
    
    HeaderRow
    .NameContent()
    [
        MyProperty->CreatePropertyNameWidget()
    ]
    .ValueContent()
    .MinDesiredWidth(300.f)
    [
        SNew(SObjectPropertyEntryBox)
        .AllowedClass(UTiledLevelAsset::StaticClass())
        .PropertyHandle(MyProperty)
        .ThumbnailPool(CustomizationUtils.GetThumbnailPool())
        .AllowClear(false)
        .DisplayUseSelected(true)
        .DisplayBrowse(true) // enable this could open weird tab..., its not a big deal compares to its merit?
        .NewAssetFactories(TArray<UFactory*>{}) // inconvenient but the most safe way, not allowed to create new asset here
        .OnShouldFilterAsset(this, &FTemplateAssetDetails::OnShouldFilterTemplateAsset)
    ];
    
}

void FTemplateAssetDetails::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle,
    IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
}

bool FTemplateAssetDetails::OnShouldFilterTemplateAsset(const FAssetData& AssetData) const
{
    if (UTiledLevelAsset* TLA = Cast<UTiledLevelAsset>(AssetData.GetAsset()))
    {
        if (Owner)
        {
            if (UTiledItemSet* OwnerItemSet = Cast<UTiledItemSet>(Owner->GetOuter()))
            {
                return TLA->GetTileSize() != OwnerItemSet->GetTileSize();
            }
        }
    }
    return true;
}

void FTiledItemObjDetails::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow,
                                           IPropertyTypeCustomizationUtils& CustomizationUtils)
{
    TiledItemProperty = PropertyHandle->GetChildHandle(TEXT("TiledItemID"));
    FString Temp_MyTiledItemID;
    TiledItemProperty->GetValue(Temp_MyTiledItemID);
    const FGuid ItemID = FGuid(Temp_MyTiledItemID);
    
    ThumbnailPool = CustomizationUtils.GetThumbnailPool();
    TArray<UObject*> Owners;
    PropertyHandle->GetOuterObjects(Owners);
    ItemRule = Cast<UAutoPaintItemRule>(Owners[0]);
    ItemSet = Cast<UAutoPaintRule>(Owners[0]->GetOutermostObject())->GetItemSet();
    UTiledLevelItem* Item = ItemSet->GetItem(ItemID);
    SelectedAssetThumbnail = MakeShareable(new FAssetThumbnail(Item, 64, 64, ThumbnailPool));
    FAssetThumbnailConfig ThumbnailConfig;
	FTiledLevelEditorUtility::ConfigThumbnailAssetColor(ThumbnailConfig, Item);
    
    HeaderRow
    .NameContent()
    [
        TiledItemProperty->CreatePropertyNameWidget()
    ]
    .ValueContent()
    .MinDesiredWidth(300.f)
    [
        SNew(SHorizontalBox)
        +SHorizontalBox::Slot()
        .VAlign(VAlign_Center)
        .AutoWidth()
        .Padding(2)
        [
            SAssignNew(SelectedItemBox, SBox)
            .WidthOverride(64.f)
            .HeightOverride(64.f)
            [
                SelectedAssetThumbnail->MakeThumbnailWidget(ThumbnailConfig)
            ]
        ]
        + SHorizontalBox::Slot()
        .VAlign(VAlign_Center)
        .AutoWidth()
        .Padding(8, 0)
        [
            SNew(SBox)
            .WidthOverride(160.f)
            [
                SAssignNew(ComboButton, SComboButton)
                .HasDownArrow(true)
                .ContentPadding(FMargin(6.f, 2.f))
                .OnGetMenuContent(this, &FTiledItemObjDetails::MakePickItemMenu)
                .MenuPlacement(MenuPlacement_CenteredBelowAnchor) // TODO: change menu spawn position not working... maybe ignore it for now...
                .ButtonContent()
                [
                    SNew(STextBlock).Text_Lambda([this]()
                    {
                        FString IDString;
                        TiledItemProperty->GetValue(IDString);
                        UTiledLevelItem* QItem = ItemSet->GetItem(FGuid(IDString));
                        if (!QItem) return FText::GetEmpty();
                        return FText::FromString(QItem->GetItemName());
                    })
                ]
            ]
        ]
    ];

}

void FTiledItemObjDetails::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder,
    IPropertyTypeCustomizationUtils& CustomizationUtils)
{
}

void FTiledItemObjDetails::OnSearchBoxChanged(const FText& InText)
{
    SearchText = InText;
}

void FTiledItemObjDetails::OnSearchBoxCommitted(const FText& InText, ETextCommit::Type CommitType)
{
    SearchText = InText;
}

TSharedRef<SWidget> FTiledItemObjDetails::MakeTypeFilterWidget()
{
	FMenuBuilder MenuBuilder(true, nullptr);
	MenuBuilder.BeginSection("TiledItemFilterOption", LOCTEXT("FilterOptionHeading", "Filter Option"));
	{
		TSharedRef<SHorizontalBox> FilterOption_PlacedType1 = SNew(SHorizontalBox);
		TSharedRef<SHorizontalBox> FilterOption_PlacedType2 = SNew(SHorizontalBox);
		TSharedRef<SHorizontalBox> FilterOption_StructureType = SNew(SHorizontalBox);
		TSharedRef<SHorizontalBox> FilterOption_SourceType = SNew(SHorizontalBox);

		TMap<bool*, FText> TypeOptions;
		TypeOptions.Add(&bShowBlock, LOCTEXT("FilterOption_Block",  "Block"));      
		TypeOptions.Add(&bShowFloor,LOCTEXT("FilterOption_Floor",  "Floor"));
		TypeOptions.Add(&bShowWall,LOCTEXT("FilterOption_Wall",   "Wall"));       
		TypeOptions.Add(&bShowEdge,LOCTEXT("FilterOption_Edge",   "Edge"));       
		TypeOptions.Add(&bShowPillar,LOCTEXT("FilterOption_Pillar", "Pillar"));
		TypeOptions.Add(&bShowPoint,LOCTEXT("FilterOption_Point", "Point"));
		TypeOptions.Add(&bShowStructure,LOCTEXT("FilterOption_Structure", "Structure"));
		TypeOptions.Add(&bShowProp,LOCTEXT("FilterOption_Prop", "Prop"));
		TypeOptions.Add(&bShowActor,LOCTEXT("FilterOption_Actor", "Actor"));
		TypeOptions.Add(&bShowMesh,LOCTEXT("FilterOption_Mesh", "Mesh"));

		int i = 0;
		FText ParentLabel;
		for (auto [k, v] : TypeOptions )
		{
			TTuple<bool*, FText> TempData = MakeTuple(k, v);
			TSharedPtr<SHorizontalBox> Parent;
			if (i < 3)
			{
				Parent = FilterOption_PlacedType1;
				ParentLabel = LOCTEXT("FilterOptionLabel_PlacedType", "Placed type: ");
			}
			else if (i < 6)
			{
				Parent = FilterOption_PlacedType2;
				ParentLabel = FText::GetEmpty();
			}
			else if (i < 8)
			{
				Parent = FilterOption_StructureType;
				ParentLabel = LOCTEXT("FilterOptionLabel_StructureType", "Structure type: ");
			}
			else
			{
				Parent = FilterOption_SourceType;
				ParentLabel = LOCTEXT("FilterOptionLabel_SourceType", "Source type: ");
			}
			if (Parent->NumSlots() == 0)
			{
				Parent->AddSlot()
				.AutoWidth()
				.HAlign(HAlign_Left)
				[
					SNew(SBox)
					.MinDesiredWidth(96.f)
					[
						SNew(STextBlock)
						.Text(ParentLabel)
					]
				];
			}
			Parent->AddSlot()
			.AutoWidth()
			.HAlign(HAlign_Right)
			[
				SNew(SCheckBox)
				.IsChecked_Lambda([=]()
				{
					return *TempData.Key ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; 
				})
				.OnCheckStateChanged_Lambda([=](ECheckBoxState NewState)
				{
					*(TempData.Key) = NewState == ECheckBoxState::Checked;
				})
			];
			Parent->AddSlot()
			.AutoWidth()
			.HAlign(HAlign_Left)
			[
				SNew(SBox)
				.MinDesiredWidth(64.f)
				[
					 SNew(STextBlock).Text(TempData.Value)
				]
			];
			i++;
		}

		TSharedRef<SBox> ResetButton = SNew(SBox)
		.Padding(FMargin(64, 0))
		 [
			 SNew(SButton)
			 .OnClicked_Lambda([this]()
			 {
			 	bShowBlock = true;
			 	bShowFloor = true;
				bShowWall = true;
				bShowEdge = true;
				bShowPillar = true;
				bShowPoint = true;
				bShowStructure = true;
				bShowProp = true;
				bShowActor = true;
				bShowMesh = true;
			 	return FReply::Handled();
			 })
			 [
				 SNew(STextBlock)
				 .Justification(ETextJustify::Center)
				 .Text(LOCTEXT("ResetFilterOptions_Label", "Reset filter"))
			 ]
		 ];

		MenuBuilder.AddWidget(FilterOption_PlacedType1, FText::GetEmpty());
		MenuBuilder.AddWidget(FilterOption_PlacedType2, FText::GetEmpty());
		MenuBuilder.AddWidget(FilterOption_StructureType, FText::GetEmpty());
		MenuBuilder.AddWidget(FilterOption_SourceType, FText::GetEmpty());
		MenuBuilder.AddWidget(ResetButton,FText::GetEmpty());
	}
	MenuBuilder.EndSection();
	return MenuBuilder.MakeWidget();
}

TSharedRef<SWidget> FTiledItemObjDetails::MakePickItemMenu()
{
    // refresh picked items
    PickedItems.Empty();
    for (const auto& [k, v] :ItemRule->AutoPaintItemSpawns)
    {
        PickedItems.Add(FGuid(k.TiledItemID));
    }

    FMenuBuilder MenuBuilder(true, nullptr);
    MenuBuilder.BeginSection("Filter", LOCTEXT("FilterLabel", "Filter"));
    {
        TSharedPtr<SWidget> SearchWidget = SNew(SBox)
        .Padding(24, 0)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .FillWidth(1.0)
            [
                SAssignNew(SearchBox, SAssetSearchBox)
                .HintText(LOCTEXT("SearchBoxHint", "Search Item" ))
                .OnTextChanged(this, &FTiledItemObjDetails::OnSearchBoxChanged)
                .OnTextCommitted(this, &FTiledItemObjDetails::OnSearchBoxCommitted)
                .DelayChangeNotificationsWhileTyping(true)
            ]
        ];
        MenuBuilder.AddWidget(SearchWidget.ToSharedRef(), FText::GetEmpty(), true, false);
    }
    MenuBuilder.EndSection();
    
    MenuBuilder.BeginSection("Item list", LOCTEXT("ItemListLabel", "Item List"));
    {
        for (auto& Item : ItemSet->GetItemSet())
        {
            if (PickedItems.Contains(Item->ItemID)) continue;
            if (Item->IsSpecialItem()) continue;
        	if (ItemRule->StructureType != Item->StructureType) continue;
        	if (ItemRule->PlacedType != Item->PlacedType) continue;
        	// xy swapped is allowed as long they rotates...
        	bool IsExtentPassed = false;
        	const FIntVector ItemExtent = FIntVector(Item->Extent);
        	if (ItemExtent == ItemRule->Extent)
        		IsExtentPassed = true;
        	else if (ItemExtent.X == ItemRule->Extent.Y && ItemExtent.Y == ItemRule->Extent.X && ItemExtent.Z == ItemRule->Extent.Z)
        		IsExtentPassed = true;
        	if (!IsExtentPassed) continue;
			// TODO: send need to rotate 1 or 3 times or can only rotate 0, 2 times information to somewhere... ???

        	
            FAssetData AssetData = FAssetData(Item);
            TSharedPtr<FAssetThumbnail> Thumbnail = MakeShareable(
                new FAssetThumbnail(AssetData, 64, 64, ThumbnailPool));
            FAssetThumbnailConfig Config;
            FTiledLevelEditorUtility::ConfigThumbnailAssetColor(Config, Item);

            TSharedPtr<SWidget> ItemWidget = SNew(SHorizontalBox)
            +SHorizontalBox::Slot()
            .AutoWidth()
            [
                SNew(SBox)
                .WidthOverride(64.f)
                .HeightOverride(64.f)
                [
                    Thumbnail->MakeThumbnailWidget(Config)
                ]
            ]
            +SHorizontalBox::Slot()
            .VAlign(VAlign_Center)
            .HAlign(HAlign_Left)
            .AutoWidth()
            [
                SNew(SVerticalBox)
                +SVerticalBox::Slot()
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(Item->GetItemName()))
                ]
                + SVerticalBox::Slot()
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(Item->GetItemInfo()))
                ]
            ];
            
            FUIAction Action;
            Action.ExecuteAction.BindLambda([=]()
            {
                TiledItemProperty->SetValue(Item->ItemID.ToString());
                SelectedAssetThumbnail->SetAsset(Item);
                SelectedItemBox->SetContent(SelectedAssetThumbnail->MakeThumbnailWidget());
            });
            Action.IsActionVisibleDelegate.BindLambda([=]()
            {
                if (SearchText.IsEmpty()) return true;
                return Item->GetItemName().Contains(SearchText.ToString())? true : false;
            });
            MenuBuilder.AddMenuEntry(Action, ItemWidget.ToSharedRef());
        }
    }
    MenuBuilder.EndSection();

	ComboButton->SetMenuContentWidgetToFocus(SearchBox);
    
    return SNew(SBox)
	.MaxDesiredHeight(600.f)
	.MinDesiredHeight(600.f)
	[MenuBuilder.MakeWidget()];
}

#undef LOCTEXT_NAMESPACE
