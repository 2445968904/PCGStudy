//$ Copyright 2015-23, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/LevelEditor/Customizations/SnapMapModuleBoundsCustomization.h"

#include "Core/Common/Utils/DungeonEditorUtils.h"
#include "Frameworks/Snap/SnapMap/SnapMapModuleBounds.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "EdMode.h"

void FSnapMapModuleBoundsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) {
	IDetailCategoryBuilder& Category = DetailBuilder.EditCategory("Dungeon Architect", FText::GetEmpty(), ECategoryPriority::Important);

	auto SetPropertyVisible = [&](const FName& InPropertyPath, bool bVisible) {
		if (bVisible) {
			const TSharedRef<IPropertyHandle> PropertyHandle = DetailBuilder.GetProperty(InPropertyPath);
			if (FProperty* Property = PropertyHandle->GetProperty()) {
				Property->SetMetaData(FEdMode::MD_MakeEditWidget, TEXT("true"));
			}
		}
		else {
			const TSharedRef<IPropertyHandle> PropertyHandle = DetailBuilder.GetProperty(InPropertyPath);
			PropertyHandle->MarkHiddenByCustomization();
			if (FProperty* Property = PropertyHandle->GetProperty()) {
				Property->RemoveMetaData(FEdMode::MD_MakeEditWidget);
			}
		}
	};

	if (const ASnapMapModuleBounds* BoundsActor = FDungeonEditorUtils::GetBuilderObject<ASnapMapModuleBounds>(&DetailBuilder)) {
		SetPropertyVisible(GET_MEMBER_NAME_CHECKED(ASnapMapModuleBounds, BoxExtent), BoundsActor->BoundsType == EDABoundsShapeType::Box);
		SetPropertyVisible(GET_MEMBER_NAME_CHECKED(ASnapMapModuleBounds, CircleRadius), BoundsActor->BoundsType == EDABoundsShapeType::Circle);
		SetPropertyVisible(GET_MEMBER_NAME_CHECKED(ASnapMapModuleBounds, PolygonPoints), BoundsActor->BoundsType == EDABoundsShapeType::Polygon);
	}

}

TSharedRef<IDetailCustomization> FSnapMapModuleBoundsCustomization::MakeInstance() {
	return MakeShareable(new FSnapMapModuleBoundsCustomization);
}



