// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Rules/GASValidationRule.h"

/**
 * 
 */
class GAMEPLAYVALIDATOR_API FRequiredTagValidationRule : IGASValidationRule
{
public:
	FRequiredTagValidationRule(FName InClassInClassName, FName InTagContainerName, TArray<FName> InTag);
	virtual void Validate(const FGASObjects& Objects, TArray<FGASValidationResult>& Results) override final;
	
private:
	FName ClassName;
	FName TagContainerName;
	TArray<FGameplayTag> Tags;
};