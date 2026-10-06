// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Rules/GASValidationRule.h"


/**
 * 
 */
class GAMEPLAYVALIDATOR_API ConditionalEffectResolveValidationRule : public IGASValidationRule
{
public:
	ConditionalEffectResolveValidationRule();
	virtual void Validate(const GASObjects& Objects, TArray<GASValidationResult>& Results) override final;
};
