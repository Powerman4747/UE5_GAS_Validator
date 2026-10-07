// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Rules/GASValidationRule.h"

/**
 * 
 */
class GAMEPLAYVALIDATOR_API ModifierValidationRule : public IGASValidationRule
{
public:
	ModifierValidationRule();
	virtual void Validate(const FGASObjects& Objects, TArray<FGASValidationResult>& Results) override final;
};
