// Fill out your copyright notice in the Description page of Project Settings.
#include "NonZeroValidationRule.h"

#include "AbilitySystemInterface.h"
#include "AttributeSet.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "AbilitySystemComponent.h"


NonZeroValidationRule::NonZeroValidationRule() :
	IGASValidationRule("AttributeNonZero")
{
	
}

void NonZeroValidationRule::Validate(UObject* Asset, TArray<GASValidationResult>& Results)
{
	AActor* Actor = Cast<AActor>(Asset);
	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Actor);
	if (!Actor || !ASI)
	{
		return;
	}

	UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent();
	if (!ASC)
	{
		// Ability System not initialized yet
		return;
	}

	bool HasDefaultStartingData = false;
	int CountResults = Results.Num();

	for (auto Defaults : ASC->DefaultStartingData)
	{
		HasDefaultStartingData = true;
		if (!Defaults.DefaultStartingTable)
		{
			GASValidationResult Result;
			Result.RuleName = GetRuleName();
			Result.Severity = EGASValidationSeverity::ERROR;
			Result.Message = FString::Printf(
				TEXT("Empty DefaultStartingData field of type %s on the AbilitySystemComponent which can be set in %s"),
				*Defaults.Attributes->GetName(), *Asset->GetName());
			Results.Add(Result);
			
			continue;
		}
		
		ValidateAttributeSet(Defaults, Results);
	}
	
	if (HasDefaultStartingData && CountResults == Results.Num())
	{
		GASValidationResult Result;
		Result.RuleName = GetRuleName();
		Result.Severity = EGASValidationSeverity::INFO;
		Result.Message = FString::Printf(
			TEXT("Asset: %s is valid"),
			*Asset->GetName());
		Results.Add(Result);
	}
}

void NonZeroValidationRule::ValidateAttributeSet(const FAttributeDefaults& Default, TArray<GASValidationResult>& Results)
{	
    UDataTable* MetaTable = Default.DefaultStartingTable;
	auto AttributeSetClass = Default.Attributes.Get();
	
    for (TFieldIterator<FStructProperty> PropIt(AttributeSetClass); PropIt; ++PropIt)
    {
	    FStructProperty* StructProp = *PropIt;
    	if (StructProp->Struct != FGameplayAttributeData::StaticStruct())
    	{
    		continue;
    	}
    	
    	bool AllowsZero = StructProp->GetBoolMetaData("AllowsZero");
    	
    	if (AllowsZero)
    	{
    		// if zero allowed skip this attribute 
    		continue;
    	}

    	const FName AttributeName = StructProp->GetFName();
    	
    	const FString RowNameString = FString::Printf(
	TEXT("%s.%s"), *AttributeSetClass->GetName(), *AttributeName.ToString());
    	const FName RowName(*RowNameString);
    	
    	const FAttributeMetaData* Row = MetaTable->FindRow<FAttributeMetaData>(RowName, TEXT("GASValidator"));
    	
    	// check if row exists
    	if (!Row)
    	{
    		// allows zero give warning
    		if (AllowsZero)
    		{
    			GASValidationResult Result;
    			Result.RuleName = GetRuleName();
    			Result.Severity = EGASValidationSeverity::WARNING;
    			Result.Message = FString::Printf(
					TEXT("%s: Attribute %s.%s isn't found and default the value to the default C++ float value (0.0). However is allowed to be zero"),
					*MetaTable->GetName(), *AttributeSetClass->GetName(), *StructProp->GetName());
    			Results.Add(Result);
    		}
		    else
		    {
		    	GASValidationResult Result;
		    	Result.RuleName = GetRuleName();
		    	Result.Severity = EGASValidationSeverity::ERROR;
		    	Result.Message = FString::Printf(
					TEXT("%s: Row %s.%s isn't found and default the value to the default C++ float value (0.0)"),
					*MetaTable->GetName(), *AttributeSetClass->GetName(), *StructProp->GetName());
		    	Results.Add(Result);
		    }    		
    		
    		// check if next attribute still exists
    		continue;
    	}
    	
    	// Check if values are not zero
    	float BaseValue = Row->BaseValue;
    	if (FMath::IsNearlyZero(BaseValue))
    	{
    		GASValidationResult Result;
    		Result.RuleName = GetRuleName();
    		Result.Severity = EGASValidationSeverity::ERROR;
    		Result.Message = FString::Printf(
				TEXT("%s: Attribute %s.%s is 0. This value is not initialized"),
				*MetaTable->GetName(), *AttributeSetClass->GetName(), *StructProp->GetName());
    		Results.Add(Result);
    	}
    }
}

GASObjects NonZeroValidationRule::FindGASRelatedFields(UObject* Class)
{
	GASObjects Objects;
	
	if (UBlueprint* Blueprint = Cast<UBlueprint>(Class))
	{
		if (Blueprint->GeneratedClass)
		{
			Class = Blueprint->GeneratedClass->GetDefaultObject();
		}
		else
		{
			return Objects; // no generated class yet (e.g. Blueprint has compile errors)
		}
	}

	if (!Class)
	{
		return Objects;
	}
	
	for (TFieldIterator<FProperty> PropertyIterator(Class->GetClass()); PropertyIterator; ++PropertyIterator)
	{
		FProperty* Property = *PropertyIterator;
		
		if(FClassProperty* ClassProperty = CastField<FClassProperty>(Property))
		{
			if (ClassProperty->MetaClass->IsChildOf(UAttributeSet::StaticClass()))
			{
				UObject* ClassValue = ClassProperty->GetObjectPropertyValue_InContainer(Class);
				if (UClass* Class = Cast<UClass>(ClassValue))
				{
					if (UAttributeSet* CDO = Cast<UAttributeSet>(Class->GetDefaultObject()))
					{
						Objects.AttributeSets.Add(CDO);
					}
				}
			}
		}
		else if (FObjectProperty* ObjectProperty = CastField<FObjectProperty>(Property))
		{
			if (ObjectProperty->PropertyClass->IsChildOf(UAttributeSet::StaticClass()))
			{
				UObject* Value = ObjectProperty->GetObjectPropertyValue_InContainer(Class);
				if (UAttributeSet* AttributeSet = Cast<UAttributeSet>(Value))
				{
					Objects.AttributeSets.Add(AttributeSet);
				}
			}
		}
	}
	
	return Objects;
}
