// Copyright 2022 Greg Shynkar. All Rights Reserved

#include "DialogueBuilderObject.h"


UDialogueBuilderObject::UDialogueBuilderObject(const FObjectInitializer & ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
}

void UDialogueBuilderObject::PostDuplicate(bool bDuplicateForPIE)
{
	IsJustDuplicated = true;
}


void UDialogueBuilderObject::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UDialogueBuilderObject, Test);
}


FDialogueDetailsStruct UDialogueBuilderObject::StartDialogue()
{
    // Find ID number 0, Entry node should be always 0;
	if (DialoguesData.Num() > 0)
	{
		if (DialoguesData[0].NextNodesId.Num()>0)
		{
			int32 IndexToReturn = GetListIndexFromNodeID(DialoguesData[0].NextNodesId[0]);

			// Update with every proceed
			UFunction* UpdateAnimation = FindFunction("UpdateAnimationForDialogue");
			if (UpdateAnimation)
			{
				struct StructLoc2
				{
					FDialogueDetailsStruct DialogueNodeDetails;
				};

				StructLoc2 ResultStructureLoc2;
				ResultStructureLoc2.DialogueNodeDetails = DialoguesData[IndexToReturn];

				ProcessEvent(UpdateAnimation, &ResultStructureLoc2);
			}
			
			UFunction* CustFuncToCall = FindFunction(DialoguesData[IndexToReturn].OnDialoguePlayFuncName);
			if (CustFuncToCall)
			{
				struct StructTest
				{
					float delay;
				};

				StructTest ResultStructure;
				ResultStructure.delay = DialoguesData[IndexToReturn].BeginPlayDelay;

				ProcessEvent(CustFuncToCall, &ResultStructure);
			}

			return DialoguesData[IndexToReturn];
		}
		else
		{
			return DialoguesData[0];
		}
	}
	else
	{ 
		// If Entry Node doesnot exist;
		FDialogueDetailsStruct StartStructure;
		StartStructure.DialogueNodeText = FText::FromString("NoDataFound");
		StartStructure.NodeId = 0;
		DialoguesData.Add(StartStructure);
		return DialoguesData[0];
	}
}

TArray<FDialogueDetailsStruct> UDialogueBuilderObject::StartDialogueFromNodeID(int32 NodeID)
{
	TArray<FDialogueDetailsStruct> LocFirstArray;
	TArray<FDialogueDetailsStruct> ReturnArray;

	//Get Index from DialogueData Array;
	int32 ListIndex = GetListIndexFromNodeID(NodeID);

	if (ListIndex!=-1)
	{
		for (int32 x = 0; x < DialoguesData[ListIndex].NextNodesId.Num(); ++x)
		{
			// Check that object is exist inside DialogueData Array;
			int32 NextNodeListIndex = GetListIndexFromNodeID(DialoguesData[ListIndex].NextNodesId[x]);

			if (NextNodeListIndex != -1)
			{
				if (DialoguesData[ListIndex].TransitionNameToCheck[x] == "True")
				{
					ReturnArray.Add(DialoguesData[NextNodeListIndex]);
				}
				else
				{
					if (IsPossibleToMoveFurther(ListIndex, x))
					{
						ReturnArray.Add(DialoguesData[NextNodeListIndex]);
					}
				}
			}
		}
	}

	//Sorting Nodes from Left To Right;
	ReturnArray.Sort([](const FDialogueDetailsStruct& a, const FDialogueDetailsStruct& b) { return a.NodePosition.X < b.NodePosition.X; });

	// Check Transition Rules From Node To Node;
	for (int32 x=0;x< ReturnArray.Num();++x)
	{
		UFunction* CustFuncToCall = FindFunction(ReturnArray[x].OnDialoguePlayFuncName);
		if (CustFuncToCall)
		{
			struct StructTest
			{
				float delay;
			};

			StructTest ResultStructure;
			ResultStructure.delay = ReturnArray[x].BeginPlayDelay;

			ProcessEvent(CustFuncToCall, &ResultStructure);
		}
		
		
		UFunction* UpdateAnimation = FindFunction("UpdateAnimationForDialogue");
		if (UpdateAnimation)
		{
			struct StructLoc2
			{
				FDialogueDetailsStruct DialogueNodeDetails;
			};

			StructLoc2 ResultStructureLoc2;
			ResultStructureLoc2.DialogueNodeDetails = ReturnArray[x];

			ProcessEvent(UpdateAnimation, &ResultStructureLoc2);
		}
	}			


	return ReturnArray;
}
	
TArray<FDialogueDetailsStruct> UDialogueBuilderObject::MoveToNextNodesFromNodeID(int32 CurrentNodeID)
{
	return StartDialogueFromNodeID(CurrentNodeID);
}

bool UDialogueBuilderObject::MarkNodeasAlreadyPlayed(int32 NodeID)
{
	int32 IndexToReturn = GetListIndexFromNodeID(NodeID);

	if (IndexToReturn!=-1)
	{
		DialoguesData[IndexToReturn].IsAlreadyPlayed=true;
		return true;
	}
	else
	{
		return false;
	}
}

int32 UDialogueBuilderObject::GetListIndexFromNodeID(int32 NodeID)
{
	for (int32 x = 0; x < DialoguesData.Num(); ++x)
	{
		if (DialoguesData[x].NodeId != -1 && DialoguesData[x].NodeId==NodeID)
		{
			return x;
			break;
		}
	}
	return -1;
}

bool UDialogueBuilderObject::IsPossibleToMoveFurther(int32 NodeListIndex,int32 FunctionNameIndex)
{
	UFunction* FunctionToCheck= FindFunction(DialoguesData[NodeListIndex].TransitionNameToCheck[FunctionNameIndex]);
	
	if (FunctionToCheck)
	{
		struct StructTest
		{
			bool bresult;
		};

		StructTest ResultStructure;
	
		ProcessEvent(FunctionToCheck, &ResultStructure);
		return ResultStructure.bresult;
	}

	return false;
}

void UDialogueBuilderObject::AddDialogueDataElement(FDialogueDetailsStruct DialoguesElement)
{
	DialoguesData.Add(DialoguesElement);
	Modify();	
}

bool UDialogueBuilderObject::RemoveDataElement(int32 ArrayElementIndex)
{
	if (DialoguesData.IsValidIndex(ArrayElementIndex))
	{
		DialoguesData.RemoveAt(ArrayElementIndex);
		Modify();
		return true;
	}
	return false;
}

bool UDialogueBuilderObject::SetDataElement(int32 DataElementIndex, FDialogueDetailsStruct NewElement)
{
	if (DialoguesData.IsValidIndex(DataElementIndex))
	{
		DialoguesData[DataElementIndex] = NewElement;

		Modify();
		return true;
	}

	return false;
}

void UDialogueBuilderObject::DebugGetAllIndexes()
{

	for (int32 x=0; x< DialoguesData.Num(); x++)
	{
		UE_LOG(LogTemp, Warning, TEXT(" DebugObject : DialogueID %d"), DialoguesData[x].NodeId);

		for (int32 y=0; y< DialoguesData[x].NextNodesId.Num();++y)
		{
			if (DialoguesData[x].TransitionNameToCheck.IsValidIndex(y))
			{
				UE_LOG(LogTemp, Warning, TEXT(" DebugObject : DialogueID %d | Connect link to %d | FunctionName %s "),

				DialoguesData[x].NodeId, DialoguesData[x].NextNodesId[y], *DialoguesData[x].TransitionNameToCheck[y].ToString());
				UFunction*TestFunc = FindFunction(DialoguesData[x].TransitionNameToCheck[y]);
				if (TestFunc)
				{
					struct StructTest
					{
						bool result;
					};
					StructTest test;

					ProcessEvent(TestFunc, &test);
					UE_LOG(LogTemp, Warning, TEXT("The boolean value is %s"), (test.result ? TEXT("true") : TEXT("false")));
				}
			}
			if (x==0 && DialoguesData[x].NextNodesId.Num()>0)
			{
				UE_LOG(LogTemp, Warning, TEXT(" DebugObject : DialogueID %d | Connect link to %d "), DialoguesData[x].NodeId, DialoguesData[x].NextNodesId[0]);
			}
		}
	}
}

