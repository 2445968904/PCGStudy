// Copyright 2022 PufStudio. All Rights Reserved.

#include "TiledLevelEdMode.h"

#include "AutoPaintHelper.h"
#include "AutoPaintRule.h"
#include "TiledLevelEdModeToolkit.h"
#include "STiledFloorList.h"
#include "STiledPalette.h"
#include "TiledLevel.h"
#include "TiledLevelAsset.h"
#include "TiledLevelCommands.h"
#include "TiledLevelEditorHelper.h"
#include "TiledLevelEditorLog.h"
#include "TiledLevelItem.h"
#include "TiledLevelSelectHelper.h"
#include "TiledLevelSettings.h"
#include "TiledLevelUtility.h"

#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "Toolkits/ToolkitManager.h"
#include "EditorModeManager.h"
#include "ProceduralMeshComponent.h"
#include "SAutoPaintItemPalette.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Interfaces/IPluginManager.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetGuidLibrary.h"

const FEditorModeID FTiledLevelEdMode::EM_TiledLevelEdModeId = TEXT("EM_TiledLevelEdMode");

#define LOCTEXT_NAMESPACE "TiledLevel"

FTiledLevelEdMode::FTiledLevelEdMode()
    : FEdMode()
{
}

FTiledLevelEdMode::~FTiledLevelEdMode()
{
}

void FTiledLevelEdMode::AddReferencedObjects(FReferenceCollector& Collector)
{
    FEdMode::AddReferencedObjects(Collector);
}

void FTiledLevelEdMode::Enter()
{
    FEdMode::Enter();
    SelectNone();
    
    if (!Toolkit.IsValid() && UsesToolkits())
    {
        Toolkit = MakeShareable(new FTiledLevelEdModeToolkit(this));
        Toolkit->Init(Owner->GetToolkitHost());
        UICommandList = Toolkit->GetToolkitCommands();
        BindCommands();
    }
    CurrentTilePosition = FIntVector(0);

    CursorResource_Select = FSlateApplication::Get().GetPlatformCursor()->CreateCursorFromFile(
        IPluginManager::Get().FindPlugin(TEXT("TiledLevel"))->GetBaseDir() / TEXT("Resources") / TEXT("Select"), FVector2D(0, 0));
    CursorResource_Pen = FSlateApplication::Get().GetPlatformCursor()->CreateCursorFromFile(
        IPluginManager::Get().FindPlugin(TEXT("TiledLevel"))->GetBaseDir() / TEXT("Resources") / TEXT("Pen"), FVector2D(0, 0));
    CursorResource_Eraser = FSlateApplication::Get().GetPlatformCursor()->CreateCursorFromFile(
        IPluginManager::Get().FindPlugin(TEXT("TiledLevel"))->GetBaseDir() / TEXT("Resources") / TEXT("Eraser"), FVector2D(0, 0));
    CursorResource_Eyedropper = FSlateApplication::Get().GetPlatformCursor()->CreateCursorFromFile(
        IPluginManager::Get().FindPlugin(TEXT("TiledLevel"))->GetBaseDir() / TEXT("Resources") / TEXT("Drop"), FVector2D(0, 0));
    CursorResource_Fill = FSlateApplication::Get().GetPlatformCursor()->CreateCursorFromFile(
        IPluginManager::Get().FindPlugin(TEXT("TiledLevel"))->GetBaseDir() / TEXT("Resources") / TEXT("Fill"), FVector2D(0, 0));
    
    LoadPreference();
}

void FTiledLevelEdMode::Exit()
{
    if (Toolkit.IsValid())
    {
        if (IsInRuleAssetEditor)
        {
            ActiveAsset->GetAutoPaintRule()->PreviewData = ActiveAsset->GetAutoPaintData();
        }
        if (!IsInAssetEditor())
        {
            if (Helper)
            {
                Helper->Destroy();
            }
            if (SelectionHelper)
            {
                SelectionCanceled();
                SelectionHelper->Destroy();
            }
            if (AutoPaintHelper)
            {
                AutoPaintHelper->Destroy();
            }
            FSlateApplication::Get().GetPlatformCursor()->SetTypeShape(EMouseCursor::Default, nullptr);
            FSlateApplication::Get().GetPlatformCursor()->SetTypeShape(EMouseCursor::Crosshairs, nullptr);
        }
        FToolkitManager::Get().CloseToolkit(Toolkit.ToSharedRef());
        Toolkit.Reset();
    }
    ActiveAsset->CanEditTileNum = false;
    ActiveAsset->OnTiledLevelAreaChanged.Clear();
    if (ActiveAsset->GetAutoPaintRule())
    {
        ActiveAsset->GetAutoPaintRule()->Items.RemoveAll([](const FAutoPaintItem& Item)
        {
            return Item.ItemName == FName("Empty");
        });
        ActiveAsset->GetAutoPaintRule()->RefreshSpawnedInstance_Delegate.Unbind();
        ActiveAsset->GetAutoPaintRule()->RequestForUpdateMatchHint.Unbind();
        ActiveAsset->GetAutoPaintRule()->RequestForTransaction.Unbind();
    }
    if (ActiveLevel)
    {
        ActiveLevel->SetActorScale3D(CachedActiveLevelScale);
    }
    ActiveLevel = nullptr;


    
    GEditor->SelectNone(false, true);
    GEditor->RedrawLevelEditingViewports();
    SavePreference();

    // Call base Exit method to ensure proper cleanup
    FEdMode::Exit();
}

void FTiledLevelEdMode::Tick(FEditorViewportClient* ViewportClient, float DeltaTime)
{
    FEdMode::Tick(ViewportClient, DeltaTime);
}

void FTiledLevelEdMode::PostUndo()
{
    if (ActiveLevel)
    {
        ActiveLevel->ResetAllInstance(true);
    }
    if (AutoPaintHelper)
    {
        if (PaintMode == ETiledLevelPaintMode::Auto)
            DisplayAutoPaintHint();
    }
    FEdMode::PostUndo();
}

bool FTiledLevelEdMode::MouseEnter(FEditorViewportClient* InViewportClient, FViewport* InViewport, int32 InMouseX, int32 InMouseY)
{
    if (!ValidCheck())
    {
        return false;
    }
    
    IsMouseInsideCanvas = true;
    LastPlacedTiles.Empty();
    LastPlacedEdges.Empty();
    LastPlacedPoints.Empty();
    
    if (!ActiveAsset->GetActiveFloor()->ShouldRenderInEditor)
    {
        SetupBrushCursorOnly();
        return true;
    }
    
    if (ActiveAsset->VersionNumber > ActiveLevel->VersionNumber + 100)
    {
        ActiveLevel->ResetAllInstance(false);
    }
    SetupBrush();
    if (IsBrushSetup)
        UpdateGirdLocation(InViewportClient, InMouseX, InMouseY);
    if (ActiveEditTool == ETiledLevelEditTool::Select || ActiveEditTool == ETiledLevelEditTool::Eyedropper)
    {
        GetPalettePtr()->ClearItemSelection();
    }
    return false;
    
}

bool FTiledLevelEdMode::MouseLeave(FEditorViewportClient* InViewportClient, FViewport* InViewport)
{
    FSlateApplication::Get().GetPlatformCursor()->SetTypeShape(EMouseCursor::Default, nullptr);
    FSlateApplication::Get().GetPlatformCursor()->SetTypeShape(EMouseCursor::Crosshairs, nullptr);
    if (!ValidCheck())
    {
        return false;
    }
    if (ActiveEditTool == ETiledLevelEditTool::Select) SelectionHelper->EmptyHints();
    if (ActiveItem)
    {
        if (ActiveItem->IsA<UTiledLevelTemplateItem>())
            SelectionHelper->EmptyHints();
    }
    IsMouseInsideCanvas = false;
    CachedBrushRotationIndex = Helper->BrushRotationIndex;
    Helper->ResetBrush();
    ShouldRotateBrushExtent = false;
    IsBrushSetup = false;
    UpdateStatistics();
    
    return false;
}

bool FTiledLevelEdMode::MouseMove(FEditorViewportClient* InViewportClient, FViewport* InViewport, int32 InMouseX,
                                  int32 InMouseY)
{
    MouseMovement_Internal(InViewportClient, InMouseX, InMouseY);
    return false;
}

bool FTiledLevelEdMode::CapturedMouseMove(FEditorViewportClient* InViewportClient, FViewport* InViewport,
                                          int32 InMouseX, int32 InMouseY)
{
    MouseMovement_Internal(InViewportClient, InMouseX, InMouseY);
    return false;
}


bool FTiledLevelEdMode::InputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event)
{
    bIsShiftDown = ((Key == EKeys::LeftShift || Key == EKeys::RightShift) && Event != IE_Released) || Viewport->KeyState(EKeys::LeftShift) ||
        Viewport->KeyState(EKeys::RightShift);
    bIsAltDown = ((Key == EKeys::LeftAlt || Key == EKeys::RightAlt) && Event != IE_Released) || Viewport->KeyState(EKeys::LeftAlt) ||
        Viewport->KeyState(EKeys::RightAlt);
    bIsControlDown = ((Key == EKeys::LeftControl || Key == EKeys::RightControl) && Event != IE_Released) || Viewport->KeyState(EKeys::LeftControl) ||
        Viewport->KeyState(EKeys::RightControl);

    // prevent edit this actor during edit... an issue I've never though of...
    if (Key == EKeys::Delete || Key == EKeys::BackSpace || Key == EKeys::H)
    {
        return true;
    }

    
    if (Key == EKeys::Escape && Event == IE_Pressed)
    {
        GLevelEditorModeTools().ActivateDefaultMode();
        return true;
    }
    
    if (Event != IE_Released && ActiveBrushAction == ETiledLevelBrushAction::None)
    {
        if (UICommandList->ProcessCommandBindings(Key, FSlateApplication::Get().GetModifierKeys(), false))
        {
            return true;
        }
    }

    // block all mouse click if it's in hidden floor..
    if (Key== EKeys::LeftMouseButton || Key==EKeys::RightMouseButton)
    {
        if (!ActiveAsset->GetActiveFloor()->ShouldRenderInEditor) return false;
    }

    if ((Key == EKeys::LeftShift || Key == EKeys::RightShift) && ActiveEditTool == ETiledLevelEditTool::Paint && Event != IE_Repeat)
    {
        if (!ActiveItem) return true;
        if (ActiveItem && ActiveItem->IsA<UTiledLevelTemplateItem>()) return true;
        if (Event == IE_Pressed)
            Helper->ToggleQuickEraserBrush(true);
        else if (Event == IE_Released)
            Helper->ToggleQuickEraserBrush(false);
        return true;
    }

    // a short cut to start box select from paint tool
    if (Event == IE_Pressed && Key == EKeys::RightMouseButton && bIsShiftDown &&
        (ActiveEditTool == ETiledLevelEditTool::Paint || ActiveEditTool == ETiledLevelEditTool::Select))
    {
        SwitchToNewTool(ETiledLevelEditTool::Select);
        SelectionStart(ViewportClient);
        return false;
    }
    
    if (Key == EKeys::RightMouseButton && Event == IE_Released && ActiveEditTool == ETiledLevelEditTool::Select)
    {
        if (!IsInstancesSelected)
        {
            SelectionEnd(ViewportClient);
            return false;
        }
    }

    if (Event == IE_Pressed && Key == EKeys::LeftMouseButton && !bIsAltDown)
    {
        switch (ActiveEditTool)
        {
        case ETiledLevelEditTool::Select:
            if (!IsInstancesSelected)
            {
                if (bIsShiftDown)
                    SelectionStart(ViewportClient);
            } else
            {
                PaintSelectedStart();
            }
            break;
        case ETiledLevelEditTool::Paint:
            if (ActiveItem && ActiveItem->IsA<UTiledLevelTemplateItem>())
            {
                PaintSelectedStart(true);
            }
            else
            {
                PaintStart();
            }
            break;
        case ETiledLevelEditTool::Eraser:
            EraserStart();
            break;
        case ETiledLevelEditTool::Eyedropper:
            PerformEyedropper();
            break;
        case ETiledLevelEditTool::Fill:
            if (IsFillTiles)
            {
                PerformFillTile();
            }
            else
            {
                PerformFillEdge();
            }
            break;
        default: ;
        }
        return true;
    }

    if (Event == IE_Released && Key == EKeys::LeftMouseButton)
    {
        switch (ActiveEditTool)
        {
        case ETiledLevelEditTool::Select:
            if (!IsInstancesSelected)
            {
                SelectionEnd(ViewportClient);
            } else
            {
                PaintSelectedEnd();
            }
            break;
        case ETiledLevelEditTool::Paint:
            if (ActiveItem && ActiveItem->IsA<UTiledLevelTemplateItem>())
            {
                PaintSelectedEnd();
            }
            else
            {
                PaintEnd();
            }
            break;
        case ETiledLevelEditTool::Eraser:
            EraserEnd();
            break;
        default: ;
        }
        return true;
    }


    /*
     * TODO: for future update or ignore since this issue also exist in paper2D
     * Can not block the context menu of selected tiled level actor
     * RMB must return false... When RMB + mouse move, cursor will disappear, and if not let RMB return false here,
     * the disappeared cursor will not comeback!!
     */
    
    // if ((Event == IE_Released || Event == IE_DoubleClick) && Key == EKeys::RightMouseButton) // force block right click
    // {
    //     if (MouseIsCaptured)
    //     {
    //         MouseIsCaptured = false;
    //         return false;
    //     }
    //     return true;
    // }

    return false;
}

void FTiledLevelEdMode::DrawHUD(FEditorViewportClient* ViewportClient, FViewport* Viewport, const FSceneView* View,
                                FCanvas* Canvas)
{
    if (!ValidCheck()) return;
    const FIntRect CanvasRect = Canvas->GetViewRect();
    // The reference implementation in paper2D did not handle DPI issue! Ha Ha, this is where I am better than Epic Games!
    const float DPIScale = ViewportClient->GetDPIScale();
    if (GetModeManager() == &GLevelEditorModeTools())
    {
        const FString EdModeHelp = TEXT("Editing tiled level ...  Press ESCAPE to exit.");
        int32 XL, YL;
        StringSize(GEngine->GetSubtitleFont(), XL, YL, *EdModeHelp);
        const float DrawX = FMath::FloorToFloat((CanvasRect.Min.X + (CanvasRect.Width() - XL) * 0.5) / DPIScale);
        const float DrawY = FMath::FloorToFloat(30 / DPIScale);
        Canvas->DrawShadowedString(DrawX, DrawY, *EdModeHelp, GEngine->GetSubtitleFont(), FLinearColor::White);
    }

    // Draw mouse follow text
    static int GapToCursor = 20;
    if (IsMouseInsideCanvas)
    {
        FString ToolDescriptionString = "";
        FLinearColor ToolPromptColor = FLinearColor::White;
        UFont* ToolFont = nullptr;
        FIntPoint Pos;
        Viewport->GetMousePos(Pos);
        FVector WorldOrigin;
        FVector WorldDirection;
        View->DeprojectFVector2D(Pos, WorldOrigin, WorldDirection);
        Pos.X = Pos.X / DPIScale;
        Pos.Y = Pos.Y / DPIScale;
        if (!ActiveAsset->GetActiveFloor()->ShouldRenderInEditor)
        {
            ToolDescriptionString = TEXT("Hidden floor");
            Canvas->DrawShadowedString(Pos.X, Pos.Y, *ToolDescriptionString, GEngine->GetMediumFont(), FColor::Red);
        }
        else if (ActiveEditTool == ETiledLevelEditTool::Eyedropper)
        {
            if (EyedropperTarget || !EyedropperTarget_AutoPaint.IsNone())
            {
                if (EyedropperTarget)
                    ToolDescriptionString = *EyedropperTarget->GetItemName();
                else
                {
                    ToolDescriptionString = *EyedropperTarget_AutoPaint.ToString();
                }
                ToolPromptColor = FColorList::White;
                ToolFont = GEngine->GetLargeFont();
                int EXL, EYL;
                // TODO: give it a little gap to cursor
                StringSize(GEngine->GetLargeFont(), EXL, EYL, *ToolDescriptionString);
                Canvas->DrawTile(Pos.X - 10 + GapToCursor, Pos.Y -10 + GapToCursor, EXL + 20, EYL + 20, 0, 0, 1, 1, FLinearColor(0.10, 0.10, 0.10));
                Canvas->DrawShadowedString(Pos.X + GapToCursor, Pos.Y + GapToCursor, *ToolDescriptionString, ToolFont, ToolPromptColor);
            }
        }
        else if (ActiveEditTool == ETiledLevelEditTool::Select && ActiveBrushAction == ETiledLevelBrushAction::BoxSelect)
        {
            Canvas->DrawTile(SelectionBeginMousePos.X, SelectionBeginMousePos.Y, Pos.X - SelectionBeginMousePos.X, Pos.Y - SelectionBeginMousePos.Y, 0, 0, 1, 1, FLinearColor(0.8, 0.8, 0.2, 0.1));
            FCanvasLineItem L1(SelectionBeginMousePos, FIntPoint(SelectionBeginMousePos.X , Pos.Y));
            FCanvasLineItem L2(SelectionBeginMousePos, FIntPoint(Pos.X , SelectionBeginMousePos.Y));
            FCanvasLineItem L3( FIntPoint(SelectionBeginMousePos.X , Pos.Y), Pos);
            FCanvasLineItem L4( FIntPoint(Pos.X , SelectionBeginMousePos.Y), Pos);
            L1.LineThickness = 1;
            L2.LineThickness = 1;
            L3.LineThickness = 1;
            L4.LineThickness = 1;
            L1.SetColor(FLinearColor(0.8, 0.8, 0.2, 1));
            L2.SetColor(FLinearColor(0.8, 0.8, 0.2, 1));
            L3.SetColor(FLinearColor(0.8, 0.8, 0.2, 1));
            L4.SetColor(FLinearColor(0.8, 0.8, 0.2, 1));
            Canvas->DrawItem(L1);
            Canvas->DrawItem(L2);
            Canvas->DrawItem(L3);
            Canvas->DrawItem(L4);
        }
        else
        {
            FString ToolInfo = "";
            switch (ActiveEditTool) {
            case ETiledLevelEditTool::Select:
                ToolInfo = TEXT("Select");
                break;
            case ETiledLevelEditTool::Paint:
                if (PaintMode == ETiledLevelPaintMode::Normal)
                {
                    ToolInfo = ActiveItem == nullptr? TEXT("Not Selected") : *ActiveItem->GetItemName();
                    if (!IsInsideValidArea || ActiveItem == nullptr)
                        ToolPromptColor = FLinearColor::Red;
                }
                else
                {
                    ToolInfo = *GetActiveAutoPaintName().ToString();
                    if (!IsInsideValidArea || ToolInfo == TEXT("Empty"))
                        ToolPromptColor = FLinearColor::Red;
                }
                break;
            case ETiledLevelEditTool::Eraser:
                ToolInfo = TEXT("Eraser");
                break;
            case ETiledLevelEditTool::Fill: break;
                default: ;
            }
            const FString EdgeAlignmentString = CurrentEdge.EdgeType == EEdgeType::Vertical? TEXT("V") : TEXT("H");
            const FString MousePositionInfo =
                CurrentEditShape == EPlacedShapeType::Shape2D?
                    FString::Printf(TEXT("(%d, %d)-%s"), CurrentEdge.X, CurrentEdge.Y, *EdgeAlignmentString) :
                    FString::Printf(TEXT("(%d, %d)"), CurrentTilePosition.X, CurrentTilePosition.Y);
            ToolDescriptionString = *FString::Printf(TEXT("%s %s"), *MousePositionInfo, *ToolInfo);
            ToolFont = GEngine->GetMediumFont();
            Canvas->DrawShadowedString(Pos.X + GapToCursor, Pos.Y + GapToCursor, *ToolDescriptionString, ToolFont, ToolPromptColor);
        }
    }

    // Draw Bottom text
    {
        FString ToolString = TEXT("No tool selected");
        FString ItemDetail = TEXT("");
        switch (ActiveEditTool)
        {
        case ETiledLevelEditTool::Select: 
            ToolString = TEXT("Select");
            ItemDetail = IsValid(SelectionHelper)? *FString::Printf(TEXT("%d placements"), SelectionHelper->GetNumOfCopiedPlacements()) : TEXT("Error");
            break;
        case ETiledLevelEditTool::Paint:
            ToolString = bIsShiftDown? TEXT("Quick Erase") : TEXT("Paint");
            if (bIsControlDown) ToolString += TEXT(" (Straight)");
            ItemDetail = IsValid(ActiveItem)? *ActiveItem->GetItemNameWithInfo() : TEXT("Empty");
            break;
        case ETiledLevelEditTool::Eraser:
            ToolString = TEXT("Erase");
            if (bIsControlDown) ToolString += TEXT(" (Straight)");
            ItemDetail = *FString::Printf(TEXT("%d items"), GetNumEraserActiveItems());
            break;
        case ETiledLevelEditTool::Eyedropper:
            break;
        default: ;
        }
        if (ActiveAsset->GetActiveFloor())
        {
            const FString FloorString = ActiveAsset->GetActiveFloor()->FloorName.ToString();
            FString BottomInfo = FString::Printf(TEXT("%s [%s] in %s"), *ToolString, *ItemDetail, *FloorString);
            FLinearColor InfoColor = FLinearColor::White;
            if (!ActiveAsset->GetActiveFloor()->ShouldRenderInEditor)
            {
                BottomInfo = TEXT("Can not do anything in hidden floor!");
                InfoColor = FLinearColor::Red;
            }
            int32 XL;
            int32 YL;
            StringSize(GEngine->GetLargeFont(), XL, YL, *BottomInfo);

            const float DrawX = FMath::FloorToFloat((CanvasRect.Min.X + (CanvasRect.Width() - XL) * 0.5f) / DPIScale);
            const float DrawY = FMath::FloorToFloat((CanvasRect.Max.Y - 20.0f - YL) / DPIScale);
            Canvas->DrawShadowedString(DrawX, DrawY, *BottomInfo, GEngine->GetLargeFont(), InfoColor);
        }
        
    }
}

bool FTiledLevelEdMode::Select(AActor* InActor, bool bInSelected)
{
    return false;
}

bool FTiledLevelEdMode::IsSelectionAllowed(AActor* InActor, bool bInSelection) const
{
    return false;
}

bool FTiledLevelEdMode::UsesToolkits() const
{
    return true;
}

bool FTiledLevelEdMode::DisallowMouseDeltaTracking() const
{
    return ActiveBrushAction != ETiledLevelBrushAction::None;
}

void FTiledLevelEdMode::SetupData(UTiledLevelAsset* InData, ATiledLevel* InLevel, ATiledLevelEditorHelper* InHelper,
    const UStaticMeshComponent* InPreviewSceneFloor, bool InIsInLevelAssetEditor, bool InIsInRuleEditor)
{
    
    ActiveAsset = InData;
    ActiveAsset->HostLevel = InLevel;
    ActiveAsset->OnResetTileSize.BindRaw(this, &FTiledLevelEdMode::ClearActiveItemSet);
    ActiveAsset->RequestForUpdateAutoPaintPlacements.BindRaw(this, &FTiledLevelEdMode::UpdateAutoPaintPlacements);
    ActiveAsset->CanEditTileNum = true;
    ActiveAsset->ViewAsAutoPaint? PaintMode = ETiledLevelPaintMode::Auto: ETiledLevelPaintMode::Normal;
    IsInLevelAssetEditor = InIsInLevelAssetEditor;
    IsInRuleAssetEditor = InIsInRuleEditor;
    
    if (Toolkit)
    {
        StaticCastSharedPtr<FTiledLevelEdModeToolkit>(Toolkit)->LoadItemSet(ActiveAsset->GetItemSetAsset());
        StaticCastSharedPtr<FTiledLevelEdModeToolkit>(Toolkit)->LoadAutoPaintRule(ActiveAsset->GetAutoPaintRule());
        if (IsInRuleAssetEditor)
        {
            GetAutoPalettePtr()->OnPaintItemChanged.AddUObject(ActiveAsset, &UTiledLevelAsset::OnAutoPaintItemChanged);
            GetAutoPalettePtr()->OnPaintItemRemoved.AddUObject(ActiveAsset, &UTiledLevelAsset::OnAutoPaintItemRemoved);
            PaintMode = ETiledLevelPaintMode::Auto;
            ActiveAsset->GetAutoPaintRule()->Items.AddUnique(FAutoPaintItem(FName("Empty"), FLinearColor::White));
        }
        if (ActiveAsset->GetAutoPaintRule())
        {
            // make empty item disappear
            if (!IsInRuleAssetEditor)
            {
                ActiveAsset->GetAutoPaintRule()->Items.RemoveAll([](FAutoPaintItem Item)
                {
                    return Item.ItemName == FName("Empty");
                });
            }
            ActiveAsset->GetAutoPaintRule()->RefreshSpawnedInstance_Delegate.BindRaw(this, &FTiledLevelEdMode::UpdateAutoPaintPlacements);
            ActiveAsset->GetAutoPaintRule()->ClearItemDataInFloor.BindRaw(this, &FTiledLevelEdMode::ClearAutoData, true);
            ActiveAsset->GetAutoPaintRule()->EmptyItemData.BindRaw(this, &FTiledLevelEdMode::ClearAutoData, false);
            GetAutoPalettePtr()->UpdatePalette(true);
        }
    }
    
    ActiveLevel = InLevel;
    ActiveLevel->PreSaveTiledLevelActor.BindLambda([this]()
    {
        GLevelEditorModeTools().ActivateDefaultMode();
    });
    ActiveLevel->SetActiveAsset(ActiveAsset);
    CachedActiveLevelScale = ActiveLevel->GetActorScale();
    ActiveLevel->SetActorScale3D(FVector(1));
    // ActiveLevel->SetEnableUpdateAINavigation(false);

    const FVector TileSize = ActiveAsset->GetTileSize();
    Helper = InHelper;
    Helper->ResizeArea(TileSize.X, TileSize.Y, TileSize.Z, ActiveAsset->X_Num, ActiveAsset->Y_Num, ActiveAsset->TiledFloors.Num(), FMath::Max(ActiveAsset->TiledFloors).FloorPosition);
    Helper->SetActorTransform(ActiveLevel->GetActorTransform());
    Helper->AttachToActor(ActiveLevel, FAttachmentTransformRules::KeepWorldTransform);
    ActiveAsset->OnTiledLevelAreaChanged.AddUObject(Helper, &ATiledLevelEditorHelper::ResizeArea);
    ActiveAsset->MoveHelperFloorGrids.BindUObject(Helper, &ATiledLevelEditorHelper::MoveFloorGrids);
    SelectionHelper = Helper->GetWorld()->SpawnActor<ATiledLevelSelectHelper>();
    SelectionHelper->SetActorTransform(ActiveLevel->GetActorTransform());
    SelectionHelper->AttachToActor(ActiveLevel, FAttachmentTransformRules::KeepWorldTransform);
    SelectionHelper->Hide();

    PreviewSceneFloor = InPreviewSceneFloor;
    
    AutoPaintHelper = Helper->GetWorld()->SpawnActor<AAutoPaintHelper>();
    AutoPaintHelper->SetActorTransform(ActiveLevel->GetTransform());
    AutoPaintHelper->Init(ActiveAsset);

    CreateAutoPaintPlacements();
    SetPaintMode(PaintMode);
    ActiveLevel->ResetAllInstance(true);
    UpdateStatistics();
}

void FTiledLevelEdMode::ClearActiveItemSet()
{
    TSharedPtr<FTiledLevelEdModeToolkit> TiledLevelEdModeToolkit = StaticCastSharedPtr<FTiledLevelEdModeToolkit>(Toolkit);
    if (TiledLevelEdModeToolkit.IsValid())
        TiledLevelEdModeToolkit->LoadItemSet(nullptr);
    if (ActiveAsset) ActiveAsset->SetActiveItemSet(nullptr);
}

void FTiledLevelEdMode::ClearItemInstances(UTiledLevelItem* TargetItem)
{
    const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "TiledLevelMode_ClearItemTransaction", "Clear Item Instance"));
    ActiveAsset->Modify();
    ActiveAsset->ClearItem(TargetItem->ItemID);
    ActiveLevel->ResetAllInstance();
    UpdateStatistics();
}

void FTiledLevelEdMode::ClearSelectedItemInstances()
{
    bool bProceed = true;
    int N = SelectedItems.Num();
    if (SelectedItems.Num() > 1)
    {
        int T = 0;
        for (const UTiledLevelItem* Item : SelectedItems)
        {
            if (PerItemInstanceCount.Contains(Item->ItemID))
            {
                T += PerItemInstanceCount[Item->ItemID];
            } else
            {
                N -= 1;
            }
        }
        if (T >= 16 && N > 1) // there is no basis for when I should notify the users... I'll just set it 16?
        {
            const FText Message = FText::Format(NSLOCTEXT("UnrealEd", "TiledLevelEditor_ClearSelectedItemInstances", "Remove {0} items with totally {1} placements?"),
                                                FText::AsNumber(N), FText::AsNumber(T));
            bProceed = (FMessageDialog::Open(EAppMsgType::OkCancel, Message) == EAppReturnType::Ok);
        }
    }
    if (bProceed)
    {
        for (UTiledLevelItem* Item : SelectedItems)
            ClearItemInstances(Item);
    }
}

bool FTiledLevelEdMode::CanClearSelectedItemInstancesInActiveFloor() const
{
    for (auto P : ActiveAsset->GetActiveFloor()->GetItemPlacements())
    {
        for (UTiledLevelItem* Item : SelectedItems)
            if (Item->ItemID == P.ItemID) return true;
    }
    return false;
}

void FTiledLevelEdMode::ClearSelectedItemInstancesInActiveFloor()
{
    const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "TiledLevelMode_ClearItemTransaction", "Clear Item Instance"));
    ActiveAsset->Modify();
    for (UTiledLevelItem* Item : SelectedItems)
        ActiveAsset->ClearItemInActiveFloor(Item->ItemID);
    ActiveLevel->ResetAllInstance();
    UpdateStatistics();
}

bool FTiledLevelEdMode::IsReadyToEdit() const
{
    if (ActiveAsset)
    {
        return ActiveAsset->IsTileSizeConfirmed();
    }
    return false;
}

TSharedPtr<STiledPalette> FTiledLevelEdMode::GetPalettePtr()
{
    return StaticCastSharedPtr<FTiledLevelEdModeToolkit>(Toolkit)->ItemPalette;
}

TSharedPtr<SAutoPaintItemPalette> FTiledLevelEdMode::GetAutoPalettePtr()
{
    return StaticCastSharedPtr<FTiledLevelEdModeToolkit>(Toolkit)->AutoItemPalette;
}

void FTiledLevelEdMode::DisplayAutoPaintHint() const
{
    if (!ActiveAsset) return;
    if (!ActiveAsset->GetAutoPaintRule()) return;
    if (!AutoPaintHelper->HasInit())
    {
        AutoPaintHelper->Init(ActiveAsset);
    }
    if (ActiveAsset->GetAutoPaintRule()->bShowHint)
        AutoPaintHelper->UpdateVisual(ActiveAsset->GetAutoPaintRule()->HintOpacity);
    else
        AutoPaintHelper->ClearHint();
}

void FTiledLevelEdMode::UpdateAutoPaintHintOpacity() const
{
    if (!ActiveAsset) return;
    if (!ActiveAsset->GetAutoPaintRule()) return;
    AutoPaintHelper->UpdateOpacity(ActiveAsset->GetAutoPaintRule()->HintOpacity);
}

void FTiledLevelEdMode::ConvertAutoPaintToNormal()
{
    FScopedTransaction Transaction(LOCTEXT("ConvertAutoToNormal", "Convert auto paint to normal"));
    ActiveAsset->Modify();
    ActiveAsset->ConvertAutoPlacementsToNormal();
    SetPaintMode(ETiledLevelPaintMode::Normal);
}

FName FTiledLevelEdMode::GetActiveAutoPaintName() const
{
    if (const UAutoPaintRule* Rule = ActiveAsset->GetAutoPaintRule())
    {
        return Rule->ActiveItemName;
    }
    return FName("Empty");
}

FLinearColor FTiledLevelEdMode::GetActiveAutoPaintColor() const
{
    if (const UAutoPaintRule* Rule = ActiveAsset->GetAutoPaintRule())
    {
        if (const FAutoPaintItem* RuleItem = Rule->Items.FindByKey(Rule->ActiveItemName))
            return RuleItem->PreviewColor;
    }
    return FLinearColor::White;
}

bool FTiledLevelEdMode::GetAutoPaintIsEnableRandomSeed() const
{
    if (ActiveAsset && ActiveAsset->GetAutoPaintRule())
    {
        return ActiveAsset->GetAutoPaintRule()->UseRandomSeed;
    }
    return false;
}

void FTiledLevelEdMode::ToggleAutoPaintRandomSeed() const
{
    if (ActiveAsset && ActiveAsset->GetAutoPaintRule())
    {
        ActiveAsset->GetAutoPaintRule()->UseRandomSeed = !ActiveAsset->GetAutoPaintRule()->UseRandomSeed;
    }
}

int FTiledLevelEdMode::GetAutoPaintRandomSeed() const
{
    if (ActiveAsset && ActiveAsset->GetAutoPaintRule())
    {
        return ActiveAsset->GetAutoPaintRule()->RandomSeed;
    }
    return 0;
}

void FTiledLevelEdMode::SetAutoPaintRandomSeed(int NewSeed) const
{
    if (ActiveAsset && ActiveAsset->GetAutoPaintRule())
    {
        ActiveAsset->GetAutoPaintRule()->RandomSeed = NewSeed;
    }
}

void FTiledLevelEdMode::BindCommands()
{
    const FTiledLevelCommands& Command = FTiledLevelCommands::Get();
    UICommandList->MapAction(
        Command.SelectSelectTool,
        FExecuteAction::CreateRaw(this, &FTiledLevelEdMode::SwitchToNewTool, ETiledLevelEditTool::Select),
        FCanExecuteAction::CreateRaw(this, &FTiledLevelEdMode::IsReadyToEdit),
        FIsActionChecked::CreateRaw(this, &FTiledLevelEdMode::IsTargetToolChecked, ETiledLevelEditTool::Select));
    UICommandList->MapAction(
        Command.SelectPaintTool,
        FExecuteAction::CreateRaw(this, &FTiledLevelEdMode::SwitchToNewTool, ETiledLevelEditTool::Paint),
        FCanExecuteAction::CreateRaw(this, &FTiledLevelEdMode::IsReadyToEdit),
        FIsActionChecked::CreateRaw(this, &FTiledLevelEdMode::IsTargetToolChecked, ETiledLevelEditTool::Paint));
    UICommandList->MapAction(
        Command.SelectEraserTool,
        FExecuteAction::CreateRaw(this, &FTiledLevelEdMode::SwitchToNewTool, ETiledLevelEditTool::Eraser),
        FCanExecuteAction::CreateRaw(this, &FTiledLevelEdMode::IsReadyToEdit),
        FIsActionChecked::CreateRaw(this, &FTiledLevelEdMode::IsTargetToolChecked, ETiledLevelEditTool::Eraser));
    UICommandList->MapAction(
        Command.SelectEyedropperTool,
        FExecuteAction::CreateRaw(this, &FTiledLevelEdMode::SwitchToNewTool, ETiledLevelEditTool::Eyedropper),
        FCanExecuteAction::CreateRaw(this, &FTiledLevelEdMode::IsReadyToEdit),
        FIsActionChecked::CreateRaw(this, &FTiledLevelEdMode::IsTargetToolChecked, ETiledLevelEditTool::Eyedropper));
    UICommandList->MapAction(
        Command.SelectFillTool,
        FExecuteAction::CreateRaw(this, &FTiledLevelEdMode::SwitchToNewTool, ETiledLevelEditTool::Fill),
        FCanExecuteAction::CreateRaw(this, &FTiledLevelEdMode::IsReadyToEdit),
        FIsActionChecked::CreateRaw(this, &FTiledLevelEdMode::IsTargetToolChecked, ETiledLevelEditTool::Fill));
    UICommandList->MapAction(
        Command.ToggleGrid,
        FExecuteAction::CreateRaw(this, &FTiledLevelEdMode::ToggleGrid),
        FCanExecuteAction(),
        FIsActionChecked::CreateLambda([this](){ return IsHelperVisible; }));

    UICommandList->MapAction(
        Command.ToggleAutoPaint,
        FExecuteAction::CreateRaw(this, &FTiledLevelEdMode::ToggleAutoPaint),
        FCanExecuteAction::CreateRaw(this, &FTiledLevelEdMode::IsReadyToEdit),
        FIsActionChecked::CreateRaw(this, &FTiledLevelEdMode::IsAutoPaintChecked)
        );
    
    UICommandList->MapAction(
        Command.RespawnAutoPaintInstances,
        FExecuteAction::CreateRaw(this, &FTiledLevelEdMode::UpdateAutoPaintPlacements)
    );
    
    UICommandList->MapAction(
        Command.ToggleAutoPaintItemHint,
        FExecuteAction::CreateRaw(this, &FTiledLevelEdMode::ToggleAutoPaintVisibility),
        FCanExecuteAction(),
        FIsActionChecked::CreateRaw(this, &FTiledLevelEdMode::GetShowAutoPaintHint)
    );
    
    UICommandList->MapAction(
        Command.ToggleEditViewMode,
        FExecuteAction::CreateRaw(this, &FTiledLevelEdMode::ToggleEditViewMode),
        FCanExecuteAction(),
        FIsActionChecked::CreateRaw(this, &FTiledLevelEdMode::IsEditViewModeSide)
    );

    
    UICommandList->MapAction(
        Command.MirrorX,
        FExecuteAction::CreateRaw(this, &FTiledLevelEdMode::MirrorItem, EAxis::X),
        FCanExecuteAction::CreateRaw(this, &FTiledLevelEdMode::CanMirrorItem),
        FIsActionChecked::CreateLambda([this](){ return IsMirrored_X; })
        );
    UICommandList->MapAction(
        Command.MirrorY,
        FExecuteAction::CreateRaw(this, &FTiledLevelEdMode::MirrorItem, EAxis::Y),
        FCanExecuteAction::CreateRaw(this, &FTiledLevelEdMode::CanMirrorItem),
        FIsActionChecked::CreateLambda([this](){ return IsMirrored_Y; })
        );
    UICommandList->MapAction(
        Command.MirrorZ,
        FExecuteAction::CreateRaw(this, &FTiledLevelEdMode::MirrorItem, EAxis::Z),
        FCanExecuteAction::CreateRaw(this, &FTiledLevelEdMode::CanMirrorItem),
        FIsActionChecked::CreateLambda([this](){ return IsMirrored_Z; })
        );
    
    UICommandList->MapAction(
        Command.RotateCW,
        FExecuteAction::CreateRaw(this, &FTiledLevelEdMode::RotateItem, true),
        FCanExecuteAction::CreateRaw(this, &FTiledLevelEdMode::CanRotateItem));
    UICommandList->MapAction(
        Command.RotateCCW,
        FExecuteAction::CreateRaw(this, &FTiledLevelEdMode::RotateItem, false),
        FCanExecuteAction::CreateRaw(this, &FTiledLevelEdMode::CanRotateItem));
    UICommandList->MapAction(
        Command.EditUpperFloor,
        FExecuteAction::CreateRaw(this, &FTiledLevelEdMode::EditAnotherFloor, true),
        FCanExecuteAction::CreateRaw(this, &FTiledLevelEdMode::IsReadyToEdit));
    UICommandList->MapAction(
        Command.EditLowerFloor,
        FExecuteAction::CreateRaw(this, &FTiledLevelEdMode::EditAnotherFloor, false),
        FCanExecuteAction::CreateRaw(this, &FTiledLevelEdMode::IsReadyToEdit));
    UICommandList->MapAction(
        Command.MultiMode,
        FExecuteAction::CreateRaw(this, &FTiledLevelEdMode::ToggleMultiMode),
        FCanExecuteAction::CreateLambda([this](){return ActiveEditTool == ETiledLevelEditTool::Paint;}),
        FIsActionChecked::CreateLambda([this](){ return IsMultiMode; })
        );
    UICommandList->MapAction(
        Command.Snap,
        FExecuteAction::CreateRaw(this, &FTiledLevelEdMode::ToggleSnap),
        FCanExecuteAction::CreateRaw(this, &FTiledLevelEdMode::CanToggleSnap),
        FIsActionChecked::CreateRaw(this, &FTiledLevelEdMode::IsEnableSnapChecked)
        );
}

bool FTiledLevelEdMode::ValidCheck()
{
    const bool Result = Helper && ActiveLevel && ActiveAsset && ActiveAsset->GetActiveFloor();
    if (!Result) ERROR_LOG("Valid Check not passed!")
    return Result;
}

void FTiledLevelEdMode::SavePreference()
{
    GConfig->SetInt(TEXT("TiledLevel"), TEXT("EraserTarget"), static_cast<int>(EraserTarget), GEditorPerProjectIni);
    GConfig->SetVector(TEXT("TiledLevel"), TEXT("EraserExtent"), FVector(EraserExtent), GEditorPerProjectIni);
    // GConfig->SetBool(TEXT("TiledLevel"), TEXT("ShowHelper"), IsHelperVisible, GEditorPerProjectIni);
    GConfig->SetFloat(TEXT("TiledLevel"), TEXT("ThumbnailScale"), GetMutableDefault<UTiledLevelSettings>()->ThumbnailScale, GEditorPerProjectIni);
}

void FTiledLevelEdMode::LoadPreference()
{
    int LoadedEraserTargetIndex = INDEX_NONE;
    FVector LoadedEraserExtent = FVector(0);
    
    GConfig->GetInt(TEXT("TiledLevel"), TEXT("EraserTarget"), LoadedEraserTargetIndex, GEditorPerProjectIni);
    GConfig->GetVector(TEXT("TiledLevel"), TEXT("EraserExtent"), LoadedEraserExtent, GEditorPerProjectIni);
    // GConfig->GetBool(TEXT("TiledLevel"), TEXT("ShowHelper"), IsHelperVisible, GEditorPerProjectIni);
    // // TODO: this will trigger a minor bug... and it does not make too much beneficial, turn it off by now
    GConfig->GetFloat(TEXT("TiledLevel"), TEXT("ThumbnailScale"), GetMutableDefault<UTiledLevelSettings>()->ThumbnailScale, GEditorPerProjectIni);
    
    if (LoadedEraserTargetIndex == INDEX_NONE)
        EraserTarget = EPlacedType::Any;
    else
        EraserTarget = static_cast<EPlacedType>(LoadedEraserTargetIndex);
    if (LoadedEraserExtent.X > 16 || LoadedEraserExtent.X < 1)
        EraserExtent = FIntVector(1);
    else
        EraserExtent = FIntVector(LoadedEraserExtent);

}

void FTiledLevelEdMode::SwitchToNewTool(ETiledLevelEditTool::Type NewTool)
{
    if (ActiveEditTool == NewTool)
    {
        return;
    }
    ActiveEditTool = NewTool;
    // clear selection will also null active item... 
    UTiledLevelItem* CachedActiveItem = ActiveItem;
    GetPalettePtr()->ClearItemSelection();
    ActiveItem = CachedActiveItem;
    if (NewTool == ETiledLevelEditTool::Paint)
    {
        if (ActiveItem)
        {
            GetPalettePtr()->SelectItem(ActiveItem);
        }
    }
    if (NewTool == ETiledLevelEditTool::Fill)
    {
        GetPalettePtr()->DisplayFillDetail(true);
    }
    else
    {
        GetPalettePtr()->DisplayFillDetail(false);
    }
    GetPalettePtr()->UpdatePalette();
    if (IsMouseInsideCanvas)
    {
        SetupBrush();
    }
}

bool FTiledLevelEdMode::IsTargetToolChecked(ETiledLevelEditTool::Type TargetTool) const
{
    return ActiveEditTool == TargetTool;
}

//instanced mesh not support negative scale... force spawn Static mesh actor instead!
void FTiledLevelEdMode::MirrorItem(EAxis::Type MirrorAxis)
{
    Helper->MirrorPaintBrush(MirrorAxis, IsMirrored_X, IsMirrored_Y, IsMirrored_Z);
}

bool FTiledLevelEdMode::CanMirrorItem() const
{
    return ActiveEditTool == ETiledLevelEditTool::Paint && IsValid(ActiveItem) && IsReadyToEdit();
}

void FTiledLevelEdMode::ToggleGrid()
{
    IsHelperVisible = !IsHelperVisible;
    Helper->SetHelperGridsVisibility(IsHelperVisible);
}

void FTiledLevelEdMode::RotateItem(bool IsClockWise)
{
    if (ActiveEditTool == ETiledLevelEditTool::Select)
    {
        SelectionHelper->RotateSelection(IsClockWise);
        return;
    }
    if (ActiveItem )
    {
        if (ActiveItem->IsA<UTiledLevelTemplateItem>())
        {
            SelectionHelper->RotateSelection(IsClockWise);
            return;
        }
    }
    if (ActiveEditTool == ETiledLevelEditTool::Eraser)
    {
        Helper->RotateEraserBrush(IsClockWise, ShouldRotateBrushExtent);
    } else
    {
        Helper->RotateBrush(IsClockWise, ActivePlacedType, ShouldRotateBrushExtent);
    }
    if (CurrentEditShape != EPlacedShapeType::Shape2D)
    {
        EnterNewGrid(CurrentTilePosition);
    } else
    {
        FTiledLevelEdge NewEdge = CurrentEdge;
        NewEdge.EdgeType = ShouldRotateBrushExtent? EEdgeType::Vertical : EEdgeType::Horizontal;
        EnterNewEdge(NewEdge);
    }
}

bool FTiledLevelEdMode::CanRotateItem() const
{
    if (!IsReadyToEdit()) return false;
    if (ActiveEditTool == ETiledLevelEditTool::Paint && ActiveItem)
        return true;
    if (ActiveEditTool == ETiledLevelEditTool::Eraser && (EraserTarget == EPlacedType::Wall || EraserTarget == EPlacedType::Edge))
        return true;
    if (ActiveEditTool == ETiledLevelEditTool::Select)
        return true;
    return false;
}

void FTiledLevelEdMode::EditAnotherFloor(bool IsUpper)
{
    bool bIsPreviousFloorHidden = !ActiveAsset->GetActiveFloor()->ShouldRenderInEditor;
    if (FloorListWidget.IsValid())
    {
        if (IsUpper)
            FloorListWidget->SelectFloorAbove(false);
        else
            FloorListWidget->SelectFloorBelow(false);
    }
    
    bool bIsCurrentFloorHidden = !ActiveAsset->GetActiveFloor()->ShouldRenderInEditor;
    if (bIsPreviousFloorHidden == bIsCurrentFloorHidden) return;
    if (bIsCurrentFloorHidden)
    {
        Helper->ResetBrush();
        IsBrushSetup = false;
        SelectionHelper->Hide();
    }
    else
    {
        SetupBrush();
    }
}

// TODO: expose multi mode to game time system?
void FTiledLevelEdMode::ToggleMultiMode()
{
    IsMultiMode = !IsMultiMode;
}

// TODO: expose auto snap mode to game time system?
void FTiledLevelEdMode::ToggleSnap()
{
    bSnapEnabled = !bSnapEnabled;
    if (IsMouseInsideCanvas)
    {
        SetupBrush();
        if (CurrentEditShape == EPlacedShapeType::Shape2D)
        {
            EnterNewEdge(CurrentEdge);
        } else
        {
            EnterNewGrid(CurrentTilePosition);
        }
    }
}

bool FTiledLevelEdMode::CanToggleSnap() const
{
    if (ActiveEditTool ==ETiledLevelEditTool::Fill)
    {
        for (auto Item : SelectedItems)
            if (Item->bSnapToFloor || Item->bSnapToWall)
                return true;
    }
    if (ActiveItem)
    {
        return (ActiveItem->bSnapToFloor || ActiveItem->bSnapToWall);
    }
    return false;
}

bool FTiledLevelEdMode::IsEnableSnapChecked() const
{
    if (ActiveEditTool ==ETiledLevelEditTool::Fill)
    {
        bool HasAnySnap = false;
        for (auto Item : SelectedItems)
        {
            if (Item->bSnapToFloor || Item->bSnapToWall )
            {
                HasAnySnap = true;
                break;
            }
        }
        return bSnapEnabled && HasAnySnap;
    }
    if (!ActiveItem) return false;
    return (ActiveItem->bSnapToFloor || ActiveItem->bSnapToWall) && bSnapEnabled;
}

void FTiledLevelEdMode::ToggleAutoPaint()
{
    const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "TiledLevelMode_ChangePaintMode", "Change paint mode"));
    ActiveAsset->Modify();
    PaintMode == ETiledLevelPaintMode::Auto? SetPaintMode(ETiledLevelPaintMode::Normal) : SetPaintMode(ETiledLevelPaintMode::Auto);
}

void FTiledLevelEdMode::SetPaintMode(ETiledLevelPaintMode::Type NewMode)
{
    if (NewMode == ETiledLevelPaintMode::Normal)
    {
        PaintMode = ETiledLevelPaintMode::Normal;
        StaticCastSharedPtr<FTiledLevelEdModeToolkit>(Toolkit)->AssetChooserSwitcher->SetActiveWidgetIndex(0);
        StaticCastSharedPtr<FTiledLevelEdModeToolkit>(Toolkit)->PaletteSwitcher->SetActiveWidgetIndex(0);
        AutoPaintHelper->ClearHint();
        ActiveAsset->SetNewViewState(false);
    }
    else
    {
        PaintMode = ETiledLevelPaintMode::Auto;
        StaticCastSharedPtr<FTiledLevelEdModeToolkit>(Toolkit)->AssetChooserSwitcher->SetActiveWidgetIndex(1);
        StaticCastSharedPtr<FTiledLevelEdModeToolkit>(Toolkit)->PaletteSwitcher->SetActiveWidgetIndex(1);
        DisplayAutoPaintHint();
        ActiveAsset->SetNewViewState(true);
    }
}

bool FTiledLevelEdMode::IsAutoPaintChecked() const
{
    return PaintMode == ETiledLevelPaintMode::Auto;
}

void FTiledLevelEdMode::ToggleAutoPaintVisibility()
{
    if (PaintMode == ETiledLevelPaintMode::Normal) return;
	ActiveAsset->GetAutoPaintRule()->bShowHint = !ActiveAsset->GetAutoPaintRule()->bShowHint;
	DisplayAutoPaintHint();
}

bool FTiledLevelEdMode::GetShowAutoPaintHint()
{
    if (!ActiveAsset) return false;
    if (!ActiveAsset->GetAutoPaintRule()) return false;
	return ActiveAsset->GetAutoPaintRule()->bShowHint;
}

void FTiledLevelEdMode::CreateAutoPaintPlacements()
{
    if (ActiveLevel == nullptr || ActiveAsset == nullptr || Helper == nullptr)
    {
        ERROR_LOG("Nullptr during update auto paint placements...")
        return;
    }
    TArray<FAutoPaintPlacement_Tile> GenTiles;
    TArray<FAutoPaintPlacement_Edge> GenEdges;
    TArray<FAutoPaintPlacement_Point> GenPoints;
    ActiveAsset->CollectAutoPaintPlacements(GenTiles, GenEdges, GenPoints);
    TArray<FAutoPaintPlacement*> GenAll; 
    for (auto& P : GenTiles)
    {
        GenAll.Add(&P);
    }
    for (auto& P : GenEdges)
        GenAll.Add(&P);
    for (auto& P : GenPoints)
        GenAll.Add(&P);

    // use helper trick to set offset properly...
    TArray<FTilePlacement> FinTiles;
    TArray<FEdgePlacement> FinEdges;
    TArray<FPointPlacement> FinPoints;
    UTiledLevelItem* ItemToPaint = nullptr;
    uint8 CurrentRotationState = 0;
    for (auto P : GenAll)
    {
        if (P->GetPlacement()->GetItem() != ItemToPaint)
        {
            ItemToPaint = P->GetPlacement()->GetItem();
            Helper->ResetBrush();
            CurrentRotationState = 0;
            IsMirrored_X = false;
            IsMirrored_Y = false;
            IsMirrored_Z = false;
            Helper->SetupPaintBrush(ItemToPaint);
            if (P->IsMirrorX) MirrorItem(EAxis::X);
            if (P->IsMirrorY) MirrorItem(EAxis::Y);
            if (P->IsMirrorZ) MirrorItem(EAxis::Z);
        }
        P->GetPlacement()->IsMirrored = P->IsMirrorX || P->IsMirrorY || P->IsMirrorZ;
        Helper->ApplyAdditionalAutoPaintTransform(P->AdditionalTransform);

        if (FAutoPaintPlacement_Tile* TP = static_cast<FAutoPaintPlacement_Tile*>(P))
        {
            Helper->MoveBrush(TP->TilePlacement.GridPosition, false);
        }
        else if (FAutoPaintPlacement_Edge* EP = static_cast<FAutoPaintPlacement_Edge*>(P))
        {
            Helper->MoveBrush(EP->EdgePlacement.Edge, false);
        }
        else if (FAutoPaintPlacement_Point* PP = static_cast<FAutoPaintPlacement_Point*>(P))
        {
            Helper->MoveBrush(PP->PointPlacement.GridPosition, false);
        }
        
        if (CurrentRotationState != P->RotationTimes)
        {
            if (P->RotationTimes > CurrentRotationState)
            {
                for (int i = 0; i < P->RotationTimes - CurrentRotationState; i++)
                    Helper->RotateBrush(true, ItemToPaint->PlacedType, ShouldRotateBrushExtent);
            }
            else
            {
                for (int i = 0; i < CurrentRotationState - P->RotationTimes; i++)
                    Helper->RotateBrush(false, ItemToPaint->PlacedType, ShouldRotateBrushExtent);
            }
            CurrentRotationState = P->RotationTimes;
        }
        
        FTransform G = Helper->GetPreviewPlacementWorldTransform().GetRelativeTransform(ActiveLevel->GetTransform());
        P->GetPlacement()->TileObjectTransform.AddToTranslation(G.GetTranslation());
        P->GetPlacement()->TileObjectTransform.SetRotation(G.GetRotation());
        P->GetPlacement()->TileObjectTransform.SetScale3D(P->GetPlacement()->TileObjectTransform.GetScale3D() * G.GetScale3D());
        if (FAutoPaintPlacement_Tile* TP = static_cast<FAutoPaintPlacement_Tile*>(P))
        {
            FinTiles.Add(TP->TilePlacement);
        }  
        else if (FAutoPaintPlacement_Edge* EP = static_cast<FAutoPaintPlacement_Edge*>(P))
        {
            FinEdges.Add(EP->EdgePlacement);
        }
        else if (FAutoPaintPlacement_Point* PP = static_cast<FAutoPaintPlacement_Point*>(P))
        {
            FinPoints.Add(PP->PointPlacement);
        }
    }
    
    Helper->ResetBrush();
    IsMirrored_X = false;
    IsMirrored_Y = false;
    IsMirrored_Z = false;
    if (IsMouseInsideCanvas)
    {
        if (PaintMode == ETiledLevelPaintMode::Auto)
        {
            Helper->SetupAutoPaintBrush(GetActiveAutoPaintColor());
        }
        else
        {
            if (ActiveItem)
            {
                Helper->SetupPaintBrush(ActiveItem);
                if (CurrentEditShape == EPlacedShapeType::Shape3D)
                {
                    Helper->MoveBrush(CurrentTilePosition, bSnapEnabled);
                }
                else
                {
                    Helper->MoveBrush(CurrentEdge, bSnapEnabled);
                }
            }
        }
    }
    ActiveAsset->UpdateAutoPaintResults(FinTiles, FinEdges, FinPoints);
    ActiveAsset->VersionNumber += 1;
}

void FTiledLevelEdMode::UpdateAutoPaintPlacements()
{
    CreateAutoPaintPlacements();
    ActiveLevel->ResetAllInstance();
}


void FTiledLevelEdMode::ToggleEditViewMode()
{
    ActiveViewMode = ActiveViewMode == ETiledLevelEditViewMode::TopDown?
        ETiledLevelEditViewMode::Side : ETiledLevelEditViewMode::TopDown;
}

bool FTiledLevelEdMode::IsEditViewModeSide() const
{
    return ActiveViewMode == ETiledLevelEditViewMode::Side;
}

void FTiledLevelEdMode::ApplyCursor(void* Cursor)
{
    FSlateApplication::Get().GetPlatformCursor()->SetTypeShape(EMouseCursor::Default, Cursor);
    FSlateApplication::Get().GetPlatformCursor()->SetTypeShape(EMouseCursor::Crosshairs, Cursor);
}

void FTiledLevelEdMode::SetupBrush()
{
    Helper->ResetBrush();
    Helper->SetTileSize(ActiveAsset->GetTileSize());
    IsBrushSetup = true;
    bStraightEdit = false;
    CurrentEditShape = EPlacedShapeType::Shape3D;
    IsMirrored_X = false;
    IsMirrored_Y = false;
    IsMirrored_Z = false;
    TArray<EPlacedType> EdgeTypes = { EPlacedType::Wall, EPlacedType::Edge };
    TArray<EPlacedType> PointTypes = { EPlacedType::Pillar, EPlacedType::Point };

    if (ActiveEditTool != ETiledLevelEditTool::Select)
    {
        IsInstancesSelected = false;
        SelectionHelper->Hide();
    }
    ApplyCursor(nullptr);
    switch (ActiveEditTool)
    {
    case ETiledLevelEditTool::Select:
        ApplyCursor(CursorResource_Select);
        IsInstancesSelected = true;
        SelectionHelper->PopulateSelected();
        SelectionHelper->Unhide();
        break;
    case ETiledLevelEditTool::Paint:
    {
        // TODO: rewrite the whole block... too much messy...
        if (PaintMode == ETiledLevelPaintMode::Auto)
        {
            IsBrushSetup = GetActiveAutoPaintName() != FName("Empty");
            if (!IsBrushSetup) return;
        }
        else if (!ActiveItem)
        {
            IsBrushSetup = false;
            return;
        }
        ApplyCursor(CursorResource_Pen);
        if (const UTiledLevelTemplateItem* TemplateItem = Cast<UTiledLevelTemplateItem>(ActiveItem))
        {
            if (ActiveAsset == TemplateItem->GetAsset()) // block edit on template
            {
                IsBrushSetup = false;
                return;
            }
            IsInstancesSelected = true;
            const UTiledLevelAsset* TemplateAsset = TemplateItem->GetAsset();
            TArray<FTilePlacement> AssetTiles = TemplateAsset->GetAllBlockPlacements();
            AssetTiles.Append(TemplateAsset->GetAllFloorPlacements());
            TArray<FEdgePlacement> AssetEdges = TemplateAsset->GetAllWallPlacements();
            AssetEdges.Append(TemplateAsset->GetAllEdgePlacements());
            TArray<FPointPlacement> AssetPoints = TemplateAsset->GetAllPointPlacements();
            AssetPoints.Append(TemplateAsset->GetAllPillarPlacements());
            SelectionHelper->SyncToSelection(ActiveAsset->GetTileSize(), AssetTiles, AssetEdges, AssetPoints, true);
            SelectionHelper->PopulateSelected(true);
            SelectionHelper->Unhide();
        }
        else
        {
            if (PaintMode == ETiledLevelPaintMode::Normal)
            {
                ActivePlacedType = ActiveItem->PlacedType;
                if (EdgeTypes.Contains(ActiveItem->PlacedType))
                    CurrentEditShape = EPlacedShapeType::Shape2D;
                else if (PointTypes.Contains(ActiveItem->PlacedType))
                    CurrentEditShape = EPlacedShapeType::Shape1D;
                Helper->SetupPaintBrush(ActiveItem);
                for (int i = 0; i < CachedBrushRotationIndex; i++)
                    RotateItem(true);
            }
            else
            {
                Helper->SetupAutoPaintBrush(GetActiveAutoPaintColor());
            }
        }
        break;
    }
    case ETiledLevelEditTool::Eraser:
    {
        ApplyCursor(CursorResource_Eraser);
        if (PaintMode == ETiledLevelPaintMode::Auto)
        {
            ActivePlacedType = EPlacedType::Block;
            // TODO: disable allowed case in toolkit?
            Helper->SetupEraserBrush(EraserExtent, EPlacedType::Block);
        }
        else
        {
            ActivePlacedType = EraserTarget;
            if (EdgeTypes.Contains(EraserTarget))
                CurrentEditShape = EPlacedShapeType::Shape2D;
            else if (PointTypes.Contains(EraserTarget))
                CurrentEditShape = EPlacedShapeType::Shape1D;
            Helper->SetupEraserBrush(EraserExtent, EraserTarget); 
        }
        break;
    }
    case ETiledLevelEditTool::Eyedropper:
        ApplyCursor(CursorResource_Eyedropper);
        break;
    case ETiledLevelEditTool::Fill:
        MaxZInFillItems = 1;
        for (UTiledLevelItem* Item : SelectedItems)
        {
            if (MaxZInFillItems < Item->Extent.Z)
            {
                MaxZInFillItems = Item->Extent.Z;
            }
        }
        ApplyCursor(CursorResource_Fill);
        break;
    default:
        IsBrushSetup = false;
        ;
    }
}


void FTiledLevelEdMode::SetupBrushCursorOnly()
{
    switch (ActiveEditTool) {
        case ETiledLevelEditTool::Select:
            ApplyCursor(CursorResource_Select);
            break;
        case ETiledLevelEditTool::Paint:
            ActiveItem == nullptr? ApplyCursor(nullptr) : ApplyCursor(CursorResource_Pen);
            break;
        case ETiledLevelEditTool::Eraser:
            ApplyCursor(CursorResource_Eraser);
            break;
        case ETiledLevelEditTool::Eyedropper:
            ApplyCursor(CursorResource_Eyedropper);
            break;
        case ETiledLevelEditTool::Fill:
            ApplyCursor(CursorResource_Fill);
            break;
        default:
            ApplyCursor(nullptr);
    }
}

void FTiledLevelEdMode::MouseMovement_Internal(FEditorViewportClient* ViewportClient, const int32& InMouseX, const int32& InMouseY)
{
    if (IsBrushSetup && ValidCheck())
    {
        if (ActiveEditTool == ETiledLevelEditTool::Eyedropper)
        {
            UpdateEyedropperTarget(ViewportClient, InMouseX, InMouseY);
        } else
        {
            UpdateGirdLocation(ViewportClient, InMouseX, InMouseY);
        }
    }
}

FVector FTiledLevelEdMode::GetMouseLocation(FEditorViewportClient* ViewportClient, int32 InMouseX, int32 InMouseY, FVector& OutTraceStart, FVector& OutTraceEnd)
{
    FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(
            ViewportClient->Viewport,
            ViewportClient->GetScene(),
            ViewportClient->EngineShowFlags)
        .SetRealtimeUpdate(ViewportClient->IsRealtime()));

    FSceneView* View = ViewportClient->CalcSceneView(&ViewFamily);
    FViewportCursorLocation MouseViewportRay(View, ViewportClient, InMouseX, InMouseY);
    
    OutTraceStart = MouseViewportRay.GetOrigin();
    OutTraceEnd = MouseViewportRay.GetOrigin() + MouseViewportRay.GetDirection() * HALF_WORLD_MAX;
    FVector TileSize = ActiveAsset->GetTileSize();
    
    // only we need convert world ray to local!!
    FTransform ToLevelTransform = ActiveLevel->GetActorTransform();
    FVector LocalStart = ToLevelTransform.InverseTransformPosition(OutTraceStart);
    FVector LocalEnd = ToLevelTransform.InverseTransformPosition(OutTraceEnd);
   
    if (ActiveAsset->GetActiveFloor() == nullptr) return FVector(0, 0, -9999);
    
    FVector TempLocation = FMath::LinePlaneIntersection(
        LocalStart,
        LocalEnd,
        FVector(0, 0, ActiveAsset->GetActiveFloor()->FloorPosition * TileSize.Z),
        FVector(0, 0, 1)
    );

    return TempLocation;
}

FVector2D FTiledLevelEdMode::GetMouse2DLocation(FEditorViewportClient* ViewportClient, int32 InMouseX, int32 InMouseY)
{
    FVector TempStart, TempEnd;
    return FVector2D(GetMouseLocation(ViewportClient, InMouseX, InMouseY, TempStart, TempEnd));
}

void FTiledLevelEdMode::UpdateGirdLocation(FEditorViewportClient* ViewportClient, int32 InMouseX, int32 InMouseY)
{
    const FVector TileSize = ActiveAsset->GetTileSize();
    FVector TraceStart, TraceEnd;
    FVector TempLocation = GetMouseLocation(ViewportClient, InMouseX, InMouseY, TraceStart, TraceEnd);

    // Make int(-10/250) to become -1 ... 
    if (TempLocation.X < 0)
    {
        TempLocation.X -= TileSize.X;
    }
    if (TempLocation.Y < 0)
    {
        TempLocation.Y -= TileSize.Y;
    }

    if (CurrentEditShape != EPlacedShapeType::Shape2D)
    {
        FIntVector NewBrushLocation = FIntVector(static_cast<int>(TempLocation.X / TileSize.X),
                                                 static_cast<int>(TempLocation.Y / TileSize.Y),
                                                 ActiveAsset->GetActiveFloor()->FloorPosition);
        if (NewBrushLocation != CurrentTilePosition)
        {
            EnterNewGrid(NewBrushLocation);
        }
    }
    else
    {
        // check whether is in new edge
        FTiledLevelEdge TestEdge;
        TestEdge.X = static_cast<int>(TempLocation.X / TileSize.X);
        TestEdge.Y = static_cast<int>(TempLocation.Y / TileSize.Y);
        TestEdge.Z = ActiveAsset->GetActiveFloor()->FloorPosition;
        TestEdge.EdgeType = ShouldRotateBrushExtent? EEdgeType::Vertical: EEdgeType::Horizontal;
        if (TestEdge != CurrentEdge )
        {
            EnterNewEdge(TestEdge);
        }
    }
}

void FTiledLevelEdMode::UpdateEyedropperTarget(FEditorViewportClient* ViewportClient, int32 InMouseX, int32 InMouseY)
{
    const FVector TileSize = ActiveAsset->GetTileSize();
    FVector TraceStart, TraceEnd;
    FVector TempLocation = GetMouseLocation(ViewportClient, InMouseX, InMouseY, TraceStart, TraceEnd);

    if (CurrentEditShape != EPlacedShapeType::Shape2D)
    {
        CurrentTilePosition = FIntVector(static_cast<int>(TempLocation.X / TileSize.X),
            static_cast<int>(TempLocation.Y / TileSize.Y),
            ActiveAsset->GetActiveFloor()->FloorPosition);
    }
    else
    {
        // check whether is in new edge
        FTiledLevelEdge TestEdge;
        TestEdge.X = static_cast<int>(TempLocation.X / TileSize.X);
        TestEdge.Y = static_cast<int>(TempLocation.Y / TileSize.Y);
        TestEdge.Z = ActiveAsset->GetActiveFloor()->FloorPosition;
        TestEdge.EdgeType = ShouldRotateBrushExtent? EEdgeType::Vertical: EEdgeType::Horizontal;
        CurrentEdge = TestEdge;
    }
    
    EyedropperTarget = nullptr;
    EyedropperTarget_AutoPaint = TEXT("");
    FHitResult HitResult;
    UWorld* World = ActiveLevel->GetWorld();
    FCollisionQueryParams CQ;
    CQ.AddIgnoredComponent(PreviewSceneFloor);
    CQ.bReturnFaceIndex = true;
    if (World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECollisionChannel::ECC_Visibility, CQ))
    {
        if (PaintMode == ETiledLevelPaintMode::Auto)
        {
            int HitSectionId = FTiledLevelUtility::GetPMC_SectionIdFromHit(HitResult);
            if (HitSectionId != INDEX_NONE)
                EyedropperTarget_AutoPaint = AutoPaintHelper->HintNames[HitSectionId];
           return; 
        }
        if (const UHierarchicalInstancedStaticMeshComponent* HitHISM = Cast<UHierarchicalInstancedStaticMeshComponent>(HitResult.GetComponent()))
        {
            EyedropperTarget = FindItemFromMeshPtr(HitHISM->GetStaticMesh());
        }
        else if (const AActor* HitActor = HitResult.GetActor())
        {
            if (HitActor->Tags.IsEmpty()) return;
            FString UidString = HitActor->Tags.Last().ToString();
            bool IsUid = false;
            FGuid ParsedUid;
            UKismetGuidLibrary::Parse_StringToGuid(UidString, ParsedUid, IsUid);
            if (!IsUid) return;
            for (UTiledLevelItem* Item : ExistingItems)
            {
                if (Item->ItemID == ParsedUid)
                {
                    EyedropperTarget = Item;
                    return;
                }
            }
        }
    }
}


void FTiledLevelEdMode::EnterNewGrid_Fill()
{
    if (SelectedItems.Num() == 0) return;
    FIntPoint BoardSize = {ActiveAsset->X_Num, ActiveAsset->Y_Num};
    if (IsFillTiles)
    {
        CandidateFillTiles.Empty();
        if (IsTileAsFillBoundary)
        {
            SetupFillBoardFromTiles();
            FTiledLevelUtility::FloodFill(Board, BoardSize,CurrentTilePosition.X, CurrentTilePosition.Y, CandidateFillTiles);
        }
        else
        {
            // Init board
            Board.Empty();
            BoardSize = { ActiveAsset->X_Num,ActiveAsset->Y_Num };
            TArray<int> Zeros;
            Zeros.Init(0, ActiveAsset->Y_Num);
            for (int i = 0; i < ActiveAsset->X_Num; i++)
                Board.Add(Zeros);
            if (NeedGround)
                UpdateFillBoardFromGround();
            // extract edge from 2D-shape placement
            TArray<FTiledLevelEdge> BlockingEdges;
            for (auto EdgePlacement : ActiveAsset->GetActiveFloor()->Get2DShapePlacements())
            {
                int Extent = EdgePlacement.GetItem()->Extent.X;
                FTiledLevelEdge FirstEdge = EdgePlacement.Edge;
                FirstEdge.Z = 1;
                BlockingEdges.AddUnique(FirstEdge);            
                if (Extent != 1)
                {
                    for (int e = 1; e < Extent; e++)
                    {
                        FTiledLevelEdge NewEdge = EdgePlacement.Edge.EdgeType == EEdgeType::Horizontal?
                                                      FTiledLevelEdge(FirstEdge.X + e, FirstEdge.Y, 1, EEdgeType::Horizontal) :
                                                      FTiledLevelEdge(FirstEdge.X, FirstEdge.Y + e, 1, EEdgeType::Vertical);
                        BlockingEdges.AddUnique(NewEdge);
                    }
                }
            }
            // implement fill
            CandidateFillTiles.Empty();
            FTiledLevelUtility::FloodFillByEdges(BlockingEdges, Board, BoardSize, CurrentTilePosition.X, CurrentTilePosition.Y, CandidateFillTiles);
        }
        Helper->UpdateFillPreviewGrids(CandidateFillTiles, ActiveAsset->ActiveFloorPosition, MaxZInFillItems);
    }
    else
    {
        CandidateFillTiles.Empty();
        SetupFillBoardFromTiles();
        FTiledLevelUtility::GetConsecutiveTiles(Board, BoardSize, CurrentTilePosition.X, CurrentTilePosition.Y, CandidateFillTiles);
        const TSet<FIntPoint> Region = TSet<FIntPoint>(CandidateFillTiles);
        CandidateFillEdges = FTiledLevelUtility::GetEdgesAroundArea(Region, ActiveAsset->ActiveFloorPosition, true);
        CandidateFillEdges.Sort();
        Helper->UpdateFillPreviewEdges(CandidateFillEdges, MaxZInFillItems);
    }
}

void FTiledLevelEdMode::UpdateIsInsideValidArea_Grid()
{
    int EX = 1;
    int EY = 1;
    if (ActiveItem)
    {
        EX = Helper->IsBrushExtentChanged()? ActiveItem->Extent.Y : ActiveItem->Extent.X;
        EY = Helper->IsBrushExtentChanged()? ActiveItem->Extent.X : ActiveItem->Extent.Y;
    }

    const bool Con1 = CurrentTilePosition.X >= 0 && CurrentTilePosition.Y >= 0;
    const bool Con2 = CurrentEditShape == EPlacedShapeType::Shape1D?
                          CurrentTilePosition.X <= ActiveAsset->X_Num - EX + 1 && CurrentTilePosition.Y <= ActiveAsset->Y_Num - EY + 1 :
                          CurrentTilePosition.X <= ActiveAsset->X_Num - EX && CurrentTilePosition.Y <= ActiveAsset->Y_Num - EY;
    IsInsideValidArea = Con1 && Con2;
}

void FTiledLevelEdMode::EnterNewGrid(FIntVector NewTilePosition)
{
    // apply steps setting
    int StepSize = GetMutableDefault<UTiledLevelSettings>()->StepSize;
    FIntPoint StartPoint = GetMutableDefault<UTiledLevelSettings>()->StartPoint;
    int NX = FMath::RoundToInt(((NewTilePosition.X - StartPoint.X) % StepSize) / float(StepSize));
    NewTilePosition.X = (FMath::FloorToInt((NewTilePosition.X - StartPoint.X) / float(StepSize)) + NX) * StepSize + StartPoint.X;
    int NY = FMath::RoundToInt(((NewTilePosition.Y - StartPoint.Y) % StepSize) / float(StepSize));
    NewTilePosition.Y = (FMath::FloorToInt((NewTilePosition.Y - StartPoint.Y) / float(StepSize)) + NY) * StepSize + StartPoint.Y;

    // apply strait edit
    if (bStraightEdit)
    {
        if (abs(NewTilePosition.X - FixedTiledPosition.X) >= abs(NewTilePosition.Y - FixedTiledPosition.Y))
        {
            CurrentTilePosition = FIntVector(NewTilePosition.X, FixedTiledPosition.Y, NewTilePosition.Z);
        } else
        {
            CurrentTilePosition = FIntVector(FixedTiledPosition.X, NewTilePosition.Y, NewTilePosition.Z);
        }
    }
    else
    {
        CurrentTilePosition = NewTilePosition;
    }

    // apply actual move
    switch (ActiveEditTool)
    {
    case ETiledLevelEditTool::Select:
        {
            if (IsInstancesSelected)
            {
                SelectionHelper->Move(CurrentTilePosition, ActiveAsset->GetTileSize(), ActiveLevel->GetActorTransform());
            }
            break;
        }
    case ETiledLevelEditTool::Paint:
        {
            if (ActiveItem && ActiveItem->IsA<UTiledLevelTemplateItem>())
            {
                SelectionHelper->Move(CurrentTilePosition, ActiveAsset->GetTileSize(), ActiveLevel->GetActorTransform());
            }
            else
            {
                Helper->MoveBrush(CurrentTilePosition, bSnapEnabled);
                UpdateIsInsideValidArea_Grid();
                if (IsInsideValidArea) // check if outside the paint area
                {
                    Helper->ToggleValidBrush(true);
                    if (ActiveBrushAction == ETiledLevelBrushAction::FreePaint)
                        PaintItemInstance();
                    else if (ActiveBrushAction == ETiledLevelBrushAction::QuickErase)
                        QuickErase();
                }
                else
                {
                    Helper->ToggleValidBrush(false);
                }
            }
            break;
        }
    case ETiledLevelEditTool::Eraser:
        {
            Helper->MoveBrush(CurrentTilePosition, false);
            if (ActiveBrushAction == ETiledLevelBrushAction::Erase)
                EraseItems();
            break;
        }
    case ETiledLevelEditTool::Fill:
        {
            EnterNewGrid_Fill();
            break;
        }
    default: ;
    }
}

void FTiledLevelEdMode::EnterNewEdge(FTiledLevelEdge NewEdge)
{
    if (!ActiveAsset->GetActiveFloor()->ShouldRenderInEditor) return;
    int StepSize = GetMutableDefault<UTiledLevelSettings>()->StepSize;
    FIntPoint StartPoint = GetMutableDefault<UTiledLevelSettings>()->StartPoint;
    int NX = FMath::RoundToInt(((NewEdge.X - StartPoint.X) % StepSize) / float(StepSize));
    NewEdge.X = (FMath::FloorToInt((NewEdge.X - StartPoint.X) / float(StepSize)) + NX) * StepSize + StartPoint.X;
    int NY = FMath::RoundToInt(((NewEdge.Y - StartPoint.Y) % StepSize) / float(StepSize));
    NewEdge.Y = (FMath::FloorToInt((NewEdge.Y - StartPoint.Y) / float(StepSize)) + NY) * StepSize + StartPoint.Y;

    
    // this is for paint only, not feasible for eraser...
    if (bStraightEdit)
    {
       if (NewEdge.X != CurrentEdge.X && CurrentEdge.EdgeType == EEdgeType::Horizontal)
       {
           CurrentEdge.X = NewEdge.X;
       } else if (NewEdge.Y != CurrentEdge.Y && CurrentEdge.EdgeType == EEdgeType::Vertical)
       {
           CurrentEdge.Y = NewEdge.Y;
       }
    }
    else
    {
        CurrentEdge = NewEdge;
    }

    Helper->MoveBrush(CurrentEdge, bSnapEnabled);
    if (ActiveEditTool == ETiledLevelEditTool::Eraser)
    {
        IsInsideValidArea = true;
        if (ActiveBrushAction == ETiledLevelBrushAction::Erase)
            EraseItems();
        return;
    }
    // update is inside area for edge 
    int Length = 1;
    if (ActiveItem)
    {
        Length = ActiveItem->Extent.X;
    }
    IsInsideValidArea = FTiledLevelUtility::IsEdgeInsideArea(CurrentEdge, Length, FIntPoint(ActiveAsset->X_Num, ActiveAsset->Y_Num));
    if (IsInsideValidArea)
    {
        Helper->ToggleValidBrush(true);
        if (ActiveBrushAction == ETiledLevelBrushAction::FreePaint)
            PaintItemInstance();
        else if (ActiveBrushAction == ETiledLevelBrushAction::QuickErase)
            QuickErase();
    } else
    {
        Helper->ToggleValidBrush(false);
    }
}


void FTiledLevelEdMode::SelectionStart(FEditorViewportClient* ViewportClient)
{
    ActiveBrushAction = ETiledLevelBrushAction::BoxSelect;
    IsInstancesSelected = false;
    const float DPIScale = ViewportClient->GetDPIScale();
    ViewportClient->Viewport->GetMousePos(SelectionBeginMousePos);
    SelectionBeginMousePos.X = SelectionBeginMousePos.X/DPIScale;
    SelectionBeginMousePos.Y = SelectionBeginMousePos.Y/DPIScale;
    SelectionBeginWorldPos = GetMouse2DLocation(ViewportClient, ViewportClient->Viewport->GetMouseX(), ViewportClient->Viewport->GetMouseY());
    
    //Hide other floors
    CachedFloorsVisibility.Empty();
    for (FTiledFloor F : ActiveAsset->TiledFloors)
    {
        CachedFloorsVisibility.Add(F.ShouldRenderInEditor);
    }
    const int ActiveFloorPosition = ActiveAsset->GetActiveFloor()->FloorPosition;
    for (FTiledFloor& F : ActiveAsset->TiledFloors)
    {
        if (bSelectAllFloors)
        {
            F.ShouldRenderInEditor = true;
        } else
        {
            if (F.FloorPosition >= ActiveFloorPosition && F.FloorPosition < ActiveFloorPosition + FloorSelectCount)
            {
                F.ShouldRenderInEditor = true;
            }
            else
            {
                F.ShouldRenderInEditor = false;
            }
        }
    }
    ActiveLevel->ResetAllInstance(true);
    SelectionHelper->Hide();
}

void FTiledLevelEdMode::SelectionEnd(FEditorViewportClient* ViewportClient)
{
    if (SelectionBeginWorldPos == FVector2D(-9999)) return;
    ActiveBrushAction = ETiledLevelBrushAction::None;
    EvaluateBoxSelection(SelectionBeginWorldPos, GetMouse2DLocation(ViewportClient, ViewportClient->Viewport->GetMouseX(), ViewportClient->Viewport->GetMouseY()));
    SelectionBeginWorldPos = FVector2D(-9999);
    // revert previous floor visibility.
    int i = 0;
    for (FTiledFloor& F : ActiveAsset->TiledFloors)
    {
        F.ShouldRenderInEditor = CachedFloorsVisibility[i];
        i++;
    }
    ActiveLevel->ResetAllInstance(true);
    SelectionHelper->PopulateSelected();
    SelectionHelper->Unhide();
}

void FTiledLevelEdMode::SelectionCanceled()
{
    SelectionHelper->EmptyHints();
    IsInstancesSelected = false;
    if (ActiveBrushAction == ETiledLevelBrushAction::BoxSelect && CachedFloorsVisibility.Num() > 0)
    {
        ActiveBrushAction = ETiledLevelBrushAction::None;
        SelectionBeginWorldPos = FVector2D(-9999);
        // revert previous floor visibility.
        int i = 0;
        for (FTiledFloor& F : ActiveAsset->TiledFloors)
        {
            F.ShouldRenderInEditor = CachedFloorsVisibility[i];
            i++;
        }
        ActiveLevel->ResetAllInstance(true);
    }
}

void FTiledLevelEdMode::EvaluateBoxSelection(FVector2D StartPos, FVector2D EndPos)
{
    if (!ActiveAsset) return;
    IsInstancesSelected = false;
    const FBox2D RegionBox = FBox2D(TArray<FVector2D>{ StartPos, EndPos});
    TArray<FTilePlacement> TilesToSelect;
    TArray<FEdgePlacement> EdgesToSelect;
    TArray<FPointPlacement> PointsToSelect;

    int StartFloorPosition = bSelectAllFloors? ActiveAsset->GetBottomFloor().FloorPosition : ActiveAsset->GetActiveFloor()->FloorPosition;
    int EndFloorPosition = bSelectAllFloors? StartFloorPosition + ActiveAsset->TiledFloors.Num() : StartFloorPosition + FloorSelectCount;
    for (int FloorPosition = StartFloorPosition; FloorPosition < EndFloorPosition; FloorPosition++)
    {
        if (!ActiveAsset->GetFloorFromPosition(FloorPosition)) break;
        for (FTilePlacement P : ActiveAsset->GetFloorFromPosition(FloorPosition)->GetTilePlacements())
        {
            if (RegionBox.Intersect(P.GetBox2D(ActiveAsset->GetTileSize())))
            {
                TilesToSelect.Add(P);
            }
        }
        for (FEdgePlacement P : ActiveAsset->GetFloorFromPosition(FloorPosition)->Get2DShapePlacements())
        {
            if (RegionBox.Intersect(P.GetBox2D(ActiveAsset->GetTileSize(), P.GetItem()->Extent)))
            {
                EdgesToSelect.Add(P);
            }
        }
        for (FPointPlacement P : ActiveAsset->GetFloorFromPosition(FloorPosition)->Get1DShapePlacements())
        {
            if (RegionBox.IsInside(P.GetPoint(ActiveAsset->GetTileSize())))
            {
                PointsToSelect.Add(P);
            }
        }
    }
    IsInstancesSelected = TilesToSelect.Num() > 0 || EdgesToSelect.Num() > 0 || PointsToSelect.Num() > 0;
    if (IsInstancesSelected)
    {
        SelectionHelper->SyncToSelection(ActiveAsset->GetTileSize(), TilesToSelect, EdgesToSelect, PointsToSelect);
        EnterNewGrid(CurrentTilePosition);
    }
    else
    {
       SelectionHelper->Hide();
    }
}

void FTiledLevelEdMode::PaintSelectedStart(bool bForTemplate)
{
    ActiveBrushAction = ETiledLevelBrushAction::PaintSelected;
    PaintSelectedInstances(bForTemplate);
}

void FTiledLevelEdMode::PaintSelectedEnd()
{
    ActiveBrushAction = ETiledLevelBrushAction::None;
}

void FTiledLevelEdMode::PaintSelectedInstances(bool bForTemplate)
{
    // make one single transaction
	 FText TransactionMessage = NSLOCTEXT("UnrealEd", "TiledLevelMode_PaintSelectedTransaction", "Paint Selected Instances");
    if (bForTemplate)
    {
        TransactionMessage = NSLOCTEXT("UnrealEd", "TiledLevelMode_PaintTemplateTransaction", "Paint template instances");
    }
	const FScopedTransaction Transaction(TransactionMessage);
    ActiveAsset->Modify();

    // Erase all in selection extent
    ActiveLevel->EraseItem_Any(CurrentTilePosition, SelectionHelper->GetSelectedExtent(bForTemplate));
    // extract data from selection helper
    // update the data && prepare what to erase
    const int BottomFloorPosition = ActiveAsset->GetBottomFloor().FloorPosition;
    const int TopFloorPosition = ActiveAsset->GetTopFloor().FloorPosition;
    TArray<FTilePlacement> SelectedTiles;
    TArray<FEdgePlacement> SelectedEdges;
    TArray<FPointPlacement> SelectedPoints;
    TArray<FTilePlacement> NewTiles;
    TArray<FEdgePlacement> NewEdges;
    TArray<FPointPlacement> NewPoints;
    SelectionHelper->GetSelectedPlacements(SelectedTiles, SelectedEdges, SelectedPoints, bForTemplate);
    for (FTilePlacement P : SelectedTiles)
    {
        P.OffsetGridPosition(CurrentTilePosition, ActiveAsset->GetTileSize());
        if (P.GridPosition.X + P.Extent.X <= ActiveAsset->X_Num && P.GridPosition.Y + P.Extent.Y <= ActiveAsset->Y_Num &&
            P.GridPosition.X >= 0 && P.GridPosition.Y >= 0 &&
            P.GridPosition.Z >= BottomFloorPosition && P.GridPosition.Z <= TopFloorPosition)
        {
            NewTiles.Add(P);
        }
    }
    
    for (FEdgePlacement P : SelectedEdges)
    {
        P.OffsetEdgePosition(CurrentTilePosition, ActiveAsset->GetTileSize());
        if (FTiledLevelUtility::IsEdgeInsideArea(P.Edge, P.GetItem()->Extent.X, FIntPoint(ActiveAsset->X_Num, ActiveAsset->Y_Num)) &&
            P.Edge.Z >= BottomFloorPosition && P.Edge.Z <= TopFloorPosition)
        {
            NewEdges.Add(P);
        }
    }
    
    for (FPointPlacement P : SelectedPoints)
    {
        P.OffsetPointPosition(CurrentTilePosition, ActiveAsset->GetTileSize());
        if (P.GridPosition.X >= 0 && P.GridPosition.X <= ActiveAsset->X_Num && P.GridPosition.Y >=0 && P.GridPosition.Y <= ActiveAsset->Y_Num &&
            P.GridPosition.Z >= BottomFloorPosition && P.GridPosition.Z <= TopFloorPosition)
        {
            NewPoints.Add(P);
        }
    }
    
    for (FTilePlacement NewTile : NewTiles)
    {
        ActiveAsset->AddNewTilePlacement(NewTile);
        ActiveLevel->PopulateSinglePlacement(NewTile);
    }
    for (FEdgePlacement NewEdge : NewEdges)
    {
        ActiveAsset->AddNewEdgePlacement(NewEdge);
        ActiveLevel->PopulateSinglePlacement(NewEdge);
    }
    for (FPointPlacement NewPoint : NewPoints)
    {
        ActiveAsset->AddNewPointPlacement(NewPoint);
        ActiveLevel->PopulateSinglePlacement(NewPoint);
    }
    UpdateStatistics();
}

// TODO: still minor issues in transaction system, but it is way better!!!
// TODO: too messy... need better organization...
void FTiledLevelEdMode::PaintStart()
{
    FixedTiledPosition = FIntVector(0);
    if (bIsControlDown)
    {
        bStraightEdit = true;
        FixedTiledPosition = CurrentTilePosition;
    }
    if (ActiveItem == nullptr && PaintMode == ETiledLevelPaintMode::Normal) return;
    if (GetActiveAutoPaintName() == FName("Empty") && PaintMode == ETiledLevelPaintMode::Auto) return;
    if (bIsShiftDown)
    {
        HasEraseAnything = false;
        const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "TiledLevelMode_QuickEraseItemTransaction", "Quick Erase Item Instance"));
        if (IsInRuleAssetEditor)
        {
            ActiveAsset->GetAutoPaintRule()->Modify();
        }
        ActiveAsset->Modify();
        QuickErase();
        ActiveBrushAction = ETiledLevelBrushAction::QuickErase;
    }
    else
    {
        if (PaintMode == ETiledLevelPaintMode::Auto)
        {
            const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "TiledLevelMode_AutoPaintItemTransaction", "AutoPaint Item Instance"));
            ActiveAsset->Modify();
            if (IsInRuleAssetEditor)
            {
                ActiveAsset->GetAutoPaintRule()->Modify();
            }
            HasPaintAnything = false;
            PaintItemInstance();
        }
        else
        {
            const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "TiledLevelMode_PaintItemTransaction", "Paint Item Instance"));
            ActiveAsset->Modify();
            HasPaintAnything = false;
            PaintItemInstance();
        }
        ActiveBrushAction = ETiledLevelBrushAction::FreePaint;
    }
    ActiveAsset->VersionNumber += 1; // This could happen even nothing painted!
}

void FTiledLevelEdMode::PaintEnd()
{
    if (ActiveBrushAction == ETiledLevelBrushAction::QuickErase && !HasEraseAnything)
    {
        UKismetSystemLibrary::CancelTransaction(1);
    }
    if (ActiveBrushAction == ETiledLevelBrushAction::FreePaint && !HasPaintAnything)
    {
        UKismetSystemLibrary::CancelTransaction(1);
    }
    
    if ((HasPaintAnything || HasEraseAnything) && (PaintMode == ETiledLevelPaintMode::Auto))
    {
        UpdateAutoPaintPlacements();
        // TODO: will this goes to right position?
        Helper->SetupAutoPaintBrush(GetActiveAutoPaintColor());
        Helper->MoveBrush(CurrentTilePosition, false);
    }
    ActiveBrushAction = ETiledLevelBrushAction::None;
    bStraightEdit = false;
    LastPlacedTiles.Empty();
    LastPlacedEdges.Empty();
}

void FTiledLevelEdMode::PaintItemInstance()
{
    if (PaintMode == ETiledLevelPaintMode::Auto)
    {
        if (IsInsideValidArea)
            HasPaintAnything = ActiveAsset->UpdateAutoPaintData(CurrentTilePosition, GetActiveAutoPaintName());
        if (HasPaintAnything)
        {
            DisplayAutoPaintHint();
        }
        return;
    }
    if (PaintItemPreparation())
    {
        HasPaintAnything = true;
        ExistingItems.Add(ActiveItem);
        FTilePlacement NewTile;
        FEdgePlacement NewEdge;
        FPointPlacement NewPoint;
        FItemPlacement* NewPlacement = nullptr;
        switch (CurrentEditShape)
        {
            case Shape3D:
                NewPlacement = &NewTile; 
                break;
            case Shape2D:
                NewPlacement = &NewEdge; 
                break;
            case Shape1D:
                NewPlacement = &NewPoint; 
                break;
        }
        
        NewPlacement->ItemSet = ActiveAsset->GetItemSetAsset();
        NewPlacement->ItemID = ActiveItem->ItemID;
        NewPlacement->IsMirrored = IsMirrored_X || IsMirrored_Y || IsMirrored_Z;

        switch (CurrentEditShape) {
            case Shape3D:
                NewTile.GridPosition = FIntVector(CurrentTilePosition.X, CurrentTilePosition.Y, ActiveAsset->ActiveFloorPosition);
                NewTile.Extent = ShouldRotateBrushExtent? FIntVector(ActiveItem->Extent.Y, ActiveItem->Extent.X, ActiveItem->Extent.Z) : FIntVector(ActiveItem->Extent);
                break;
            case Shape2D:
                NewEdge.Edge = CurrentEdge;
                break;
            case Shape1D:
                NewPoint.GridPosition = FIntVector(CurrentTilePosition.X, CurrentTilePosition.Y, ActiveAsset->ActiveFloorPosition);
                break;
        }
        FTransform G = Helper->GetPreviewPlacementWorldTransform();
        NewPlacement->TileObjectTransform = G.GetRelativeTransform(ActiveLevel->GetTransform());
        switch (CurrentEditShape) {
            case Shape3D:
                ActiveAsset->AddNewTilePlacement(NewTile);
                ActiveLevel->PopulateSinglePlacement<FTilePlacement>(NewTile);
                LastPlacedTiles.Add(NewTile);
                break;
            case Shape2D:
                ActiveAsset->AddNewEdgePlacement(NewEdge);
                ActiveLevel->PopulateSinglePlacement<FEdgePlacement>(NewEdge);
                LastPlacedEdges.Add(NewEdge);
                break;
            case Shape1D:
                ActiveAsset->AddNewPointPlacement(NewPoint);
                ActiveLevel->PopulateSinglePlacement<FPointPlacement>(NewPoint);
                LastPlacedPoints.Add(NewPoint);
                break;
        }
    }
}

/*
*  TODO: potential performance issue...
*  Could have some performance issue when under stressful scene...
*  However, I can not find room to improve...
*/
bool FTiledLevelEdMode::PaintItemPreparation()
{
    if (!ActiveItem) return false;
    if (!IsInsideValidArea) return false;
    if (IsMultiMode)
    {
        FTransform PreviewTransform = Helper->GetPreviewPlacementWorldTransform();
        for (auto P : ActiveAsset->GetActiveFloor()->GetItemPlacements())
        {
            if (P.ItemID != ActiveItem->ItemID) continue;
            FVector V = ActiveLevel->GetActorTransform().TransformPosition(P.TileObjectTransform.GetLocation());
            FRotator R = ActiveLevel->GetActorTransform().TransformRotation(P.TileObjectTransform.Rotator().Quaternion()).Rotator();
            if (V.Equals(PreviewTransform.GetLocation(), 10) && R.Equals(PreviewTransform.Rotator(), 10))
            {
                return false;
            }
        }
        return true;
    }

    switch (CurrentEditShape)
    {
    case Shape3D:
        {
            FTilePlacement TestPlacement;
            TestPlacement.GridPosition = CurrentTilePosition;
            const FIntVector ItemExtent = FIntVector(ActiveItem->Extent);
            TestPlacement.Extent = ShouldRotateBrushExtent? FIntVector(ItemExtent.Y, ItemExtent.X, ItemExtent.Z) : ItemExtent;
            for (auto& F: LastPlacedTiles)
                if (FTiledLevelUtility::IsTilePlacementOverlapping(F, TestPlacement)) return false;

            TArray<FTilePlacement> OverlappingPlacements;
            if (ActiveItem->PlacedType == EPlacedType::Block)
            {
                OverlappingPlacements = ActiveAsset->GetAllBlockPlacements().FilterByPredicate([=](const FTilePlacement& Block)
                {
                    return FTiledLevelUtility::IsTilePlacementOverlapping(TestPlacement, Block);
                });
            }
            else
            {
                OverlappingPlacements = ActiveAsset->GetAllFloorPlacements().FilterByPredicate([=](const FTilePlacement& Floor)
                {
                    return FTiledLevelUtility::IsTilePlacementOverlapping(TestPlacement, Floor);
                });
            }
        
            // check no placement -> pass
            if (OverlappingPlacements.Num() == 0)
            {
                return true;
            }

            for (FTilePlacement Placement : OverlappingPlacements)
            {
                if (Placement.GetItem() == ActiveItem && Placement.GridPosition == CurrentTilePosition)
                {
                    if (FMath::IsNearlyEqual(Placement.TileObjectTransform.Rotator().Yaw, Helper->GetPreviewPlacementWorldTransform().Rotator().Yaw))
                        return false;
                }
            }
        
            // remove overlaps: (1) structure only one (2) prop only one per item
            TArray<FTilePlacement> ExistingTilePlacements;
            if (ActiveItem->StructureType == ETLStructureType::Structure)
            {
                ExistingTilePlacements = OverlappingPlacements.FilterByPredicate([=](const FTilePlacement& Tile)
                {
                    if (Tile.GetItem())
                        return Tile.GetItem()->StructureType == ETLStructureType::Structure;
                    return false;
                });
            } else
            {
                ExistingTilePlacements = OverlappingPlacements.FilterByPredicate([=](const FTilePlacement& Tile)
                {
                    return Tile.GetItem() == ActiveItem;
                });
            }

            ActiveLevel->RemovePlacements(ExistingTilePlacements);
            return true;
        }
    case Shape2D:
        {
            FEdgePlacement TestPlacement;
            TestPlacement.Edge = CurrentEdge;
            TestPlacement.ItemSet = ActiveAsset->GetItemSetAsset();
            TestPlacement.ItemID = ActiveItem->ItemID;
            for (auto& F : LastPlacedEdges)
            {
                if (FTiledLevelUtility::IsEdgePlacementOverlapping(F, TestPlacement))
                {
                    return false;
                }
            }
            TArray<FEdgePlacement> OverlappingPlacements;
            if (ActiveItem->PlacedType == EPlacedType::Wall)
            {
                OverlappingPlacements = ActiveAsset->GetAllWallPlacements().FilterByPredicate([=](const FEdgePlacement& Wall)
                {
                    return FTiledLevelUtility::IsEdgePlacementOverlapping(TestPlacement, Wall);
                });
            }
            else
            {
                OverlappingPlacements = ActiveAsset->GetAllEdgePlacements().FilterByPredicate([=](const FEdgePlacement& Wall)
                {
                    return FTiledLevelUtility::IsEdgePlacementOverlapping(TestPlacement, Wall);
                });
            }
            // Empty wall, pass
            if (OverlappingPlacements.Num() == 0)
            {
                return true;
            }

            TArray<FEdgePlacement> PlacementsToKeep;
            for (const FEdgePlacement& Placement : OverlappingPlacements)
            {
                if (Placement.GetItem() == ActiveItem && Placement.Edge == CurrentEdge)
                {
                    if (FMath::IsNearlyEqual(Placement.TileObjectTransform.Rotator().Yaw,Helper->GetPreviewPlacementWorldTransform().Rotator().Yaw, 5.0))
                        return false;
                    if (IsMultiMode)
                        PlacementsToKeep.Add(Placement);
                }
            }
        
            for (auto P : PlacementsToKeep)
                OverlappingPlacements.Remove(P);

            TArray<FEdgePlacement> ExistingWallPlacements;
            if (ActiveItem->StructureType == ETLStructureType::Structure)
            {
                ExistingWallPlacements = OverlappingPlacements.FilterByPredicate([=](const FEdgePlacement& Wall)
                {
                    return Wall.GetItem()->StructureType == ETLStructureType::Structure;
                });
            } else
            {
                ExistingWallPlacements = OverlappingPlacements.FilterByPredicate([=](const FEdgePlacement& Wall)
                {
                    return Wall.GetItem() == ActiveItem;
                });
            }

            ActiveLevel->RemovePlacements(ExistingWallPlacements);
            return true;
        }
    case Shape1D:
        {
            FPointPlacement TestPlacement;
            TestPlacement.GridPosition = CurrentTilePosition;
            // const FIntVector ItemExtent = FIntVector(ActiveItem->Extent);
            for (auto& F: LastPlacedPoints)
                if (FTiledLevelUtility::IsPointPlacementOverlapping(F, F.GetItem()->Extent.Z, TestPlacement, ActiveItem->Extent.Z)) return false;
            TArray<FPointPlacement> OverlappingPlacements;
            if (ActiveItem->PlacedType == EPlacedType::Pillar)
            {
                OverlappingPlacements = ActiveAsset->GetAllPillarPlacements().FilterByPredicate([=](const FPointPlacement& Pillar)
                {
                    return FTiledLevelUtility::IsPointPlacementOverlapping(TestPlacement, ActiveItem->Extent.Z, Pillar, Pillar.GetItem()->Extent.Z );
                });
            }
            else
            {
                OverlappingPlacements = ActiveAsset->GetAllPointPlacements().FilterByPredicate([=](const FPointPlacement& Point)
                {
                    return FTiledLevelUtility::IsPointPlacementOverlapping(TestPlacement, ActiveItem->Extent.Z, Point, Point.GetItem()->Extent.Z);
                });
            }
        
            // check no placement -> pass
            if (OverlappingPlacements.Num() == 0)
            {
                return true;
            }

            for (FPointPlacement Placement : OverlappingPlacements)
            {
                if (Placement.GetItem() == ActiveItem && Placement.GridPosition == CurrentTilePosition)
                {
                    if (FMath::IsNearlyEqual(Placement.TileObjectTransform.Rotator().Yaw, Helper->GetPreviewPlacementWorldTransform().Rotator().Yaw))
                        return false;
                }
            }
        
            // remove overlaps: (1) structure only one (2) prop only one per item
            TArray<FPointPlacement> ExistingPointPlacements;
            if (ActiveItem->StructureType == ETLStructureType::Structure)
            {
                ExistingPointPlacements = OverlappingPlacements.FilterByPredicate([=](const FPointPlacement& Point)
                {
                    if (Point.GetItem())
                        return Point.GetItem()->StructureType == ETLStructureType::Structure;
                    return false;
                });
            } else
            {
                ExistingPointPlacements = OverlappingPlacements.FilterByPredicate([=](const FPointPlacement& Point)
                {
                    return Point.GetItem() == ActiveItem;
                });
            }

            ActiveLevel->RemovePlacements(ExistingPointPlacements);
            return true;
        }
    }
    return false;
}

void FTiledLevelEdMode::QuickErase()
{
    bool TempHasEraseAny = false;
    if (PaintMode == ETiledLevelPaintMode::Normal)
    {
        switch (CurrentEditShape) {
            case Shape3D:
                TempHasEraseAny = ActiveLevel->EraseSingleItem(CurrentTilePosition, FIntVector(ActiveItem->Extent), ActiveItem->ItemID) > 0;
                break;
            case Shape2D:
                TempHasEraseAny = ActiveLevel->EraseSingleItem(CurrentEdge, FIntVector(ActiveItem->Extent) , ActiveItem->ItemID) > 0;
                break;
            case Shape1D:
                TempHasEraseAny = ActiveLevel->EraseSingleItem(CurrentTilePosition, ActiveItem->Extent.Z, ActiveItem->ItemID) > 0;
                break;
        }
    }
    else
    {
        TempHasEraseAny = ActiveAsset->RemoveAutoPaintDataAt(CurrentTilePosition);
        if (TempHasEraseAny)
            DisplayAutoPaintHint();
    }
    if (TempHasEraseAny)
        HasEraseAnything = true;
}

void FTiledLevelEdMode::EraserStart()
{
    FixedTiledPosition = FIntVector(0);
    if (bIsControlDown)
    {
        FixedTiledPosition = CurrentTilePosition;
        bStraightEdit = true;
    }
    HasEraseAnything = false;
    const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "TiledLevelMode_EraseItemTransaction", "Erase Item Instance"));
    if (IsInRuleAssetEditor)
    {
        ActiveAsset->GetAutoPaintRule()->Modify();
    }
    ActiveAsset->Modify();
    EraseItems();
    ActiveBrushAction = ETiledLevelBrushAction::Erase;
    ActiveAsset->VersionNumber += 1;
}

void FTiledLevelEdMode::EraserEnd()
{
    // TODO: not working here!! it works on PaintEnd, but not here?? why?
    if (!HasEraseAnything)
    {
        UKismetSystemLibrary::CancelTransaction(1);
    }
    if (HasEraseAnything && PaintMode == ETiledLevelPaintMode::Auto)
    {
        UpdateAutoPaintPlacements();
        Helper->SetupEraserBrush(EraserExtent, EPlacedType::Block);
        Helper->MoveBrush(CurrentTilePosition, false);
    }
    ActiveBrushAction = ETiledLevelBrushAction::None;
    bStraightEdit = false;
}

void FTiledLevelEdMode::EraseItems()
{
    bool TempHasEraseAny = false;
    if (PaintMode == ETiledLevelPaintMode::Auto)
    {
        TempHasEraseAny = ActiveAsset->RemoveAutoPaintDataAt(CurrentTilePosition); // TODO: consider case for bigger extent and rotated... so tedious...
        DisplayAutoPaintHint();
        if (TempHasEraseAny) HasEraseAnything = true;
        return;
    }
    switch (EraserTarget)
    {
    case EPlacedType::Block:
        TempHasEraseAny = ActiveLevel->EraseItem(CurrentTilePosition, EraserExtent) > 0;
        break;
    case EPlacedType::Floor:
        TempHasEraseAny = ActiveLevel->EraseItem(CurrentTilePosition, EraserExtent, true) > 0;
        break;
    case EPlacedType::Wall:
        TempHasEraseAny = ActiveLevel->EraseItem(CurrentEdge, EraserExtent ) > 0;
        break;
    case EPlacedType::Edge:
        TempHasEraseAny = ActiveLevel->EraseItem(CurrentEdge, EraserExtent, true ) > 0;
        break;
    case EPlacedType::Pillar:
        TempHasEraseAny = ActiveLevel->EraseItem(CurrentTilePosition, EraserExtent.Z) > 0;
        break;
    case EPlacedType::Point:
        TempHasEraseAny = ActiveLevel->EraseItem(CurrentTilePosition, EraserExtent.Z, true) > 0;
        break;
    case EPlacedType::Any:
        TempHasEraseAny = ActiveLevel->EraseItem_Any(CurrentTilePosition, EraserExtent) > 0;
        break;
    default: ;
    }
    if (TempHasEraseAny) HasEraseAnything = true;
}

void FTiledLevelEdMode::PerformEyedropper()
{
    if (!EyedropperTarget_AutoPaint.IsNone())
    {
        ChangeActiveAutoPaintItem();
        return;
    }
    if (EyedropperTarget == nullptr) return;
    if (UTiledItemSet* NewItemSet = Cast<UTiledItemSet>(EyedropperTarget->GetOuter()))
    {
        if (NewItemSet != ActiveAsset->GetItemSetAsset())
        {
            StaticCastSharedPtr<FTiledLevelEdModeToolkit>(Toolkit)->LoadItemSet(NewItemSet);
            ActiveAsset->SetActiveItemSet(NewItemSet);
        }
    }
    ActiveItem = EyedropperTarget;
    EyedropperTarget = nullptr;
    SwitchToNewTool(ETiledLevelEditTool::Paint);
    if (CurrentEditShape == EPlacedShapeType::Shape2D)
        EnterNewEdge(CurrentEdge);
    else
    {
        EnterNewGrid(CurrentTilePosition);
    }
}

void FTiledLevelEdMode::PerformFillTile()
{
    if (SelectedItems.Num() == 0) return;
    if (CandidateFillTiles.Num() == 0) return;

    // make transaction
    const FScopedTransaction Transaction(LOCTEXT("TiledLevelMode_FillTilesTransaction", "Fill Tiles"));
    ActiveAsset->Modify();
    
    TArray<FIntVector> RegionToEmpty;
    // remove existing tile placements for edge as boundary case
    if (!IsTileAsFillBoundary)
    {
        for (auto P : CandidateFillTiles)
            RegionToEmpty.Add(FIntVector(P.X, P.Y, ActiveAsset->ActiveFloorPosition));    
    }
    // remove across floor placements
    if (MaxZInFillItems > 1)
    {
        for (int FloorPosition = ActiveAsset->ActiveFloorPosition + 1; FloorPosition < ActiveAsset->ActiveFloorPosition + MaxZInFillItems ;FloorPosition++)
        {
            for (auto P : CandidateFillTiles)
                RegionToEmpty.Add(FIntVector(P.X, P.Y, FloorPosition));
        }
    }
    ActiveAsset->EmptyRegionData(RegionToEmpty);
    ActiveLevel->ResetAllInstance(true);
    
    // perform fill
      // handle normal fill item
    float TotalWeights = 0.f;
    TArray<float> FillItemCoefficients;
    TArray<UTiledLevelItem*> FillItems;
    for (UTiledLevelItem* Item : SelectedItems)
    {
        if (Item->bAllowOverlay) continue;
        FillItemCoefficients.Add(Item->FillCoefficient);
        FillItems.Add(Item);
        TotalWeights += Item->FillCoefficient;
    }
    TArray<float> WeightedCoefficients = FTiledLevelUtility::GetWeightedCoefficient(FillItemCoefficients);

    int RandRotationIndex = 0;
    FIntPoint OutPoint = {0, 0};
    float RandValue = 0.f;

    // insert gaps
    if (EnableFillGap)
    {
        float GapRatio = GapCoefficient / (TotalWeights + GapCoefficient);
        CreateEmptyGaps<FIntPoint>(CandidateFillTiles, GapRatio);
    }
    
    TArray<FIntPoint> PointsToFill = CandidateFillTiles; // make a copy
    
    bool CanPutFillItemInBoard = true;
    // fill normal items
    while (PointsToFill.Num() > 0 && CanPutFillItemInBoard)
    {
        RandValue = FMath::FRand();
        for (int i = 0; i < FillItems.Num(); i++)
        {
            if (WeightedCoefficients[i] < RandValue) continue;
            RandRotationIndex = FillItems[i]->bAllowRandomRotation? FMath::RandRange(0, 3) : 0;
            
            if (FTiledLevelUtility::GetFeasibleFillTile(FillItems[i], RandRotationIndex, PointsToFill, OutPoint))
            {
                // create placement data
                FTilePlacement NewTile;
                NewTile.ItemSet = ActiveAsset->GetItemSetAsset();
                NewTile.ItemID = FillItems[i]->ItemID;
                NewTile.GridPosition = FIntVector(OutPoint.X, OutPoint.Y, ActiveAsset->ActiveFloorPosition);
                NewTile.Extent = ShouldRotateBrushExtent? FIntVector(FillItems[i]->Extent.Y, FillItems[i]->Extent.X, FillItems[i]->Extent.Z) : FIntVector(FillItems[i]->Extent);
                Helper->ResetBrush();
                Helper->SetupPaintBrush(FillItems[i]);
                Helper->MoveBrush(FIntVector(OutPoint.X, OutPoint.Y, ActiveAsset->ActiveFloorPosition), bSnapEnabled);
                bool temp;
                for (int r = 0; r < RandRotationIndex; r++)
                    Helper->RotateBrush(true, FillItems[i]->PlacedType, temp);
                FTransform G = Helper->GetPreviewPlacementWorldTransform();
                NewTile.TileObjectTransform = G.GetRelativeTransform(ActiveLevel->GetTransform());
                
                // populate instance and add new placement data
                ActiveAsset->AddNewTilePlacement(NewTile);
                ActiveLevel->PopulateSinglePlacement(NewTile); // must still populate placement one by one in order to make auto-snap work...
            }
            else
            {
                // Change the weight to skip it during random choose
                FillItemCoefficients[i] = 0.000001;
                WeightedCoefficients = FTiledLevelUtility::GetWeightedCoefficient(FillItemCoefficients);
                
                // if all weight is set to near 0, that means no enough space for any items, break the while loop
                float SumOfRawWeight = 0.f;
                for (float& Value : FillItemCoefficients)
                    SumOfRawWeight += Value;
                CanPutFillItemInBoard = SumOfRawWeight >= 0.001;
            }
            break;
        }
    }
    
     // handle overlay items
    TArray<UTiledLevelItem*> OverlayItems = SelectedItems.FilterByPredicate([=](const UTiledLevelItem* Item)
    {
        return Item->bAllowOverlay;
    });
    
    for (UTiledLevelItem* Item : OverlayItems)
    {
        PointsToFill = CandidateFillTiles;
        while (PointsToFill.Num() > 0)
        {
            RandValue = FMath::FRand();
            if (Item->FillCoefficient < RandValue)
            {
                PointsToFill.RemoveAt(0);
                continue;
            }
            RandRotationIndex = Item->bAllowRandomRotation? FMath::RandRange(0, 3) : 0;
            if (FTiledLevelUtility::GetFeasibleFillTile(Item, RandRotationIndex, PointsToFill, OutPoint))
            {
                // create placement data
                FTilePlacement NewTile;
                NewTile.ItemSet = ActiveAsset->GetItemSetAsset();
                NewTile.ItemID = Item->ItemID;
                NewTile.IsMirrored = IsMirrored_X || IsMirrored_Y || IsMirrored_Z;
                NewTile.GridPosition = FIntVector(OutPoint.X, OutPoint.Y, ActiveAsset->ActiveFloorPosition);
                NewTile.Extent = ShouldRotateBrushExtent? FIntVector(Item->Extent.Y, Item->Extent.X, Item->Extent.Z) : FIntVector(Item->Extent);
                Helper->ResetBrush();
                Helper->SetupPaintBrush(Item);
                Helper->MoveBrush(FIntVector(OutPoint.X, OutPoint.Y, ActiveAsset->ActiveFloorPosition), bSnapEnabled);
                bool temp;
                for (int r = 0; r < RandRotationIndex; r++)
                    Helper->RotateBrush(true, Item->PlacedType, temp);
                FTransform G = Helper->GetPreviewPlacementWorldTransform();
                NewTile.TileObjectTransform = G.GetRelativeTransform(ActiveLevel->GetTransform());
                
                // populate instance and add new placement data
                ActiveAsset->AddNewTilePlacement(NewTile);
                ActiveLevel->PopulateSinglePlacement(NewTile);
            }
        }
    }
    Helper->ResetBrush();
}

void FTiledLevelEdMode::PerformFillEdge()
{
    if (SelectedItems.Num() == 0) return;
    if (CandidateFillEdges.Num() == 0) return;

    // make transaction
    const FScopedTransaction Transaction(LOCTEXT("TiledLevelMode_FillEdgesTransaction", "Fill Edges"));
    ActiveAsset->Modify();

    TArray<FTiledLevelEdge> EdgeRegionsToEmpty;

    for (int FloorPosition = ActiveAsset->ActiveFloorPosition; FloorPosition < ActiveAsset->ActiveFloorPosition + MaxZInFillItems ;FloorPosition++)
    {
        for (auto e : CandidateFillEdges)
            EdgeRegionsToEmpty.Add(FTiledLevelEdge(e.X, e.Y, FloorPosition, e.EdgeType));
    }
    
    // remove across floor existing edge placements
    ActiveAsset->EmptyEdgeRegionData(EdgeRegionsToEmpty);
    ActiveLevel->ResetAllInstance(true);
    

    // perform fill
      // handle normal fill item
    float TotalWeights = 0.f;
    TArray<float> FillItemCoefficients;
    TArray<float> WeightedCoefficients;
    TArray<UTiledLevelItem*> FillItems;
    for (UTiledLevelItem* Item : SelectedItems)
    {
        if (Item->bAllowOverlay) continue;
        FillItemCoefficients.Add(Item->FillCoefficient);
        FillItems.Add(Item);
        TotalWeights += Item->FillCoefficient;
    }
    WeightedCoefficients = FTiledLevelUtility::GetWeightedCoefficient(FillItemCoefficients);

    bool ShouldRotate = false;
    FTiledLevelEdge OutEdge;
    float RandValue = 0.f;

    // insert gap
    if (EnableFillGap)
    {
        float GapRatio = GapCoefficient / (GapCoefficient + TotalWeights);
        CreateEmptyGaps<FTiledLevelEdge>(CandidateFillEdges, GapRatio);
    }
    
    TArray<FTiledLevelEdge> EdgesToFill = CandidateFillEdges; // make a copy
    
    bool CanPutFillItemInBoard = true;
    // fill normal items
    while (EdgesToFill.Num() > 0 && CanPutFillItemInBoard)
    {
        RandValue = FMath::FRand();
        for (int i = 0; i < FillItems.Num(); i++)
        {
            if (WeightedCoefficients[i] < RandValue) continue;
            ShouldRotate = FillItems[i]->bAllowRandomRotation? FMath::RandBool() : false;
            
            if (FTiledLevelUtility::GetFeasibleFillEdge(FillItems[i], EdgesToFill, OutEdge))
            {
                // create placement data
                FEdgePlacement NewEdge;
                NewEdge.ItemSet = ActiveAsset->GetItemSetAsset();
                NewEdge.ItemID = FillItems[i]->ItemID;
                NewEdge.Edge = OutEdge;
                Helper->ResetBrush();
                Helper->SetupPaintBrush(FillItems[i]);
                Helper->MoveBrush(OutEdge, bSnapEnabled);
                bool temp;
                if (OutEdge.EdgeType==EEdgeType::Vertical)
                    Helper->RotateBrush(true, FillItems[i]->PlacedType, temp);
                if (ShouldRotate)
                {
                    Helper->RotateBrush(true, FillItems[i]->PlacedType, temp);
                    Helper->RotateBrush(true, FillItems[i]->PlacedType, temp);
                }
                FTransform G = Helper->GetPreviewPlacementWorldTransform();
                NewEdge.TileObjectTransform = G.GetRelativeTransform(ActiveLevel->GetTransform());
                // populate instance and add new placement data
                ActiveAsset->AddNewEdgePlacement(NewEdge);
                ActiveLevel->PopulateSinglePlacement(NewEdge);
            }
            else
            {
                // Change the weight to skip it during random choose
                FillItemCoefficients[i] = 0.000001;
                WeightedCoefficients = FTiledLevelUtility::GetWeightedCoefficient(FillItemCoefficients);
                
                // if all weight is set to near 0, that means no enough space for any items, break the while loop
                float SumOfRawWeight = 0.f;
                for (float& Value : FillItemCoefficients)
                    SumOfRawWeight += Value;
                CanPutFillItemInBoard = SumOfRawWeight >= 0.001;
            }
            break;
        }
    }
    
     // handle overlay items
    TArray<UTiledLevelItem*> OverlayItems = SelectedItems.FilterByPredicate([=](const UTiledLevelItem* Item)
    {
        return Item->bAllowOverlay;
    });
    for (UTiledLevelItem* Item : OverlayItems)
    {
        EdgesToFill = CandidateFillEdges;
        while (EdgesToFill.Num() > 0)
        {
            RandValue = FMath::FRand();
            if (Item->FillCoefficient < RandValue)
            {
                EdgesToFill.RemoveAt(0);
                continue;
            }
            ShouldRotate = Item->bAllowRandomRotation? FMath::RandBool() : false;
            if (FTiledLevelUtility::GetFeasibleFillEdge(Item, EdgesToFill, OutEdge))
            {
                // create placement data
                FEdgePlacement NewEdge;
                NewEdge.ItemSet = ActiveAsset->GetItemSetAsset();
                NewEdge.ItemID = Item->ItemID;
                NewEdge.Edge = OutEdge;
                Helper->ResetBrush();
                Helper->SetupPaintBrush(Item);
                Helper->MoveBrush(OutEdge, bSnapEnabled);
                bool temp;
                if (OutEdge.EdgeType==EEdgeType::Vertical)
                    Helper->RotateBrush(true, Item->PlacedType, temp);
                if (ShouldRotate)
                {
                    Helper->RotateBrush(true, Item->PlacedType, temp);
                    Helper->RotateBrush(true, Item->PlacedType, temp);
                }
                FTransform G = Helper->GetPreviewPlacementWorldTransform();
                NewEdge.TileObjectTransform = G.GetRelativeTransform(ActiveLevel->GetTransform());
                
                // populate instance and add new placement data
                ActiveAsset->AddNewEdgePlacement(NewEdge);
                ActiveLevel->PopulateSinglePlacement(NewEdge);
            }
        }
    }
    Helper->ResetBrush();
    
}

void FTiledLevelEdMode::SetupFillBoardFromTiles()
{
    // Init board
    Board.Empty();
    TArray<int> Zeros;
    Zeros.Init(0, ActiveAsset->Y_Num);
    for (int i = 0; i < ActiveAsset->X_Num; i++)
        Board.Add(Zeros);
    // fill in board value by tile placements
    for (auto Block : ActiveAsset->GetActiveFloor()->GetTilePlacements())
    {
        int tile_x = Block.GridPosition.X;
        int tile_y = Block.GridPosition.Y;
        for (int x = tile_x; x < tile_x + Block.Extent.X; x++)
        {
            for (int y = tile_y; y < tile_y + Block.Extent.Y; y++)
            {
                Board[x][y] = 1;
            } 
        }
    }
    // fill space for ground
    if (NeedGround && IsFillTiles)
    {
        UpdateFillBoardFromGround(); 
    }
}

void FTiledLevelEdMode::UpdateFillBoardFromGround()
{
    if (FTiledFloor* BelowFloor = ActiveAsset->GetBelowActiveFloor())
    {
        TArray<FIntPoint> ExistingBelowTiles;
        for (auto Block : BelowFloor->GetTilePlacements())
        {
            int tile_x = Block.GridPosition.X;
            int tile_y = Block.GridPosition.Y;
            for (int x = tile_x; x < tile_x + Block.Extent.X; x++)
            {
                for (int y = tile_y; y < tile_y + Block.Extent.Y; y++)
                {
                    ExistingBelowTiles.AddUnique(FIntPoint(x, y));
                } 
            }
        }
        for (int x = 0; x < ActiveAsset->X_Num; x++)
        {
            for (int y = 0; y < ActiveAsset->Y_Num; y++)
            {
                if (!ExistingBelowTiles.Contains(FIntPoint(x, y)))
                    Board[x][y] = 1;
            } 
        }
    }
}

template <typename T>
void FTiledLevelEdMode::CreateEmptyGaps(TArray<T>& InCandidates, float Ratio)
{
    TArray<T> ToRemove;
    for (auto p : InCandidates)
        if (FMath::FRand() < Ratio)
            ToRemove.Add(p);
    for (auto p : ToRemove)
        InCandidates.Remove(p);
}


void FTiledLevelEdMode::UpdateStatistics()
{
	PerFloorInstanceCount.Empty();
	PerItemInstanceCount.Empty();
	TotalInstanceCount = 0;
    ExistingItems.Empty() ;
	for (auto& F : ActiveAsset->TiledFloors)
	{
		uint32 N = F.GetItemPlacements().Num();
		PerFloorInstanceCount.Emplace(F.FloorPosition, N);
		TotalInstanceCount += N;
		for (auto& P : F.GetItemPlacements())
		{
			if (PerItemInstanceCount.Contains(P.ItemID))
			{
				PerItemInstanceCount[P.ItemID] += 1;
			} else
			{
				PerItemInstanceCount.Emplace(P.ItemID, 1);
			}
		    ExistingItems.Add(P.GetItem());
		}
	}
    // Assets using this item is total not necessary now... every asset use its own item set
    // for (UTiledLevelItem* Item: ExistingItems)
    // {
    //     if (Item)
    //         Item->AssetsUsingThisItem.AddUnique(ActiveAsset);
    // }
    // if (UTiledItemSet* ItemSet = Cast<UTiledItemSet>(StaticCastSharedPtr<FTiledLevelEdModeToolkit>(Toolkit)->GetCurrentItemSet()))
    // {
    //     for (UTiledLevelItem* Item : ItemSet->GetItemSet())
    //     {
    //         if (!ExistingItems.Contains(Item))
    //             Item->AssetsUsingThisItem.Remove(ActiveAsset);
    //     }
    // }
}

int32 FTiledLevelEdMode::GetNumEraserActiveItems() const
{
    int Out = 0;
    if (ActiveAsset)
    {
        for (UTiledLevelItem* Item : ActiveAsset->GetItemSet())
            if (Item->bIsEraserAllowed) Out += 1;
    }
    return Out;
}

UTiledLevelItem* FTiledLevelEdMode::FindItemFromMeshPtr(UStaticMesh* MeshPtr)
{
    for (UTiledLevelItem* Item : ExistingItems)
    {
        if (Item->SourceType == ETLSourceType::Mesh)
            if (Item->TiledMesh == MeshPtr)
                return Item;
    }
    return nullptr;
}

void FTiledLevelEdMode::ClearAutoData(FName ItemName, bool IsSelectedFloorOnly)
{
    FScopedTransaction Transaction(LOCTEXT("ClearAutoPaintItemData", "Clear auto paint item data"));
    ActiveAsset->Modify();
    TArray<FIntVector> PosToRemove;
    int FloorPosition = ActiveAsset->GetActiveFloor()->FloorPosition;
    for (auto [p, n] : ActiveAsset->GetAutoPaintData())
    {
        if (IsSelectedFloorOnly)    
        {
            if (n == ItemName &&  p.Z == FloorPosition)
                PosToRemove.Add(p);
        }
        else
        {
            if (n == ItemName)
                PosToRemove.Add(p);
        }
    }
    for (auto P : PosToRemove)
        ActiveAsset->GetAutoPaintData().Remove(P);
    UpdateAutoPaintPlacements();
    DisplayAutoPaintHint();
}

void FTiledLevelEdMode::ChangeActiveAutoPaintItem()
{
    if (UAutoPaintRule* Rule = ActiveAsset->GetAutoPaintRule())
    {
        GetAutoPalettePtr()->SelectItem(EyedropperTarget_AutoPaint);
        Rule->ActiveItemName = EyedropperTarget_AutoPaint;
    }
    EyedropperTarget_AutoPaint=TEXT("");
    SwitchToNewTool(ETiledLevelEditTool::Paint);
    EnterNewGrid(CurrentTilePosition);
}

#undef LOCTEXT_NAMESPACE
