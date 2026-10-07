// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GASValidationRule.h"

/**
 * 
 */
class GAMEPLAYVALIDATOR_API FAbilityReachEffectValidationRule : public IGASValidationRule
{
public:
	FAbilityReachEffectValidationRule();
	virtual void Validate(const FGASObjects& Objects, TArray<FGASValidationResult>& Results) override final;
};
