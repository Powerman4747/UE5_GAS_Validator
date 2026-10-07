// Fill out your copyright notice in the Description page of Project Settings.

#include "Rules/CalculationAttributesValidationRule.h"
#include "DataStructures/DiscoveryData.h"

FCalculationAttributesValidationRule::FCalculationAttributesValidationRule() :
	IGASValidationRule("CalculationAttributes")
{
}

void FCalculationAttributesValidationRule::Validate(const FGASObjects& Objects, TArray<FGASValidationResult>& Results)
{
	for (const auto& Pair : Objects.Calculations)
	{
		const auto& Calculation = Pair.Value;

		for (const auto& Attribute : Calculation.CapturedAttributes)
		{
			if (!Attribute.IsValid())
			{
				FGASValidationResult Result = CreateResult(EGASValidationSeverity::ERROR, 
					FString::Printf(TEXT("'%s': Invalid Attribute in 'Relevant Attributes To Capture'"), *Pair.Key->GetFName().ToString()));
				Results.Add(Result);
			}
		}
	}
}

