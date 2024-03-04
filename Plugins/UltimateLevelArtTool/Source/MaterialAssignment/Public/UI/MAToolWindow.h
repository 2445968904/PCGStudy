// Copyright 2023 Leartes Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityWidget.h"
#include "AssetRegistry/ARFilter.h"
#include "Interfaces/MAToolWindowInterface.h"
#include "MAToolWindow.generated.h"

class UScrollBox;
class UMAAssignmentSlot;
class UODPresetObject;
class UODSpawnCenterData;
class AStaticMeshActor;
class UODDistributionBase;
class UButton;
class AActor;
class UStaticMesh;
class UScrollBox;
class UPanelWidget;


UCLASS()
class MATERIALASSIGNMENT_API UMAToolWindow : public UEditorUtilityWidget, public IMAToolWindowInterface
{
	GENERATED_BODY()

	bool bMouseOnWToolWindow;

	UPROPERTY()
	TArray<FName> SlotNames;

	UPROPERTY()
	TArray<UStaticMesh*> SelectedStaticMeshes;

	UPROPERTY()
	TArray<UMAAssignmentSlot*> MaterialSlotWidgets;
	
	int32 AlteredSlotCounter;

	FARFilter Filter;
	
public:
	virtual void NativePreConstruct() override;
	
	virtual void NativeConstruct() override;

	virtual void NativeDestruct() override;
protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ApplyAllBtn;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UScrollBox> SlotBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> ApplyAllBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UTextBlock> NoMeshText;
	
	UFUNCTION()
	void ApplyAllBtnPressed();

private:
	void UpdateSelectedAssets();
	
	void ResetAllVariables();

	TArray<UStaticMesh*> FilterStaticMeshAssets(const TArray<UObject*>& Assets) const;

	void RecalculateApplyAllBtnStatus();

public:
	virtual void MaterialPropertyChanged(bool bIncreaseCounter) override;
	virtual void ApplySingleMaterialChange(const FName& SlotName , UMaterialInterface* InMaterialInterface) override;
	virtual bool RenameMaterialSlot(const FName& CurrentSlotName,const FName& NewSlotName) override;
	
	void HandleOnAssetSelectionChanged(const TArray<FAssetData>& NewSelectedAssets, bool bIsPrimaryBrowser);
	void HandleOnAssetPathChanged(const FString& NewPath);
	void HandleOnAssetRemoved(const FAssetData& InAsset);
	void HandleOnAssetRenamed(const FAssetData& InAsset, const FString& InOldObjectPath);

private:
	void FixUpTheList();
	void FixUpDirectories() const;
};
