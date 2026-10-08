// Fill out your copyright notice in the Description page of Project Settings.

#include "GASValidator.h"

#include "AbilitySystemInterface.h"
#include "GASValidatorLog.h"
#include "EditorValidatorSubsystem.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"

#include "Engine/Blueprint.h"
#include "Editor.h"
#include "GameplayCueSet.h"
#include "GameplayEffectComponent.h"
#include "GameplayModMagnitudeCalculation.h"
#include "DataStructures/DiscoveryData.h"
#include "Rules/AbilityReachEffectValidationRule.h"
#include "Rules/CalculationAttributesValidationRule.h"
#include "Rules/ConditionalEffectResolveValidationRule.h"
#include "Rules/ExecutionValidationRule.h"
#include "Rules/GASValidationRule.h"
#include "Rules/ModifierValidationRule.h"
#include "Rules/NonZeroValidationRule.h"
#include "Rules/TagRegistryValidationRule.h"
#include "DataStructures/ValidationResult.h"
#include "Discovery/GASDiscovery.h"
#include "Discovery/GASDiscoveryInternal.h"


TArray<TSharedRef<IGASValidationRule>> UGASValidator::Rules;

#ifndef USE_NO_STANDARD_RULES
GAS_VALIDATION_REGISTER_RULE(FNonZeroValidationRule); 
GAS_VALIDATION_REGISTER_RULE(FTagRegistryValidationRule); 
GAS_VALIDATION_REGISTER_RULE(FConditionalEffectResolveValidationRule); 
GAS_VALIDATION_REGISTER_RULE(FModifierValidationRule); 
GAS_VALIDATION_REGISTER_RULE(FExecutionValidationRule); 
GAS_VALIDATION_REGISTER_RULE(FCalculationAttributesValidationRule);
GAS_VALIDATION_REGISTER_RULE(FAbilityReachEffectValidationRule);
#endif

void UGASValidator::RunValidator()
{
	UE_LOG(LogGASValidator, Log, TEXT("GAS Validator started"));

	UEditorValidatorSubsystem* ValidatorSubsystem = GEditor->GetEditorSubsystem<UEditorValidatorSubsystem>();
	
	FValidateAssetsSettings Settings;
	Settings.bShowIfNoFailures = true;

	FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	TArray<FAssetData> AssetDataList;
	AssetRegistryModule.Get().GetAssetsByPath(FName("/Game"), AssetDataList, true);
	
	if (AssetDataList.Num() <= 0)
	{
		UE_LOG(LogGASValidator, Log, TEXT("No assets to validate"));
	}
	
	// NEW: separately handle native classes, which the above will never reach
	TArray<UClass*> NativeClasses;
	GetDerivedClasses(UGameplayAbility::StaticClass(), NativeClasses, true);
	
	FGASObjects Objects;
	TSet<UClass*> VisitedClasses;
	for (UClass* Class : NativeClasses)
	{
		if (Class->ClassGeneratedBy != nullptr) continue; // skip Blueprint-generated
		
		FindGASObjects(Class->GetDefaultObject(), Objects, VisitedClasses);
	}
	
	NativeClasses.Empty();
	GetDerivedClasses(UGameplayEffect::StaticClass(), NativeClasses, true);
	for (UClass* Class : NativeClasses)
	{
		if (Class->ClassGeneratedBy != nullptr) continue; // skip Blueprint-generated
		
		FindGASObjects(Class->GetDefaultObject(), Objects, VisitedClasses);
	}
	
	TArray<FGASValidationResult> ValidationResults;
	for (auto Rule : Rules)
	{
		Rule->Validate(Objects, ValidationResults);
	}
	
	LogResults(ValidationResults);
	
	FValidateAssetsResults Results;
	ValidatorSubsystem->ValidateAssetsWithSettings(AssetDataList, Settings, Results);
	
	UE_LOG(LogGASValidator, Log, TEXT("GAS Validator ended"));
}

bool UGASValidator::CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset,
	FDataValidationContext& InContext) const
{
	UClass* ClassToCheck = InAsset->GetClass();

    if (const UBlueprint* Blueprint = Cast<UBlueprint>(InAsset))
    {
        ClassToCheck = Blueprint->GeneratedClass;
    }

    if (!ClassToCheck)
    {
        return false;
    }

    if (ClassToCheck->IsChildOf(UGameplayAbility::StaticClass())
        || ClassToCheck->IsChildOf(UGameplayEffect::StaticClass())
        || ClassToCheck->IsChildOf(UAttributeSet::StaticClass())
        || ClassToCheck->ImplementsInterface(UAbilitySystemInterface::StaticClass()))
    {
        return true;
    }
	
	return UGASValidator::HasGASProperties(ClassToCheck);
}

EDataValidationResult UGASValidator::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset,
                                                         FDataValidationContext& Context)
{
	if (UBlueprint* Blueprint = Cast<UBlueprint>(InAsset))
	{
		if (Blueprint->GeneratedClass)
		{
			InAsset = Blueprint->GeneratedClass->GetDefaultObject();
		}
	}
	FGASObjects Objects;
	TSet<UClass*> VisitedSet;
	FindGASObjects(InAsset, Objects, VisitedSet);
	
	TArray<FGASValidationResult> Results;
	for (auto Rule : Rules)
	{
		Rule->Validate(Objects, Results);
	}
	
	bool bHasError = UGASValidator::LogResults(Results, InAsset->GetName());
	
	if (bHasError)
	{
		AssetFails(InAsset, FText::FromString(TEXT("GAS validation failed - see errors above")));
		return EDataValidationResult::Invalid;
	}
		
	AssetPasses(InAsset);
	return 	EDataValidationResult::Valid;
}

void UGASValidator::FindGASObjects(UObject* Object, FGASObjects& GASObjects, TSet<UClass*>& VisitedClasses)
{
	auto Class = Object->GetClass();
	if (VisitedClasses.Contains(Class))
	{
		return;
	}
	
	VisitedClasses.Add(Class);
	
	if (Object->IsA(UGameplayAbility::StaticClass()))
	{
		GASDiscovery::DiscoverAbility(Class, GASObjects);
	}
	else if (Object->IsA(UGameplayEffect::StaticClass()))
	{
		GASDiscovery::DiscoverEffect(Class, GASObjects);
	}
	else if (Object->IsA(UAttributeSet::StaticClass()))
	{
		GASDiscovery::DiscoverAttributes(Class, GASObjects);
	}
	else if (Object->IsA(UGameplayEffectComponent::StaticClass()))
    {
        FindGASObjects(Object, GASObjects, VisitedClasses);
    }
	
	for (TFieldIterator<FProperty> PropIt(Class); PropIt; ++PropIt)
	{
		FProperty* Property = *PropIt;
		UClass* PropertyType = GASDiscovery::Private::ResolvePropertyType(Property);
		if (!PropertyType)
		{
			if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
			{
				if (StructProp->Struct == FGameplayTagContainer::StaticStruct())
				{
					GASDiscovery::DiscoverTags(Class, Object, GASObjects);
				}
				else
				{
					FindGASObjectsInStruct(Class, StructProp->ContainerPtrToValuePtr<void>(Object),StructProp->Struct,GASObjects, VisitedClasses);
				}
			}
			else if (FArrayProperty* ArrayProp = CastField<FArrayProperty>(Property))
			{	
				GASDiscovery::Private::IterateArray(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(Object), 
											[&Class, &GASObjects, &VisitedClasses](const FProperty* Property, const void* ElementPtr)
													{
														if (const FStructProperty* Struct = CastField<FStructProperty>(Property))
														{
															FindGASObjectsInStruct(Class, ElementPtr, Struct->Struct, GASObjects, VisitedClasses);
														}
														else if (GASDiscovery::Private::ResolvePropertyType(Property))
														{
															UClass* Value = GASDiscovery::Private::ResolveClassValueFromElement(Property, ElementPtr);
															if (Value && !VisitedClasses.Contains(Value))
															{
																FindGASObjects(Value->GetDefaultObject(), GASObjects, VisitedClasses);
															}
														}
													});
			}
			continue;
		}
		
		UClass* Value = GASDiscovery::Private::ResolveClassValue(Property, Object);
		
		if (!Value || !IsValid(Value) || VisitedClasses.Contains(Value))
		{
			continue;
		}
		
		// any class
		if (auto* CDO = Value->GetDefaultObject())
		{
			FindGASObjects(CDO, GASObjects, VisitedClasses);
		}
	}
	
	if (Class->ImplementsInterface(UAbilitySystemInterface::StaticClass()))
	{
		const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Object);
		if (!ASI)
		{
			return;
		}

		UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent(); // the guarded virtual call

		if (!ASC)
		{
			return;
		}

		for (const FAttributeDefaults& Entry : ASC->DefaultStartingData)
		{
			
			if (!Entry.Attributes)
			{
				continue;
			}

			if (!VisitedClasses.Contains(Entry.Attributes))
			{
				GASDiscovery::DiscoverAttributes(Entry.Attributes, GASObjects);
			}

			if (!Entry.DefaultStartingTable)
		    {
		        continue;
		    }
			
			TArray<FDiscoveredAttribute>& Attributes = GASObjects.Attributes[Entry.Attributes];

			for (auto& Attribute : Attributes)
			{
				Attribute.SourceOfValue = Entry.DefaultStartingTable->GetFName();
				
				const FString AttributeName = Attribute.Attribute.AttributeName;
				const FString RowNameString = FString::Printf(TEXT("%s.%s"), *Entry.Attributes->GetName(), *AttributeName);
				const FName RowName(*RowNameString);
				const FAttributeMetaData* Row = Entry.DefaultStartingTable->FindRow<FAttributeMetaData>(RowName, TEXT("GASValidator"));
                                   						
				if (Row)
				{
					float BaseValue = Row->BaseValue;
					Attribute.Value = BaseValue;
				}
			}
		}
	}
}

void UGASValidator::FindGASObjectsInStruct(UClass* Class, const void* StructInstance, UScriptStruct* StructType,
	FGASObjects& GASObjects, TSet<UClass*>& VisitedClasses)
{
    if (!StructInstance || !StructType)
    {
        return;
    }

    for (TFieldIterator<FProperty> PropIt(StructType); PropIt; ++PropIt)
    {
        FProperty* Property = *PropIt;
        UClass* PropertyType = GASDiscovery::Private::ResolvePropertyType(Property);

        if (!PropertyType)
        {
            if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
            {
                if (StructProp->Struct == FGameplayTagContainer::StaticStruct())
                {
                    GASDiscovery::DiscoverTags(Class, StructInstance, GASObjects);
                }
                else
                {
                    FindGASObjectsInStruct(
                    	Class,
                        StructProp->ContainerPtrToValuePtr<void>(StructInstance),
                        StructProp->Struct,
                        GASObjects,
                        VisitedClasses);
                }
            }
            else if (FArrayProperty* ArrayProp = CastField<FArrayProperty>(Property))
            {
            	GASDiscovery::Private::IterateArray(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(StructInstance), 
											[&Class, &GASObjects, &VisitedClasses](const FProperty* Property, const void* ElementPtr)
													{
														if (const FStructProperty* Struct = CastField<FStructProperty>(Property))
														{
															FindGASObjectsInStruct(Class, ElementPtr, Struct->Struct, GASObjects, VisitedClasses);
														}
														else if (GASDiscovery::Private::ResolvePropertyType(Property))
														{
															UClass* Value = GASDiscovery::Private::ResolveClassValueFromElement(Property, ElementPtr);
															if (Value && !VisitedClasses.Contains(Value))
															{
																FindGASObjects(Value->GetDefaultObject(), GASObjects, VisitedClasses);
															}
														}
													});
            }
            continue;
        }

    	const void* ValuePtr = Property->ContainerPtrToValuePtr<void>(StructInstance);
        UClass* Value = GASDiscovery::Private::ResolveClassValueFromElement(Property, ValuePtr);
        if (!Value || !IsValid(Value) || VisitedClasses.Contains(Value))
        {
            continue;
        }

    	if (auto* CDO = Value->GetDefaultObject())
    	{
    		FindGASObjects(CDO, GASObjects, VisitedClasses);
    	}
    }
}

bool UGASValidator::HasGASProperties(UClass* Class)
{
	for (TFieldIterator<FProperty> PropIt(Class); PropIt; ++PropIt)
	{
		FProperty* Property = *PropIt;
		UClass* PropertyType = GASDiscovery::Private::ResolvePropertyType(Property);
		if (!PropertyType)
		{
			if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
			{
				if (StructProp->Struct == FGameplayTagContainer::StaticStruct())
				{
					return true;
				}
			}
			else if (FArrayProperty* ArrayProp = CastField<FArrayProperty>(Property))
			{
				// Has GAS Property in array
				UClass* InnerType = GASDiscovery::Private::ResolvePropertyType(ArrayProp->Inner);
				if (InnerType && (InnerType->IsChildOf(UAttributeSet::StaticClass())
					|| InnerType->IsChildOf(UGameplayAbility::StaticClass())
					|| InnerType->IsChildOf(UGameplayEffect::StaticClass())
					|| InnerType->IsChildOf(UAbilitySystemComponent::StaticClass())))
				{
					return true;
				}
			}
			continue;
		}
		
		if (PropertyType->IsChildOf(UAttributeSet::StaticClass()) 
			|| PropertyType->IsChildOf(UGameplayAbility::StaticClass()) 
			|| PropertyType->IsChildOf(UGameplayEffect::StaticClass()))
		{
			return true;
		}
	}
	
	return false;
}

bool UGASValidator::LogResults(TArray<FGASValidationResult>& Results, FString AssetName)
{
	bool bHasError = false;
	
	for (auto Result : Results)
	{
		UE_LOG(LogGASValidator, Log, TEXT("%s: %s"), *AssetName, *Result.Message)
		
		if (Result.Severity == EGASValidationSeverity::ERROR)
		{
			bHasError = true;
		}
	}
	
	return bHasError;
}