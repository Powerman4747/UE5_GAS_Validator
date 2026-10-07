// Fill out your copyright notice in the Description page of Project Settings.
#include "Rules/ExecutionValidationRule.h"

#include "GameplayEffectCalculation.h"
#include "DataStructures/DiscoveryData.h"


ExecutionValidationRule::ExecutionValidationRule() :
	IGASValidationRule("ExecutionValidation")
{
}

void ExecutionValidationRule::Validate(const FGASObjects& Objects, TArray<FGASValidationResult>& Results)
{
	for (const auto& Pair : Objects.Effects)
	{
		const auto& Effect = Pair.Value;
		for (const auto& Execution : Effect.Executions)
		{
			if (!Execution.CalculationClass)
			{
				// Still can be wanted so the additional effects are always triggered
				FGASValidationResult Result;
				Result.RuleName = GetRuleName();
				Result.Severity = EGASValidationSeverity::WARNING; 
				Result.Message = FString::Printf(
					TEXT("'%s': Execution 'Calculation Class' is empty"), *Pair.Key->GetFName().ToString());
				Results.Add(Result);
			}
		}
	}
}