// Copyright 2022 PufStudio. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "AutoPaintRuleFactory.generated.h"

class UTiledItemSet;
UCLASS()
class TILEDLEVEL_API UAutoPaintRuleFactory : public UFactory
{
    GENERATED_BODY()
	
public:
	UAutoPaintRuleFactory();
    virtual bool ConfigureProperties() override;
    virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags,
	    UObject* Context, FFeedbackContext* Warn) override;
	void SetSourceItemSet(const TWeakObjectPtr<UTiledItemSet>& InItemSetPtr);

	
private:
    // Init item set, can never be nullptr
	TWeakObjectPtr<UTiledItemSet> InitialItemSetPtr;

	TSharedRef<SWidget> CreateDialogContent();
	bool bSuccessInitItemSet = false;
	UObject* GetItemSet() const;
	void OnSetItemSet(UObject* NewAsset);
};
