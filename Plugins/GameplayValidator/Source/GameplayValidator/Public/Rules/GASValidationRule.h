// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DataStructures/ValidationResult.h"

struct FGASObjects;
class UObject; 

class GAMEPLAYVALIDATOR_API IGASValidationRule
{
public:
	IGASValidationRule(FString Name);
	FString GetRuleName();
	virtual void Validate(const FGASObjects& Objects, TArray<FGASValidationResult>& Results) = 0;
	virtual ~IGASValidationRule() {};
	
protected:
	FGASValidationResult CreateResult(EGASValidationSeverity Severity, FString Message);
	
private:
	FString RuleName;
};
