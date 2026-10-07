// Fill out your copyright notice in the Description page of Project Settings.
#include "Rules/AbilityReachEffectValidationRule.h"

#include "GameplayEffect.h"
#include "GASValidator.h"

AbilityReachEffectValidationRule::AbilityReachEffectValidationRule() :
IGASValidationRule("AbilityReachesEffect")
{
}

void AbilityReachEffectValidationRule::Validate(const GASObjects& Objects, TArray<GASValidationResult>& Results)
{
	for (const auto& Pair : Objects.Abilities)
	{
		const auto& Ability = Pair.Value;
		if (Ability.Effects.Num() == 0)
		{
			continue; // no Effects referenced at all
		}

		bool bAnyEffectHasContent = false;
		for (const FDiscoveredEffectReference& Ref : Ability.Effects)
		{
			if (!Ref.EffectClass)
			{
				GASValidationResult Result;
				Result.RuleName = GetRuleName();
				Result.Severity = EGASValidationSeverity::UNRESOLVED;
				Result.Message = FString::Printf(
					TEXT("'%s': Empty Effect class on '%s'. Could be set in code or blueprints but can't check"), *Pair.Key->GetFName().ToString(), *Ref.PropertyName.ToString());
				Results.Add(Result);
				continue; 
			}

			const FDiscoveredEffect* Effect = Objects.Effects.Find(Ref.EffectClass);
			if (!Effect)
			{
				continue; // should not happen, just safety guard
			}

			if (Effect->Modifiers.Num() > 0 || Effect->Executions.Num() > 0 || Effect->Effects.Num() > 0)
			{
				bAnyEffectHasContent = true;
				break;
			}
		}

		if (!bAnyEffectHasContent)
		{
			GASValidationResult Result;
			Result.RuleName = GetRuleName();
			Result.Severity = EGASValidationSeverity::ERROR;
			Result.Message = FString::Printf(
				TEXT("'%s': Ability doesn't have any effects that do anything"), *Pair.Key->GetFName().ToString());
			Results.Add(Result);
		}
	}
}
