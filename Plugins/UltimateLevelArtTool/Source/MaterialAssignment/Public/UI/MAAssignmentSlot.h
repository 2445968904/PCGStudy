// Copyright 2023 Leartes Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MAAssignmentSlot.generated.h"

class UMAMaterialParamObject;
class USinglePropertyView;
class UButton;
class UTextBlock;
class UEditableTextBox;
class UPanelWidget;

UCLASS()
class MATERIALASSIGNMENT_API UMAAssignmentSlot : public UUserWidget
{
	GENERATED_BODY()

	FName SlotName;
	
	FString DefaultMaterialName;
	
	bool bMaterialParamChangedOnce;

public:
	virtual void NativePreConstruct() override;
	
	virtual void NativeConstruct() override;

	virtual void NativeDestruct() override;

protected:
	UPROPERTY()
	TObjectPtr<UMAMaterialParamObject> MaterialParamObject;
	
protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CounterText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> MultipleChoiceBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UEditableTextBox> SlotRenameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> MeshNameList;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USinglePropertyView> MaterialPropertyView;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ApplyBtn;


	UFUNCTION()
	void ApplyBtnPressed();

	UFUNCTION()
	void OnSlotNameTextCommitted(const FText& Text,ETextCommit::Type CommitMethod);


private:
	TArray<FString> MeshNames;

	void RefreshTheSMNameList();

public:
	void InitializeTheSlot(const FName& InSlotName,UMaterialInterface* InExistingMaterial, const FString& InFirstMeshName);
	void AddNewObjectToCounter(const FString& InMeshName);
	void CheckForMaterialDifferences(const FString& InMaterialName) const;
	TObjectPtr<UMaterialInterface> GetNewMaterial() const;
	FName& GetSlotName(){return SlotName;}
	void DisableApplyButton();
	const bool& IsMaterialParamChanged() const {return bMaterialParamChangedOnce;}

	void AllMaterialsApplied();

protected:
	void OnMaterialParamChanged();
	
};
