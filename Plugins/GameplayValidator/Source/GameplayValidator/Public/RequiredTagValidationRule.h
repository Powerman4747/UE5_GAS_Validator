// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GASValidationRule.h"

/**
 * 
 */
class GAMEPLAYVALIDATOR_API RequiredTagValidationRule : IGASValidationRule
{
public:
	RequiredTagValidationRule(FName InClassInClassName, FName InTagContainerName, TArray<FName> InTag);
	void Validate(const GASObjects& Objects, TArray<GASValidationResult>& Results) override;
	
private:
	FName ClassName;
	FName TagContainerName;
	TArray<FGameplayTag> Tags;
};