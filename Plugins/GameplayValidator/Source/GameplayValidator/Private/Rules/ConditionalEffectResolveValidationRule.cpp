// Fill out your copyright notice in the Description page of Project Settings.
#include "Rules/ConditionalEffectResolveValidationRule.h"

#include "GameplayEffect.h"
#include "DataStructures/DiscoveryData.h"


FConditionalEffectResolveValidationRule::FConditionalEffectResolveValidationRule() :
	IGASValidationRule("ConditionalEffectResolve")
{
}

void FConditionalEffectResolveValidationRule::Validate(const FGASObjects& Objects, TArray<FGASValidationResult>& Results)
{
	for (const auto& Pair : Objects.Effects)
	{
		const auto& Effect = Pair.Value;

		for (const auto& ConditionalEffect : Effect.Effects)
		{
			if (ConditionalEffect.EffectClass == nullptr)
			{				
				FGASValidationResult Result = CreateResult(EGASValidationSeverity::ERROR, 
												FString::Printf(TEXT("'%s': Unset Effect at property '%s'"), *Pair.Key->GetFName().ToString(), *ConditionalEffect.PropertyName.ToString()));
				Results.Add(Result);
			}
			else if (ConditionalEffect.EffectClass == Pair.Key)
			{
				FGASValidationResult Result = CreateResult(EGASValidationSeverity::UNRESOLVED, 
												FString::Printf(TEXT("'%s': Circular dependency created at property '%s'"), *Pair.Key->GetFName().ToString(), *ConditionalEffect.PropertyName.ToString()));
				
				Results.Add(Result);
			}			
		}
	}
}
