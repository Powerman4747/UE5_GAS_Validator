// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Rules/GASValidationRule.h"

struct FAttributeDefaults;
class UAttributeSet;
class UAbilitySystemComponent;
/**
 * 
 */
class GAMEPLAYVALIDATOR_API FNonZeroValidationRule : public IGASValidationRule
{
public:
	FNonZeroValidationRule();
	virtual void Validate(const FGASObjects& Objects, TArray<FGASValidationResult>& Results) override final;
};
