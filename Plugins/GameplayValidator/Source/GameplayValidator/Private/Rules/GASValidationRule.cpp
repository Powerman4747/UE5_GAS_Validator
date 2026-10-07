// Fill out your copyright notice in the Description page of Project Settings.


#include "Rules/GASValidationRule.h"

IGASValidationRule::IGASValidationRule(FString Name)
{
	RuleName = Name;
}

FString IGASValidationRule::GetRuleName()
{
	return RuleName;
}

GASValidationResult IGASValidationRule::CreateResult(EGASValidationSeverity Severity, FString Message)
{
	GASValidationResult Result;
	Result.RuleName = GetRuleName();
	Result.Severity = Severity;
	Result.Message = Message;
	return Result;
}
