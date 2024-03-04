// Copyright 2020-2024 Maksym Maisak. All Rights Reserved.

#pragma once

#include "GameplayTagsManager.h"
#include "Math/Range.h"
#include "Misc/StringBuilder.h"

struct FGameplayTag;
class UHTNComponent;
class UWorldStateProxy;
class UBlackboardData;

namespace FHTNHelpers
{
	// When recursively pretty-printing a struct, some structs (e.g. FVector) should be displayed in one line without separating the members.
	// This is a list of such structs for convenience.
	HTN_API TArrayView<UScriptStruct* const> GetStructsToPrintDirectly();

	HTN_API UHTNComponent* GetHTNComponent(AActor* Target);

	HTN_API UWorldStateProxy* GetWorldStateProxy(AActor* Target, bool bIsPlanning);

	HTN_API void ForGameplayTagAndChildTags(const FGameplayTag& GameplayTag, TFunctionRef<void(const FGameplayTag&)> Callable);

	// Goes through all properties of the object's class and its superclasses (up to StopAtClass)
	// and initializes FBlackboardKeySelector properties with the given BlackboardAsset.
	// Traverses struct properties recursively, so FHTNBlackboardKeySelectors inside struct properties are also resolved.
	// Logs warning and invalidates selectors if the provided asset is null.
	HTN_API void ResolveBlackboardKeySelectors(UObject& Object, const UClass& StopAtClass, const UBlackboardData* BlackboardAsset);

	HTN_API FString CollectPropertyDescription(const UObject* Ob, const UClass* StopAtClass, TArrayView<FProperty* const> InPropertiesToSkip);
	HTN_API FString CollectPropertyDescription(const UObject* Ob, const UClass* StopAtClass, TFunctionRef<bool(FProperty* /*TestProperty*/)> ShouldSkipProperty);

	// Describes a TRange (e.g. FFloatRange) in a form like "(-inf, 5]".
	template<typename ValueType>
	FString DescribeRange(const TRange<ValueType>& Range)
	{
		const TRangeBound<ValueType> LowerBound = Range.GetLowerBound();
		const TRangeBound<ValueType> UpperBound = Range.GetUpperBound();

		TStringBuilder<256> SB;

		if (LowerBound.IsOpen())
		{
			SB << TEXT("(-inf");
		}
		else
		{
			SB << (LowerBound.IsInclusive() ? TEXT("[") : TEXT("("));
			SB << FString::SanitizeFloat(LowerBound.GetValue());
		}

		SB << TEXT(", ");

		if (UpperBound.IsOpen())
		{
			SB << TEXT("inf)");
		}
		else
		{
			SB << FString::SanitizeFloat(UpperBound.GetValue());
			SB << (UpperBound.IsInclusive() ? TEXT("]") : TEXT(")"));
		}

		return SB.ToString();
	}
}
