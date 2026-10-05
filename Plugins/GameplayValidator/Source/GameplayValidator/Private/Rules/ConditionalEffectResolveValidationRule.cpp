// Fill out your copyright notice in the Description page of Project Settings.
#include "Rules/ConditionalEffectResolveValidationRule.h"

#include "GASValidator.h"
#include "GameplayEffect.h"


ConditionalEffectResolveValidationRule::ConditionalEffectResolveValidationRule() :
	IGASValidationRule("ConditionalEffectResolve")
{
}

void ConditionalEffectResolveValidationRule::Validate(const GASObjects& Objects, TArray<GASValidationResult>& Results)
{
	for (const auto& Pair : Objects.Effects)
	{
		const auto& Effect = Pair.Value;

		for (const auto& ConditionalEffect : Effect.Effects)
		{
			if (ConditionalEffect.EffectClass == nullptr)
			{
				GASValidationResult Result;
				Result.RuleName = GetRuleName();
				Result.Severity = EGASValidationSeverity::ERROR;
				Result.Message = FString::Printf(
					TEXT("'%s':Unset Effect at property '%s'"), *Pair.Key->GetFName().ToString(), *ConditionalEffect.PropertyName.ToString());
				Results.Add(Result);
			}
			else if (ConditionalEffect.EffectClass == Pair.Key)
			{
				GASValidationResult Result;
				Result.RuleName = GetRuleName();
				Result.Severity = EGASValidationSeverity::ERROR;
				Result.Message = FString::Printf(
					TEXT("'%s': Circular dependency created at property '%s'"), *Pair.Key->GetFName().ToString(), *ConditionalEffect.PropertyName.ToString());
				Results.Add(Result);
			}			
		}
	}
}
