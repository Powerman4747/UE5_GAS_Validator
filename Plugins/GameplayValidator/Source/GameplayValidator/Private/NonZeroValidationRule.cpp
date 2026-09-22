// Fill out your copyright notice in the Description page of Project Settings.
#include "NonZeroValidationRule.h"

#include "AttributeSet.h"
#include "GASValidator.h"



NonZeroValidationRule::NonZeroValidationRule() :
	IGASValidationRule("AttributeNonZero")
{
}

void NonZeroValidationRule::Validate(const GASObjects& Objects, TArray<GASValidationResult>& Results)
{
	for (auto Attribute : Objects.Attributes)
	{
		// if this attribute will not have a DataTable then the others of this Attribute set also will not 
		if (!Attribute.SourceOfValue.IsSet())
		{
			GASValidationResult Result;
			Result.RuleName = GetRuleName();
			Result.Severity = EGASValidationSeverity::ERROR;
			Result.Message = FString::Printf(
				TEXT("Missing DefaultStartingData field of type %s on the AbilitySystemComponent"), *Attribute.Attribute.GetAttributeSetClass()->GetName());
			Results.Add(Result);
			break;
		}
		const auto SourceOfValue = Attribute.SourceOfValue.GetValue().ToString();
		if (!Attribute.Value.IsSet())
		{
			// Row in UDataTable could not be found...
			GASValidationResult Result;
			Result.RuleName = GetRuleName();
			Result.Severity = EGASValidationSeverity::ERROR;
			Result.Message = FString::Printf(
				TEXT("%s: Attribute %s.%s isn't found and default the value to the default C++ float value (0.0)"),
				*SourceOfValue, *Attribute.Attribute.GetAttributeSetClass()->GetName(), *Attribute.Attribute.AttributeName);
			Results.Add(Result);
			continue;
		}
		
		if (Attribute.Metadata.Contains("AllowsZero"))
		{
		    continue;
		}
		
		float Value = Attribute.Value.GetValue();
		if (FMath::IsNearlyZero(Value))
		{
			GASValidationResult Result;
			Result.RuleName = GetRuleName();
			Result.Severity = EGASValidationSeverity::ERROR;
			Result.Message = FString::Printf(
				TEXT("%s: Attribute %s.%s is 0. This value is not initialized"),
				*SourceOfValue, *Attribute.Attribute.GetAttributeSetClass()->GetName(), *Attribute.Attribute.AttributeName);
			Results.Add(Result);
		}
	}	
}