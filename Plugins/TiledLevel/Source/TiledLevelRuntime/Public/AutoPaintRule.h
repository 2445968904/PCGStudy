// Copyright 2023 PufStudio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "TiledLevelItem.h"
#include "Misc/EnumRange.h"
#include "AutoPaintRule.generated.h"

UENUM()
enum class EImpactSize : uint8
{
	One = 0 UMETA(DisplayName = "1 X 1"),
	Three = 1 UMETA(DisplayName = "3 X 3"),
	Five = 2 UMETA(DisplayName = "5 X 5"),
	Count UMETA(Hidden)
};
ENUM_RANGE_BY_COUNT(EImpactSize, EImpactSize::Count);

UENUM()
enum class ERuleDuplication : uint8
{
	No = 0 UMETA(ToolTip = "No additional adjacency rule"),
	Mirrored = 1 UMETA(ToolTip = "Duplicate one extra MIRRORED adjacency rule"),
	Rotated = 2 UMETA(ToolTip = "Duplicate three extra ROTATED adjacency rule"),
	Count UMETA(Hidden)
};
ENUM_RANGE_BY_COUNT(ERuleDuplication, ERuleDuplication::Count);


USTRUCT()
struct FAutoPaintItem
{
	GENERATED_BODY()

	FAutoPaintItem()
	: ItemName(FName("Auto")), PreviewColor(FLinearColor::Red)
	{};

	FAutoPaintItem(const FName& InItemName, const FLinearColor& InColor)
		: ItemName(InItemName), PreviewColor(InColor) 
	{
	}

	UPROPERTY(EditAnywhere, Category="Item")
	FName ItemName;

	UPROPERTY(EditAnywhere, Category="Item")
	FLinearColor PreviewColor = FLinearColor::White;

	bool operator==(const FAutoPaintItem& Other) const
	{
		return ItemName == Other.ItemName;
	}

	bool operator==(const FName& Other) const
	{
		return ItemName == Other;
	}
};

/*
 * ex: bNegate + Wall means except  for wall
 * if ItemName == empty, means empty!
 */
USTRUCT()
struct FAdjacencyRule
{
	GENERATED_BODY()

	FAdjacencyRule() {};

	FAdjacencyRule(FName InItemName, bool InbNegate)
		: AutoPaintItemName(InItemName), bNegate(InbNegate)
	{
	}
	
	UPROPERTY(EditAnywhere, Category="Rule")
	FName AutoPaintItemName = "Empty";

	UPROPERTY(EditAnywhere, Category="Rule")
	bool bNegate = false;
};

USTRUCT()
struct FAutoPaintSpawnAdjustment
{
	GENERATED_USTRUCT_BODY()

	// the coefficient to decide probability to pick this item if multiple items are used
	UPROPERTY(EditAnywhere, Category="Spawn")
	float RandomCoefficient = 1.f;

	// hint: rotate 3 times equals rotate 1 time in counter clockwise direction
	UPROPERTY(EditAnywhere, DisplayName="rotation times (CW)",  meta=(UIMin=0, UIMax=3, ClampMin=0, ClampMax=3), Category="Spawn")
	uint8 RotationTimes = 0;

	// mirror the item in X axis (same function as mirror tool in normal paint)
	UPROPERTY(EditAnywhere, Category="Spawn")
	bool MirrorX = false;
	
	// mirror the item in Y axis
	UPROPERTY(EditAnywhere, Category="Spawn")
	bool MirrorY = false;
	
	// mirror the item in Z axis
	UPROPERTY(EditAnywhere, Category="Spawn")
	bool MirrorZ = false;

	// another additional transform to apply if you need...
	UPROPERTY(EditAnywhere, Category="Spawn")
	FTransform TransformAdjustment;
};

UENUM()
enum class EDuplicationType
{
	No, // No original copy
	R1, // rotation 1 time
	R2, // rotate 2 times = mirrored
	R3 // rotation 3 times
};

struct FDuplicationMod
{
	FDuplicationMod(uint8 InRotationTimes = 0, const FIntVector& InPosOffset = FIntVector(0) )
		: RotationTimes(InRotationTimes), PosOffset(InPosOffset)
	{}
	uint8 RotationTimes = 0;
	FIntVector PosOffset = FIntVector(0);
};

UCLASS()
class TILEDLEVELRUNTIME_API UAutoPaintItemRule : public UObject
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category="Rule")
	EImpactSize ImpactSize = EImpactSize::Three;

	UPROPERTY(EditAnywhere, Category="Rule")
	bool bIncludeZ = false;

	UPROPERTY(EditAnywhere, Category="Rule")
	TMap<FIntVector, FAdjacencyRule> AdjacencyRules;

	UPROPERTY(EditAnywhere, Category="Rule")
	FName OutOfBoundaryRule = "Empty";

	UPROPERTY(EditAnywhere, Category="Rule")
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, Category="Rule")
	float Probability = 1.f;

	UPROPERTY(EditAnywhere, Category="Rule")
	bool bStopOnMet = true;

	// Automatically add extra duplicated mirrored or rotated versions of adjacency rule
	UPROPERTY(EditAnywhere, Category="Rule")
	ERuleDuplication RuleDuplication = ERuleDuplication::No;

	UPROPERTY(EditAnywhere, Category="Spawn")
	EPlacedType PlacedType = EPlacedType::Block;

	UPROPERTY(EditAnywhere, Category="Spawn")
	FIntVector Extent = FIntVector(1, 1, 1);
	
	UPROPERTY(EditAnywhere, Category="Spawn")
	EEdgeType EdgeType = EEdgeType::Horizontal; // edge type is required!!! and restrict rotation possibility for spawn detail
	
	// prop items may stack on other items...
	UPROPERTY(EditAnywhere, Category="Spawn")
	ETLStructureType StructureType = ETLStructureType::Structure;
	
	UPROPERTY(EditAnywhere, Category="Spawn")
	FIntVector PositionOffset = FIntVector(0, 0, 0);

	// TODO: Z = 0 will crash...
	// limit this rule to apply on every X, Y, Z steps
	UPROPERTY(EditAnywhere, Category="StepSize")
	FIntVector StepSize = FIntVector(1);

	// this rule will apply on every step size + step offset grids.
	UPROPERTY(EditAnywhere, Category="StepSize")
	FIntVector StepOffset = FIntVector(0);

	// randomly pick one of the items here to spawn if met
	UPROPERTY(EditAnywhere, Category="Spawn")
	TMap<FTiledItemObj, FAutoPaintSpawnAdjustment> AutoPaintItemSpawns;

	UPROPERTY()
	FGuid RuleId = FGuid::NewGuid();

	UPROPERTY()
	FText Note;

	int CurrentEditImpactFloor = 0; // -2 ~ 2

	void UpdateImpactSize(EImpactSize NewImpactSize);
	
	FTiledItemObj PickWhatToSpawn() const;
	
	TArray<EDuplicationType> GetDuplicateCases() const;

	// deep copy?
	void DeepCopy(const UAutoPaintItemRule& Other)
	{
		ImpactSize = Other.ImpactSize;
		bIncludeZ = Other.bIncludeZ;
		AdjacencyRules = Other.AdjacencyRules;
		OutOfBoundaryRule = Other.OutOfBoundaryRule;
		bEnabled = Other.bEnabled;
		Probability = Other.Probability;
		bStopOnMet = Other.bStopOnMet;
		RuleDuplication = Other.RuleDuplication;
		PlacedType = Other.PlacedType;
		Extent = Other.Extent;
		EdgeType = Other.EdgeType;
		StructureType = Other.StructureType;
		PositionOffset = Other.PositionOffset;
		StepSize = Other.StepSize;
		StepOffset = Other.StepOffset;
		AutoPaintItemSpawns = Other.AutoPaintItemSpawns;
		CurrentEditImpactFloor = Other.CurrentEditImpactFloor;
		RuleId = FGuid::NewGuid();
		Note = Other.Note;
	}
	
#if WITH_EDITOR	
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

};

USTRUCT()
struct FAutoPaintRuleGroup
{
	GENERATED_BODY()

	FAutoPaintRuleGroup() {};

	FAutoPaintRuleGroup(const FName& NewName)
		: GroupName(NewName)
	{}
	
	UPROPERTY(EditAnywhere, Category="Group")
	FName GroupName;

	UPROPERTY(EditAnywhere, Category="Group")
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, Category="Group")
	TArray<UAutoPaintItemRule*> Rules;

	UPROPERTY()
	FText Note;

	bool operator==(const FAutoPaintRuleGroup& Other) const
	{
		return GroupName == Other.GroupName;
	}
	
	bool operator==(const FName& Other) const
	{
		return GroupName == Other;
	}
};



namespace EAutoPaintEditorSelectionState
{
	enum Type
	{
		None,
		Item,
		Group,
		Rule
	};
}



struct FAutoPaintMatchData
{
	FAutoPaintMatchData() = default;

	FIntVector MetPos;
	EDuplicationType DupType = EDuplicationType::No;
	FGuid ToPaint = FGuid();
	FIntVector PosOffset;
	// What to spawn is picked during this struct is generated!
	uint8 RotateTimes;
	FAutoPaintSpawnAdjustment* SpawnAdj = nullptr;
	UAutoPaintItemRule* BaseRulePtr;

};

struct FAutoPaintPlacement
{
	virtual ~FAutoPaintPlacement() = default;

	FAutoPaintPlacement(uint8 InRotationTimes, const FTransform& InAdditionalTransform, bool InIsMirrorX, bool InIsMirrorY, bool InIsMirrorZ)
		:RotationTimes(InRotationTimes), AdditionalTransform(InAdditionalTransform), IsMirrorX(InIsMirrorX), IsMirrorY(InIsMirrorY), IsMirrorZ(InIsMirrorZ)
	{}
	virtual FItemPlacement* GetPlacement() = 0;
	uint8 RotationTimes;
	FTransform AdditionalTransform;
	bool IsMirrorX;
	bool IsMirrorY;
	bool IsMirrorZ;
};


struct FAutoPaintPlacement_Tile :  FAutoPaintPlacement
{
	FAutoPaintPlacement_Tile(const FTilePlacement& InPlacement, uint8 InRotationTimes, const FTransform& InAdditionalTransform,
		bool InIsMirrorX, bool InIsMirrorY, bool InIsMirrorZ)
		:FAutoPaintPlacement(InRotationTimes, InAdditionalTransform, InIsMirrorX, InIsMirrorY, InIsMirrorZ), TilePlacement(InPlacement)
	{}
	FTilePlacement TilePlacement;
	virtual FItemPlacement* GetPlacement() override
	{
		return &TilePlacement;
	}
	bool operator<(const FAutoPaintPlacement_Tile& Other) const
	{
		return TilePlacement < Other.TilePlacement;		
	}
};

struct FAutoPaintPlacement_Edge : FAutoPaintPlacement
{
	FAutoPaintPlacement_Edge(const FEdgePlacement& InPlacement, uint8 InRotationTimes, const FTransform& InAdditionalTransform,
		bool InIsMirrorX, bool InIsMirrorY, bool InIsMirrorZ)
		:FAutoPaintPlacement(InRotationTimes, InAdditionalTransform, InIsMirrorX, InIsMirrorY, InIsMirrorZ), EdgePlacement(InPlacement)
	{}
	FEdgePlacement EdgePlacement;
	virtual FItemPlacement* GetPlacement() override
	{
		return &EdgePlacement;
	}
	bool operator<(const FAutoPaintPlacement_Edge& Other) const
	{
		return EdgePlacement < Other.EdgePlacement;		
	}
};

struct FAutoPaintPlacement_Point : FAutoPaintPlacement
{
	FAutoPaintPlacement_Point(const FPointPlacement& InPlacement, uint8 InRotationTimes, const FTransform& InAdditionalTransform,
		bool InIsMirrorX, bool InIsMirrorY, bool InIsMirrorZ)
		:FAutoPaintPlacement(InRotationTimes, InAdditionalTransform, InIsMirrorX, InIsMirrorY, InIsMirrorZ), PointPlacement(InPlacement)
	{}
	FPointPlacement PointPlacement;
	virtual FItemPlacement* GetPlacement() override
	{
		return &PointPlacement;	
	}
	bool operator<(const FAutoPaintPlacement_Point& Other) const
	{
		return PointPlacement < Other.PointPlacement;		
	}
};


DECLARE_DELEGATE(FAutoPaintRulePostEdit);
DECLARE_DELEGATE_OneParam(FRequestForUpdateMatchHint, UAutoPaintItemRule*);
DECLARE_DELEGATE_OneParam(FRequestForTransaction, FText);
DECLARE_DELEGATE_OneParam(FClearItemData, FName);

/**
 * 
 */
UCLASS()
class TILEDLEVELRUNTIME_API UAutoPaintRule : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category="Items")
	TArray<FAutoPaintItem> Items;

	UPROPERTY(EditAnywhere, Category="Rules")
	TArray<FAutoPaintRuleGroup> Groups = {FAutoPaintRuleGroup("Default")};

	TArray<UAutoPaintItemRule*> GetEnabledRuleList();
	
	void ReorderItem(const FName& ItemName1, const FName& ItemName2);

	static bool IsRuleMet(UAutoPaintItemRule* QueryRule, EDuplicationType DupCase, FIntVector QueryPosition, const TMap<FIntVector, FName>& ExistingData,
		FIntVector MinBoundary, FIntVector MaxBoundary);

	static FIntVector RotatePosToDuplicated(const FIntVector& InPos, EDuplicationType DupType);

	static FDuplicationMod GetDuplicationMod(EDuplicationType DupType, EPlacedShapeType WhichShape, uint8 RotTimes, FIntVector Pos);
	
	UPROPERTY()
	bool UseRandomSeed = false;

	UPROPERTY()
	int RandomSeed = 0;

	// used in runtime... grab the item set from tiled level asset!
	UTiledItemSet* GetItemSet();
	
	FName ActiveItemName = FName("Empty");

	TArray<FAutoPaintMatchData> MatchData;

	TMap<UAutoPaintItemRule*, TArray<FIntVector>> GetMatchedPositions();
	
	FAutoPaintRulePostEdit RefreshAutoPalette_Delegate;
	FAutoPaintRulePostEdit RefreshRuleEditor_Delegate;
	FAutoPaintRulePostEdit RefreshSpawnedInstance_Delegate;
	FRequestForUpdateMatchHint RequestForUpdateMatchHint;
	FRequestForTransaction RequestForTransaction;
	FClearItemData ClearItemDataInFloor;
	FClearItemData EmptyItemData;
	
	UPROPERTY()
	TObjectPtr<class UTiledItemSet> SourceItemSet; // used in auto paint editor only...
	
#if WITH_EDITORONLY_DATA
	
	UPROPERTY(EditAnywhere, Category="Editor", DisplayName="Number of X Tiles",  meta=(UIMin=1, UIMax=64, ClampMin=1, ClampMax=64))
	uint8 X_Num = 10;
	
	UPROPERTY(EditAnywhere, Category="Editor", DisplayName="Number of Y Tiles", meta=(UIMin=1, UIMax=64, ClampMin=1, ClampMax=64))
	uint8 Y_Num = 10;
	
	UPROPERTY(EditAnywhere, Category="Editor", DisplayName="Number of Floors")
	uint8 Floor_Num = 3;
	
	UPROPERTY(EditAnywhere, Category="Editor", DisplayName="Start Floor")
	int StartFloor = 0;

	UPROPERTY(EditAnywhere, Category="Editor")
	TMap<FIntVector, FName> PreviewData;

	EAutoPaintEditorSelectionState::Type SelectionState = EAutoPaintEditorSelectionState::None;
	virtual void PostEditUndo() override;
	
	UPROPERTY()
	float HintOpacity = 0.5f;
	
	UPROPERTY()
	bool bShowHint = true;
	
	UPROPERTY()
	TArray<FName> OpenedGroups;

#endif
};
