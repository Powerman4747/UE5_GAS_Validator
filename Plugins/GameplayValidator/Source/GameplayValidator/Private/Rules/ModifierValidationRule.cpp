// Fill out your copyright notice in the Description page of Project Settings.
#include "Rules/ModifierValidationRule.h"

#include "GameplayEffect.h"
#include "GameplayModMagnitudeCalculation.h"
#include "GASValidator.h"

ModifierValidationRule::ModifierValidationRule() :
	IGASValidationRule("ModifierValidation")
{
}

void ModifierValidationRule::Validate(const GASObjects& Objects, TArray<GASValidationResult>& Results)
{
	for (const auto& Pair : Objects.Effects)
	{
		const auto& Effect = Pair.Value;
		for (const auto& Modifier : Effect.Modifiers)
		{
			if (!Modifier.Attribute.IsValid())
			{
				GASValidationResult Result;
				Result.RuleName = GetRuleName();
				Result.Severity = EGASValidationSeverity::ERROR;
				Result.Message = FString::Printf(
					TEXT("'%s': Modifier attribute '%s' is invalid. Please change it to a valid attribute you have created 'AttributeSet.Attribute'"), *Pair.Key->GetFName().ToString(), *Modifier.Attribute.AttributeName);
				Results.Add(Result);
			}
			
			switch (Modifier.TypeOfCalculation)
			{
				case EGameplayEffectMagnitudeCalculation::ScalableFloat:
				{
					if (FMath::IsNearlyZero(Modifier.FloatValue))
					{
						GASValidationResult Result;
						Result.RuleName = GetRuleName();
						Result.Severity = EGASValidationSeverity::ERROR;
						Result.Message = FString::Printf(
							TEXT("'%s': Modifer 'Scalable float', it doesn't do anything, because value is '%f'"), *Pair.Key->GetFName().ToString(), Modifier.FloatValue);
						Results.Add(Result);
					}
					break;
				}
				case EGameplayEffectMagnitudeCalculation::AttributeBased:
				{
					if (!Modifier.BasedOnAttribute.IsValid())
					{
						GASValidationResult Result;
						Result.RuleName = GetRuleName();
						Result.Severity = EGASValidationSeverity::ERROR;
						Result.Message = FString::Printf(
							TEXT("'%s': Modifier 'Attribute Based', attribute '%s' is invalid. Please change it to a valid attribute you have created 'AttributeSet.Attribute'"), *Pair.Key->GetFName().ToString(), *Modifier.BasedOnAttribute.AttributeName);
						Results.Add(Result);
					}
					break;
				}
				case EGameplayEffectMagnitudeCalculation::CustomCalculationClass:
				{
					if (Modifier.CalculationClassReference == nullptr)
					{
						GASValidationResult Result;
						Result.RuleName = GetRuleName();
						Result.Severity = EGASValidationSeverity::ERROR;
						Result.Message = FString::Printf(
							TEXT("'%s': Modifier 'Custom Calculation Class', The class is not given. Please give a 'UGameplayModMagnitudeCalculation' class"), *Pair.Key->GetFName().ToString());
						Results.Add(Result);
					}
					break;
				}
				case EGameplayEffectMagnitudeCalculation::SetByCaller:
				{
					if (!Modifier.CallableTag.IsValid())
					{
						GASValidationResult Result;
						Result.RuleName = GetRuleName();
						Result.Severity = EGASValidationSeverity::ERROR;
						Result.Message = FString::Printf(
							TEXT("'%s': Modifier 'Set By Caller', Please give a a valid tag 'AttributeSet.Attribute'"), *Pair.Key->GetFName().ToString());
						Results.Add(Result);
					}
					if(Modifier.CallableName.IsNone())
					{
						GASValidationResult Result;
						Result.RuleName = GetRuleName();
						Result.Severity = EGASValidationSeverity::WARNING;
						Result.Message = FString::Printf(
							TEXT("'%s': Modifier 'Set By Caller', Can't resolve the name. Check yourself in code or in blueprints if it is set"), *Pair.Key->GetFName().ToString());
						Results.Add(Result);
					}
					break;
				}
			}
		}
	}
}


