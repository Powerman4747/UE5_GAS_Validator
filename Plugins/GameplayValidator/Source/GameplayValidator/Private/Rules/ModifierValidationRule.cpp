// Fill out your copyright notice in the Description page of Project Settings.
#include "Rules/ModifierValidationRule.h"

#include "GameplayEffect.h"
#include "GameplayModMagnitudeCalculation.h"
#include "GASValidator.h"

ModifierValidationRule::ModifierValidationRule() :
	IGASValidationRule("ModifierValidation")
{
}

void ModifierValidationRule::Validate(const GASObjects& Objects, TArray<GASValidationResult>& Results) override final
{
	for (const auto& Pair : Objects.Effects)
	{
		const auto& Effect = Pair.Value;
		for (const auto& Modifier : Effect.Modifiers)
		{
			if (!Modifier.Attribute.IsValid())
			{
				// attribute invalid
			}
			
			switch (Modifier.TypeOfCalculation)
			{
				case EGameplayEffectMagnitudeCalculation::ScalableFloat:
				{
					if (FMath::IsNearlyZero(Modifier.FloatValue))
					{
						// invalid
					}
					break;
				}
				case EGameplayEffectMagnitudeCalculation::AttributeBased:
				{
					if (!Modifier.BasedOnAttribute.IsValid())
					{
						// invalid
					}
					break;
				}
				case EGameplayEffectMagnitudeCalculation::CustomCalculationClass:
				{
					if (Modifier.CalculationClassReference == nullptr)
					{
						// invalid
					}
					break;
				}
				case EGameplayEffectMagnitudeCalculation::SetByCaller:
				{
					if (!Modifier.CallableName.IsValid() || !Modifier.CallableTag.IsValid())
					{
						// invalid
					}
					break;
				}
			}
		}
	}
}


