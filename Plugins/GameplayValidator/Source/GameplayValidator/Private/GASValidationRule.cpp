// Fill out your copyright notice in the Description page of Project Settings.


#include "GASValidationRule.h"


IGASValidationRule::IGASValidationRule(FString Name)
{
	RuleName = Name;
}

FString IGASValidationRule::GetRuleName()
{
	return RuleName;
}
