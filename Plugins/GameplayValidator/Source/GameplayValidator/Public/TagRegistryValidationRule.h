// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GASValidationRule.h"

/**
 * 
 */
class GAMEPLAYVALIDATOR_API TagRegistryValidationRule : public IGASValidationRule
{
public:
	TagRegistryValidationRule();
	void Validate(const GASObjects& Objects, TArray<GASValidationResult>& Results) override final;
};
