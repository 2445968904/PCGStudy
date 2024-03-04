// Copyright 2022 PufStudio. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "TiledLevelTypes.h"
#include "EdMode.h"

class AAutoPaintHelper;
class UTiledLevelItem;
struct FTiledLevelEdge;
class ATiledLevelEditorHelper;
class ATiledLevelSelectHelper;
class ATiledLevel;
class UTiledLevelLayer;
class UTiledLevelAsset;

namespace ETiledLevelEditTool
{
	enum Type
	{
		Select,
		Paint,
		Eraser,
		Eyedropper,
		Fill,
	};
}



namespace ETiledLevelBrushAction 
{
	enum Type
	{
		None,
		FreePaint,
		QuickErase,
		Erase,
		BoxSelect,
		PaintSelected,
	};
}

namespace ETiledLevelPaintMode
{
	enum Type
	{
		Normal,
		Auto
	};
}

namespace ETiledLevelEditViewMode
{
	enum Type
	{
		// The most common way...
		TopDown,
		// For metroidvania game
		Side,
		
	};
}

class FTiledLevelEdMode : public FEdMode
{
public:
	const static FEditorModeID EM_TiledLevelEdModeId;
	FTiledLevelEdMode();
	virtual ~FTiledLevelEdMode();

	// FEdMode interface 扩展编辑器的接口
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override { return "FTiledLevelEdMode"; }
	virtual void Enter() override;//顺序是Enter -> Init ->SNewToolkitTool
	virtual void Exit() override;
	virtual void Tick(FEditorViewportClient* ViewportClient, float DeltaTime) override;
	virtual void PostUndo() override;
	virtual bool MouseEnter(FEditorViewportClient* InViewportClient, FViewport* InViewport, int32 InMouseX, int32 InMouseY) override;
	virtual bool MouseLeave(FEditorViewportClient* InViewportClient, FViewport* InViewport) override;
	virtual bool MouseMove(FEditorViewportClient* InViewportClient, FViewport* InViewport, int32 InMouseX, int32 InMouseY) override;
	virtual bool CapturedMouseMove(FEditorViewportClient* InViewportClient, FViewport* InViewport, int32 InMouseX,
		int32 InMouseY) override;
	virtual bool InputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key,
		EInputEvent Event) override;
	virtual void DrawHUD(FEditorViewportClient* ViewportClient, FViewport* Viewport, const FSceneView* View,
		FCanvas* Canvas) override;
	virtual bool Select(AActor* InActor, bool bInSelected) override;
	virtual bool IsSelectionAllowed(AActor* InActor, bool bInSelection) const override;
	bool UsesToolkits() const override;
	virtual bool AllowWidgetMove() override { return  false; }
	virtual bool ShouldDrawWidget() const override { return false; }
	virtual bool UsesTransformWidget() const override { return false; }
	virtual bool DisallowMouseDeltaTracking() const override;
	// End of FEdMode interface
	
	// TODO: use an enum variable instead of two boolean?
	void SetupData(UTiledLevelAsset* InData, ATiledLevel* InLevel, ATiledLevelEditorHelper* InHelper,
		const UStaticMeshComponent* InPreviewSceneFloor, bool InIsInLevelAssetEditor, bool InIsInRuleEditor);
	void ClearActiveItemSet();
	void ClearItemInstances(UTiledLevelItem* TargetItem);
	void ClearSelectedItemInstances();
	bool CanClearSelectedItemInstancesInActiveFloor() const;
	void ClearSelectedItemInstancesInActiveFloor();
	ATiledLevelEditorHelper* GetEditorHelper() const { return Helper; }
	bool IsReadyToEdit() const;
	ATiledLevel* GetActiveLevelActor() const { return ActiveLevel; }
	TSharedPtr<class STiledPalette> GetPalettePtr();
	TSharedPtr<class SAutoPaintItemPalette> GetAutoPalettePtr();
	
	TSharedPtr<class STiledFloorList> FloorListWidget;
	UTiledLevelItem* ActiveItem = nullptr;
	TArray<UTiledLevelItem*> SelectedItems;
	UTiledLevelAsset* ActiveAsset = nullptr;
	EPlacedType EraserTarget = EPlacedType::Any;
	ETiledLevelEditTool::Type ActiveEditTool = ETiledLevelEditTool::Paint;
	FIntVector EraserExtent{1, 1, 1};
	
	ETiledLevelEditViewMode::Type ActiveViewMode = ETiledLevelEditViewMode::TopDown;
	ETiledLevelPaintMode::Type PaintMode = ETiledLevelPaintMode::Normal;

    // selection tool params
    int FloorSelectCount = 1;
    bool bSelectAllFloors = false;
    
	// fill tool params
	bool IsFillTiles = true;
	bool IsTileAsFillBoundary = true;
	bool NeedGround = false;
	bool EnableFillGap = false;
	float GapCoefficient = 1.0f;

	// auto paint params

	void DisplayAutoPaintHint() const;
	void UpdateAutoPaintHintOpacity() const;
	void ConvertAutoPaintToNormal();
	FName GetActiveAutoPaintName() const;
	FLinearColor GetActiveAutoPaintColor() const;
	bool GetAutoPaintIsEnableRandomSeed() const;
	void ToggleAutoPaintRandomSeed() const;
	int GetAutoPaintRandomSeed() const;
	void SetAutoPaintRandomSeed(int NewSeed) const;
	void ToggleAutoPaintVisibility();
	bool GetShowAutoPaintHint();
	void CreateAutoPaintPlacements(); // apply the transform from origin adjustment, rotation, gird actual position, and ... to each placement
	void UpdateAutoPaintPlacements();

	// where this edmode is triggered
	bool IsInLevelAssetEditor = false; // in level asset editor?
	bool IsInRuleAssetEditor = false; // in  rule asset editor?
	bool IsInAssetEditor() const
	{
		return IsInLevelAssetEditor || IsInRuleAssetEditor;		
	}
	// statistics
	TMap<int, uint32> PerFloorInstanceCount;
	TMap<FGuid, uint32> PerItemInstanceCount;
	uint32 TotalInstanceCount = 0;


private:

	void BindCommands();
	bool ValidCheck(); // Check core ptrs are correctly setup


	// Settings 
	// Save preference vars to EditorPerProjectIni, empty for now??
	void SavePreference();
	void LoadPreference();
	
	//Toolbar functions
	void SwitchToNewTool(ETiledLevelEditTool::Type NewTool);
	bool IsTargetToolChecked(ETiledLevelEditTool::Type TargetTool) const;
	void MirrorItem(EAxis::Type MirrorAxis);
	bool CanMirrorItem() const;
	void ToggleGrid();
	void RotateItem(bool IsClockWise = true);
	bool CanRotateItem() const;
	void EditAnotherFloor(bool IsUpper = true);
	void ToggleMultiMode();
	void ToggleSnap();
	bool CanToggleSnap() const;
	bool IsEnableSnapChecked() const;
	void ToggleAutoPaint();
	void SetPaintMode(ETiledLevelPaintMode::Type NewMode);
	bool IsAutoPaintChecked() const;
	
	// void RespawnAutoPaintInstances() const;
	void ToggleEditViewMode();
	bool IsEditViewModeSide() const;
	
	//Brush Functions
	void ApplyCursor(void* Cursor);
	void SetupBrush();
	void SetupBrushCursorOnly();
	void MouseMovement_Internal(class FEditorViewportClient* ViewportClient, const int32& InMouseX, const int32& InMouseY);
	// the the world position under mouse which intersect with active floor plane
	FVector GetMouseLocation(FEditorViewportClient* ViewportClient, int32 InMouseX, int32 InMouseY, FVector& OutTraceStart, FVector& OutTraceEnd);
	FVector2D GetMouse2DLocation(FEditorViewportClient* ViewportClient, int32 InMouseX, int32 InMouseY);
	void UpdateGirdLocation(FEditorViewportClient* ViewportClient, int32 InMouseX, int32 InMouseY);
	void UpdateEyedropperTarget(FEditorViewportClient* ViewportClient, int32 InMouseX, int32 InMouseY);
	void EnterNewGrid_Fill();
	void UpdateIsInsideValidArea_Grid();
	void EnterNewGrid(FIntVector NewTilePosition); // also include point
	void EnterNewEdge(FTiledLevelEdge NewEdge);
	void SelectionStart(FEditorViewportClient* ViewportClient);
	void SelectionEnd(FEditorViewportClient* ViewportClient);
	void SelectionCanceled();
	void EvaluateBoxSelection(FVector2D StartPos, FVector2D EndPos);
	void PaintSelectedStart(bool bForTemplate = false); 
	void PaintSelectedEnd();
	void PaintSelectedInstances(bool bForTemplate = false);
	void PaintStart();
	void PaintEnd();
	void PaintItemInstance();
	bool PaintItemPreparation();
	void QuickErase();
	void EraserStart();
	void EraserEnd();
	void EraseItems();
	void PerformEyedropper();
	void PerformFillTile();
	void PerformFillEdge();
	void SetupFillBoardFromTiles();
	void UpdateFillBoardFromGround();
	template <typename T>
	void CreateEmptyGaps(TArray<T>& InCandidatePoints, float Ratio); // randomly remove candidate points
	
	void UpdateStatistics();
	int32 GetNumEraserActiveItems() const;
	UTiledLevelItem* FindItemFromMeshPtr(UStaticMesh* MeshPtr);
	void ClearAutoData(FName ItemName, bool IsSelectedFloorOnly );
	void ChangeActiveAutoPaintItem();

	// Core params

	ATiledLevelEditorHelper* Helper = nullptr;
	ATiledLevel* ActiveLevel = nullptr;
	ATiledLevelSelectHelper* SelectionHelper = nullptr;
	AAutoPaintHelper* AutoPaintHelper = nullptr;
	TSharedPtr<FUICommandList> UICommandList;
	const UStaticMeshComponent* PreviewSceneFloor = nullptr; // for fixing line trace hit on it!
	// TODO: can't not handle scale issue when edit on level, I just cache and force it be FVector(1) as a temporary solution
	FVector CachedActiveLevelScale = FVector(1); 
	
	// Brush control params
	bool IsBrushSetup = false;
	EPlacedShapeType CurrentEditShape = EPlacedShapeType::Shape3D; // either editing tile or edge (Tile VS Edge VS Point placement)
	EPlacedType ActivePlacedType = EPlacedType::Block;
	bool bStraightEdit = false; // make brush only move vertical or horizontal, active when ctl is pressed 
	bool ShouldRotateBrushExtent = false;
	ETiledLevelBrushAction::Type ActiveBrushAction = ETiledLevelBrushAction::None;
	FIntVector CurrentTilePosition{0, 0, 0};
	FIntVector FixedTiledPosition{0, 0, 0}; // use for directional paint
	TArray<FTilePlacement> LastPlacedTiles;
	TArray<FEdgePlacement> LastPlacedEdges;
	TArray<FPointPlacement> LastPlacedPoints;
	FTiledLevelEdge CurrentEdge;
	FTiledLevelEdge FixedEdge;
	bool IsMouseInsideCanvas = false;
	bool IsInsideValidArea = false;
	bool IsHelperVisible = true;
	bool IsMultiMode = false;
	bool bSnapEnabled = false;
	bool IsMirrored_X = false;
	bool IsMirrored_Y = false;
	bool IsMirrored_Z = false;
	UTiledLevelItem* EyedropperTarget = nullptr;
	FName EyedropperTarget_AutoPaint;
	TSet<UTiledLevelItem*> ExistingItems;
	void* CursorResource_Select = nullptr; 
	void* CursorResource_Pen = nullptr;
	void* CursorResource_Eraser = nullptr;
	void* CursorResource_Eyedropper = nullptr;
	void* CursorResource_Fill = nullptr;
	uint8 CachedBrushRotationIndex = 0;
	bool HasPaintAnything = false;
	bool HasEraseAnything = false;
	
	// selection related params
	bool IsInstancesSelected = false;
	FIntPoint SelectionBeginMousePos; // for draw hud
	FVector2D SelectionBeginWorldPos; // for actual implement copy...
	TArray<bool> CachedFloorsVisibility;

	// Fill tool params
	TArray<TArray<int>> Board;
	TArray<FIntPoint> CandidateFillTiles;
	TArray<FTiledLevelEdge> CandidateFillEdges;
	int MaxZInFillItems = 1;

	// Auto paint params

	// Input Params
	bool bIsAltDown = false;
	bool bIsShiftDown = false;
	bool bIsControlDown = false;

};
