// Copyright 2020-2024 Maksym Maisak. All Rights Reserved.

#include "Utility/HTNHelpers.h"

#include "GameplayTagsManager.h"
#include "HTNComponent.h"

#include "AIController.h"
#include "Algo/Accumulate.h"
#include "Algo/NoneOf.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "GameplayTagContainer.h"
#include "Misc/EngineVersionComparison.h"
#include "String/ParseLines.h"
#include "UObject/TextProperty.h"

TArrayView<UScriptStruct* const> FHTNHelpers::GetStructsToPrintDirectly()
{
	static const TArray<UScriptStruct*> StructsToPrintDirectly
	{
		TBaseStructure<FVector2D>::Get(),
		TBaseStructure<FVector>::Get(),
		TBaseStructure<FQuat>::Get(),

#if !UE_VERSION_OLDER_THAN(5, 1, 0)
		TBaseStructure<FVector4>::Get(),
		TBaseStructure<FIntPoint>::Get(),
		TBaseStructure<FIntVector>::Get(),
		TBaseStructure<FIntVector4>::Get(),
#endif

		TBaseStructure<FRotator>::Get(),
		TBaseStructure<FColor>::Get(),
		TBaseStructure<FLinearColor>::Get()
	};

	return StructsToPrintDirectly;
}

namespace
{
	FString JoinElementDescriptionsCommaSeparated(const FString& StartSeparator, const FString& EndSeparator, TArrayView<const FString> ElementDescriptions)
	{
		constexpr int32 MaxLineLength = 60;

		TStringBuilder<2048> SB;
		SB << StartSeparator;

		const FString SingleLineSeparator = TEXT(", ");
		const bool bMultiLine =
			MaxLineLength < Algo::TransformAccumulate(ElementDescriptions, &FString::Len, 0) + SingleLineSeparator.Len() * (ElementDescriptions.Num() - 1) ||
			ElementDescriptions.ContainsByPredicate([](const FString& ElementDescription) { return ElementDescription.Contains(TEXT("\n")); });
		if (bMultiLine)
		{
			bool bFirst = true;
			for (const FString& SubPropertyDescription : ElementDescriptions)
			{
				if (!bFirst)
				{
					SB << TEXT(",\n");
				}
				else
				{
					SB << TEXT("\n");
					bFirst = false;
				}

				bool bAnyLines = false;
				UE::String::ParseLines(SubPropertyDescription, [&](FStringView Line)
				{
					bAnyLines = true;
					SB << TEXT("    ") << Line << TEXT("\n");
				});
				if (bAnyLines)
				{
					// Remove the last newline
					SB.RemoveSuffix(1);
				}
			}
		}
		else
		{
			bool bFirst = true;
			for (const FString& SubPropertyDescription : ElementDescriptions)
			{
				if (!bFirst)
				{
					SB << SingleLineSeparator;
				}
				else
				{
					bFirst = false;
				}
				SB << SubPropertyDescription;
			}
		}

		SB << EndSeparator;

		return SB.ToString();
	}

	FString DescribeProperty(const FProperty* Prop, const uint8* PropertyAddr, bool bWithName = true)
	{
		FString ExportedStringValue;
		if (!Prop)
		{
			return ExportedStringValue;
		}

		const FStructProperty* const StructProp = CastField<const FStructProperty>(Prop);
		const FNumericProperty* const NumericProp = CastField<const FNumericProperty>(Prop);

		// Special case for objects and classes
		if (const FObjectPropertyBase* const ObjectPropertyBase = CastField<const FObjectPropertyBase>(Prop))
		{
			const UObject* const Object = ObjectPropertyBase->GetObjectPropertyValue(PropertyAddr);
			ExportedStringValue = GetNameSafe(Object);
			if (Cast<UClass>(Object))
			{
				ExportedStringValue.RemoveFromEnd(TEXT("_C"));
			}
		}
		else if (const FArrayProperty* const ArrayProperty = CastField<const FArrayProperty>(Prop))
		{
			TArray<FString, TInlineAllocator<16>> ArrayElementDescriptions;
			FScriptArrayHelper ArrayHelper(ArrayProperty, PropertyAddr);
			for (int32 Index = 0; Index < ArrayHelper.Num(); Index++)
			{
				ArrayElementDescriptions.Emplace(DescribeProperty(ArrayProperty->Inner, ArrayHelper.GetRawPtr(Index), /*bWithName=*/false));
			}

			ExportedStringValue = JoinElementDescriptionsCommaSeparated(TEXT("["), TEXT("]"), ArrayElementDescriptions);
		}
		else if (const FSetProperty* const SetProperty = CastField<const FSetProperty>(Prop))
		{
			TArray<FString, TInlineAllocator<16>> SetElementDescriptions;
			FScriptSetHelper SetHelper(SetProperty, PropertyAddr);
			for (int32 Index = 0; Index < SetHelper.Num(); Index++)
			{
				SetElementDescriptions.Emplace(DescribeProperty(SetProperty->ElementProp, SetHelper.GetElementPtr(Index), /*bWithName=*/false));
			}

			ExportedStringValue = JoinElementDescriptionsCommaSeparated(TEXT("{"), TEXT("}"), SetElementDescriptions);
		}
		else if (const FMapProperty* const MapProperty = CastField<const FMapProperty>(Prop))
		{
			TArray<FString, TInlineAllocator<16>> MapElementDescriptions;
			FScriptMapHelper MapHelper(MapProperty, PropertyAddr);
			for (int32 Index = 0; Index < MapHelper.Num(); Index++)
			{
				FString& ElementDescription = MapElementDescriptions.AddDefaulted_GetRef();
				ElementDescription += DescribeProperty(MapProperty->KeyProp, MapHelper.GetKeyPtr(Index), /*bWithName=*/false);
				ElementDescription += TEXT(": ");
				ElementDescription += DescribeProperty(MapProperty->ValueProp, MapHelper.GetValuePtr(Index), /*bWithName=*/false);
			}

			ExportedStringValue = JoinElementDescriptionsCommaSeparated(TEXT("{"), TEXT("}"), MapElementDescriptions);
		}
		else if (StructProp && StructProp->Struct && !FHTNHelpers::GetStructsToPrintDirectly().Contains(StructProp->Struct))
		{
			// Special case for blackboard key selectors
			if (StructProp->Struct->IsChildOf(TBaseStructure<FBlackboardKeySelector>::Get()))
			{
				const FBlackboardKeySelector* const PropertyValue = reinterpret_cast<const FBlackboardKeySelector*>(PropertyAddr);
				ExportedStringValue = PropertyValue->SelectedKeyName.ToString();
			}
			// Special case for gameplay tags
			else if (StructProp->Struct->IsChildOf(TBaseStructure<FGameplayTag>::Get()))
			{
				ExportedStringValue = reinterpret_cast<const FGameplayTag*>(PropertyAddr)->ToString();

#if WITH_EDITOR
				const FString CategoryLimit = StructProp->GetMetaData(TEXT("Categories"));
				if (!CategoryLimit.IsEmpty() && ExportedStringValue.StartsWith(CategoryLimit))
				{
					ExportedStringValue.MidInline(CategoryLimit.Len(), MAX_int32, false);
				}
#endif
			}
#define HTN_RANGE_STRUCT_TO_STRING(RangeStructType) \
			else if (StructProp->Struct->IsChildOf(TBaseStructure<RangeStructType>::Get())) \
			{ \
				const RangeStructType* const PropertyValue = reinterpret_cast<const RangeStructType*>(PropertyAddr); \
				ExportedStringValue = FHTNHelpers::DescribeRange(*PropertyValue); \
			}
			// The types of TRange are the ones as declared in Range.h
			// which also have a specialization of TBaseStructure<>::Get().
			HTN_RANGE_STRUCT_TO_STRING(FFloatRange)
			HTN_RANGE_STRUCT_TO_STRING(FInt32Range)
#undef HTN_RANGE_STRUCT_TO_STRING
			// Generic case for structs. Process the struct's properties recursively.
			else
			{
				TStringBuilder<2048> SB;
				for (const FProperty* const SubProperty : TFieldRange<FProperty>(StructProp->Struct))
				{
					const uint8* const SubPropertyAddr = SubProperty->ContainerPtrToValuePtr<uint8>(PropertyAddr);
					const FString SubPropertyDescription = DescribeProperty(SubProperty, SubPropertyAddr);
					UE::String::ParseLines(SubPropertyDescription, [&](FStringView Line)
					{
						SB << TEXT("\n    ") << Line;
					});
				}

				ExportedStringValue = SB.ToString();
			}
		}
		// Special case for floats to remove unnecessary zeros
		else if (NumericProp && NumericProp->IsFloatingPoint())
		{
			const double DoubleValue = NumericProp->GetFloatingPointPropertyValue(PropertyAddr);
			ExportedStringValue = FString::SanitizeFloat(DoubleValue);
		}
		else
		{
#if UE_VERSION_OLDER_THAN(5, 1, 0)
			Prop->ExportTextItem(ExportedStringValue, PropertyAddr, nullptr, nullptr, PPF_PropertyWindow, nullptr);
#else
			Prop->ExportTextItem_Direct(ExportedStringValue, PropertyAddr, nullptr, nullptr, PPF_PropertyWindow, nullptr);
#endif
		}

		if (bWithName)
		{
			const bool bIsBool = Prop->IsA(FBoolProperty::StaticClass());
			return FString::Printf(TEXT("%s: %s"),
				*FName::NameToDisplayString(Prop->GetName(), bIsBool),
				*ExportedStringValue);
		}
		else
		{
			return ExportedStringValue;
		}
	}
}

UHTNComponent* FHTNHelpers::GetHTNComponent(AActor* Target)
{
	if (AAIController* const Controller = UAIBlueprintHelperLibrary::GetAIController(Target))
	{
		if (UHTNComponent* const HTNComponent = Controller->FindComponentByClass<UHTNComponent>())
		{
			return HTNComponent;
		}
	}

	if (UHTNComponent* const HTNComponent = Target->FindComponentByClass<UHTNComponent>())
	{
		return HTNComponent;
	}

	return nullptr;
}

UWorldStateProxy* FHTNHelpers::GetWorldStateProxy(AActor* Target, bool bIsPlanning)
{
	if (UHTNComponent* const HTNComponent = GetHTNComponent(Target))
	{
		return bIsPlanning ? HTNComponent->GetPlanningWorldStateProxy() : HTNComponent->GetBlackboardProxy();
	}

	return nullptr;
}

void FHTNHelpers::ForGameplayTagAndChildTags(const FGameplayTag& GameplayTag, TFunctionRef<void(const FGameplayTag&)> Callable)
{
	struct Local
	{
		static void ForNodeAndChildren(const TSharedPtr<FGameplayTagNode>& Node, TFunctionRef<void(const FGameplayTag&)> Visitor)
		{
			if (Node.IsValid())
			{
				const FGameplayTag& TagAtNode = Node->GetCompleteTag();
				Visitor(TagAtNode);
				for (const TSharedPtr<FGameplayTagNode>& Child : Node->GetChildTagNodes())
				{
					ForNodeAndChildren(Child, Visitor);
				}
			}
		}
	};

	const TSharedPtr<FGameplayTagNode> TagNode = UGameplayTagsManager::Get().FindTagNode(GameplayTag);
	Local::ForNodeAndChildren(TagNode, Callable);
}

void FHTNHelpers::ResolveBlackboardKeySelectors(UObject& Object, const UClass& StopAtClass, const UBlackboardData* BlackboardAsset)
{
	struct Local
	{
		static void ProcessRecursive(FStructProperty* StructProperty, void* DataPtr, const UBlackboardData* BlackboardAsset)
		{
			void* const PropertyData = StructProperty->ContainerPtrToValuePtr<void>(DataPtr);

			if (StructProperty->Struct->IsChildOf(FBlackboardKeySelector::StaticStruct()))
			{
				FBlackboardKeySelector* const BlackboardKeySelector = StaticCast<FBlackboardKeySelector*>(PropertyData);

				if (BlackboardAsset)
				{
					BlackboardKeySelector->ResolveSelectedKey(*BlackboardAsset);
				}
				else
				{
					UE_LOG(LogHTN, Warning, TEXT("Can't initialize HTNLocationSource due to missing blackboard data."));
					BlackboardKeySelector->InvalidateResolvedKey();
				}
			}

			for (FStructProperty* const SubstructProperty : TFieldRange<FStructProperty>(StructProperty->Struct))
			{
				ProcessRecursive(SubstructProperty, PropertyData, BlackboardAsset);
			}
		}
	};

	for (FProperty* TestProperty = Object.GetClass()->PropertyLink; TestProperty; TestProperty = TestProperty->PropertyLinkNext)
	{
		// Stop when reaching base class
		if (TestProperty->GetOwner<UObject>() == &StopAtClass)
		{
			break;
		}

		if (FStructProperty* const StructProperty = CastField<FStructProperty>(TestProperty))
		{
			Local::ProcessRecursive(StructProperty, &Object, BlackboardAsset);
		}
	}
}

FString FHTNHelpers::CollectPropertyDescription(const UObject* Ob, const UClass* StopAtClass, TArrayView<FProperty* const> InPropertiesToSkip)
{
	return CollectPropertyDescription(Ob, StopAtClass, [InPropertiesToSkip](FProperty* InTestProperty) { return InPropertiesToSkip.Contains(InTestProperty); });
}

FString FHTNHelpers::CollectPropertyDescription(const UObject* Ob, const UClass* StopAtClass, TFunctionRef<bool(FProperty* /*TestProperty*/)> ShouldSkipProperty)
{
	static const TArray<FFieldClass*> DisallowedPropertyClasses
	{
		FDelegateProperty::StaticClass(),
		FMulticastDelegateProperty::StaticClass()
	};

	TStringBuilder<2048> SB;
	for (FProperty* TestProperty = Ob->GetClass()->PropertyLink; TestProperty; TestProperty = TestProperty->PropertyLinkNext)
	{
		// Stop when reaching base class
		if (TestProperty->GetOwner<UObject>() == StopAtClass)
		{
			break;
		}

		if (TestProperty->HasAnyPropertyFlags(CPF_Transient) ||
			TestProperty->HasAnyPropertyFlags(CPF_DisableEditOnInstance) ||
			ShouldSkipProperty(TestProperty))
		{
			continue;
		}

		if (Algo::NoneOf(DisallowedPropertyClasses, [TestProperty](FFieldClass* FieldClass) { return TestProperty->IsA(FieldClass); }))
		{
			if (SB.Len())
			{
				SB << TEXT('\n');
			}

			SB << DescribeProperty(TestProperty, TestProperty->ContainerPtrToValuePtr<uint8>(Ob));
		}
	}

	return SB.ToString();
}
