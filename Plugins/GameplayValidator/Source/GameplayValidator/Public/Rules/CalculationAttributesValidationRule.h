// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GASValidationRule.h"

/**
 * 
 */
class GAMEPLAYVALIDATOR_API FCalculationAttributesValidationRule : public IGASValidationRule
{
public:
	FCalculationAttributesValidationRule();
	virtual void Validate(const FGASObjects& Objects, TArray<FGASValidationResult>& Results) override final;
};
