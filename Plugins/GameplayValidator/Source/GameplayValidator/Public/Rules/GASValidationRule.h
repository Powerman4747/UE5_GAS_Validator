// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DataStructures/ValidationResult.h"

struct GASObjects;
class UObject; 

class GAMEPLAYVALIDATOR_API IGASValidationRule
{
public:
	IGASValidationRule(FString Name);
	FString GetRuleName();
	virtual void Validate(const GASObjects& Objects, TArray<GASValidationResult>& Results) = 0;
	virtual ~IGASValidationRule() {};
	
protected:
	GASValidationResult CreateResult(EGASValidationSeverity Severity, FString Message);
	
private:
	FString RuleName;
};
