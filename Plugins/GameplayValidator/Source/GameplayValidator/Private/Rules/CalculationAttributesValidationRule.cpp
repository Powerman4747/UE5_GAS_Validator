// Fill out your copyright notice in the Description page of Project Settings.

#include "Rules/CalculationAttributesValidationRule.h"
#include "DataStructures/DiscoveryData.h"

CalculationAttributesValidationRule::CalculationAttributesValidationRule() :
	IGASValidationRule("CalculationAttributes")
{
}

void CalculationAttributesValidationRule::Validate(const GASObjects& Objects, TArray<GASValidationResult>& Results)
{
	for (const auto& Pair : Objects.Calculations)
	{
		const auto& Calculation = Pair.Value;

		for (const auto& Attribute : Calculation.CapturedAttributes)
		{
			if (!Attribute.IsValid())
			{
				GASValidationResult Result;
				Result.RuleName = GetRuleName();
				Result.Severity = EGASValidationSeverity::ERROR; 
				Result.Message = FString::Printf(
					TEXT("'%s': Invalid Attribute in 'Relevant Attributes To Capture'"), *Pair.Key->GetFName().ToString());
				Results.Add(Result);
			}
		}
	}
}

