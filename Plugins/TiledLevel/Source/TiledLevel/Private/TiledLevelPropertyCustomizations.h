// Copyright 2022 PufStudio. All Rights Reserved.

#pragma once

#include "IDetailCustomization.h"
#include "IPropertyTypeCustomization.h"

/**
 * Inside template item
 */
class FTemplateAssetDetails : public IPropertyTypeCustomization
{
public:

    static TSharedRef<IPropertyTypeCustomization> MakeInstance()
    {
        return MakeShareable(new FTemplateAssetDetails());
    }
    
    virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow,
        IPropertyTypeCustomizationUtils& CustomizationUtils) override;
    virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder,
        IPropertyTypeCustomizationUtils& CustomizationUtils) override;
    
private:
    bool OnShouldFilterTemplateAsset(const FAssetData& AssetData) const;
    class UTiledLevelTemplateItem* Owner = nullptr;
};

class FTiledItemObjDetails : public IPropertyTypeCustomization
{
public:


    static TSharedRef<IPropertyTypeCustomization> MakeInstance()
    {
        return MakeShareable(new FTiledItemObjDetails());
    }

    virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow,
                                 IPropertyTypeCustomizationUtils& CustomizationUtils) override;
    virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder,
        IPropertyTypeCustomizationUtils& CustomizationUtils) override;
    
private:
    void OnSearchBoxChanged(const FText& InText);
    void OnSearchBoxCommitted(const FText& InText, ETextCommit::Type CommitType);
    TSharedPtr<class SComboButton> ComboButton;
    TSharedPtr<class SAssetSearchBox> SearchBox;
    TSharedRef<SWidget> MakeTypeFilterWidget();
    TSharedRef<SWidget> MakePickItemMenu();
    FText SearchText;
    TSharedPtr<SBox> SelectedItemBox;
    TSharedPtr<IPropertyHandle> TiledItemProperty;
    class UAutoPaintItemRule* ItemRule = nullptr;
    class UTiledItemSet* ItemSet = nullptr;
    TSharedPtr<FAssetThumbnail> SelectedAssetThumbnail;
    TSharedPtr<FAssetThumbnailPool> ThumbnailPool;
    TArray<FGuid> PickedItems;
    // filtered options
    bool bShowBlock = true;
    bool bShowFloor = true;
    bool bShowWall = true;
    bool bShowEdge = true;
    bool bShowPillar = true;
    bool bShowPoint = true;
    bool bShowStructure = true;
    bool bShowProp = true;
    bool bShowActor = true;
    bool bShowMesh = true;
};
