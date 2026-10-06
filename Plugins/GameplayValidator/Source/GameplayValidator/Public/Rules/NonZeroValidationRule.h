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
class GAMEPLAYVALIDATOR_API NonZeroValidationRule : public IGASValidationRule
{
public:
	NonZeroValidationRule();
	virtual void Validate(const GASObjects& Objects, TArray<GASValidationResult>& Results) override final;
};
