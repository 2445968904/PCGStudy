// Copyright 2023 Leartes Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityWidget.h"
#include "ODDistributionBase.h"
#include "Data/ODMeshData.h"
#include "ODToolWindow.generated.h"

class UBorder;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLevelActorDeletedNativeSignature, AActor*, Actor);

class AODSpawnCenter;
class UODPresetObject;
class AStaticMeshActor;
class UODDistributionBase;
class UButton;
class UEditableTextBox;
class UDetailsView;
class UPanelWidget;
class AActor;
class UODAssetBorder;

UCLASS()
class OBJECTDISTRIBUTION_API UODToolWindow : public UEditorUtilityWidget
{
	GENERATED_BODY()

	bool bIsToolDestroying = false;

	bool bTraceForVelocity = false;

	UPROPERTY()
	FVector LastSpawnCenterLocation;

	UPROPERTY()
	FVector SpawnCenterVelocity;

	bool bTraceForRotationDiff = false;
	
	UPROPERTY()
	FRotator LastSpawnCenterRotation;

	UPROPERTY()
	FRotator SpawnCenterRotation;

public:
	virtual void NativePreConstruct() override;
	
	virtual void NativeConstruct() override;

	virtual void NativeDestruct() override;
	
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	
	void OnDistributionTypeChanged(EObjectDistributionType NewDistributionType);
	
	UPROPERTY()
	TObjectPtr<UODDistributionBase> PropDistributionBase;

	UPROPERTY()
	TObjectPtr<UODPresetObject> PresetObject;
	
	static FVector FindSpawnLocationForSpawnCenter();

	void GetSpawnCenterToCameraView() const;

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UDetailsView> PresetDetails;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UDetailsView> PropDistributionDetails;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> PresetSettingsBorder;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> TitleBorder;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UODAssetBorder> AssetDropBorder;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> FinishBtn;

public:
#pragma region Reference


protected:
	UPROPERTY()
	TObjectPtr<AODSpawnCenter> SpawnCenterRef;
	
	void SpawnSpawnCenter();

public:
	AActor* GetSpawnCenterRef() const;

#pragma endregion Reference

	UPROPERTY(BlueprintAssignable)
	FOnLevelActorDeletedNativeSignature OnLevelActorDeletedNative;
	
	void HandleOnLevelActorDeletedNative(AActor* InActor);

	void CopyPermanentDistributionSettings(UODDistributionBase* InCreatedDistObject) const;

protected:
	
#pragma region Presets

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> AddNewPresetBtn;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> AddSelectedAssetsBtn;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> NewPresetText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> AddAssetsText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> RenamePresetBtn;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> RenamePresetText;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> RemovePresetBtn;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> SavePresetBtn;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> SaveAsNewPresetBtn;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> SaveAsText;
	
	UFUNCTION()
	void AddNewPresetBtnPressed();

	UFUNCTION()
	void AddSelectedAssetsBtnPressed();
	
	UFUNCTION()
	void RenamePresetBtnPressed();

	UFUNCTION()
	void RemovePresetBtnPressed();

	UFUNCTION()
	void SavePresetBtnPressed();

	UFUNCTION()
	void SaveAsNewPresetBtnPressed();

	UFUNCTION()
	void OnNewPresetTextCommitted( const FText& InText, ETextCommit::Type InCommitMethod);
	
	UFUNCTION()
	void OnAddAssetsTextCommitted( const FText& InText, ETextCommit::Type InCommitMethod);

	UFUNCTION()
	void OnRenamePresetTextCommitted( const FText& InText, ETextCommit::Type InCommitMethod);

	UFUNCTION()
	void OnSaveAsTextCommitted( const FText& InText, ETextCommit::Type InCommitMethod);

	UFUNCTION()
	void FinishBtnPressed();

private:
	void OnFinishConditionChanged(bool InCanFinish);

	bool bAskOnce;
	float AskOnceTimer;

#pragma endregion Presets

#pragma  region  DistributionVisualiton

	void OnAfterODRegenerated() const;

#pragma  endregion  DistributionVisualiton

private:
	void OnMixerModeChanged(bool InIsInMixerMode);

	void OnPresetCategoryHidden(bool InbIsItOpen);

	void OnAssetDropped(TArrayView<FAssetData> DroppedAssets);

};
