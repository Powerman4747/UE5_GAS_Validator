// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

struct GASObjects;
class UObject; 

enum class EGASValidationSeverity
{
	INFO,
	WARNING,
	ERROR
};

struct GASValidationResult
{
	FString RuleName;
	EGASValidationSeverity Severity;
	FString Message; // The reason of a warning or error
};

class GAMEPLAYVALIDATOR_API IGASValidationRule
{
public:
	IGASValidationRule(FString Name);
	FString GetRuleName();
	virtual void Validate(const GASObjects& Objects, TArray<GASValidationResult>& Results) = 0;
	virtual ~IGASValidationRule() {};
	
private:
	FString RuleName;
};
