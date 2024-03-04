// Copyright 2022 Greg Shynkar. All Rights Reserved

#pragma once

#include "Graphs/DialogueBuilderEdGraph.h"
#include "EdGraphNode_Comment.h"
#include "EdGraph/EdGraphNode.h"
#include "CommentNodeDB.generated.h"


UCLASS(MinimalAPI)
class UCommentNodeDB : public UEdGraphNode
{
	GENERATED_BODY()
public:

	UCommentNodeDB(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~ Begin UObject Interface
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	//~ End UObject Interface

	//~ Begin UEdGraphNode Interface
	virtual void ResizeNode(const FVector2D& NewSize) override;
	virtual void OnRenameNode(const FString& NewName) override;
	//~ End UEdGraphNode Interface

	UPROPERTY(EditAnywhere, Category= Comment)
	FString CommentText = "Comment Text";

	/** Size of the text in the comment box */
	UPROPERTY(EditAnywhere, Category = Comment, meta = (ClampMin = 1, ClampMax = 1000))
	int32 FontSize;

	/** Color to style comment with */
	UPROPERTY(EditAnywhere, Category = Comment)
	FLinearColor CommentColor = FLinearColor(1.f, 1.f, 1.f, 0.5f);

	/** comment Depth */
	UPROPERTY()
	int32 CommentDepth;

};
