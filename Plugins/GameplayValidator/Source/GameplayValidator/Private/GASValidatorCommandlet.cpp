// Fill out your copyright notice in the Description page of Project Settings.

#include "GASValidatorCommandlet.h"
#include "GASValidator.h"

UGASValidatorCommandlet::UGASValidatorCommandlet()
{
	IsServer = false;
	IsClient = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UGASValidatorCommandlet::Main(const FString& Params)
{
	GLog->Log(TEXT("RAW MARKER: GAS Validator started"));
	int errors = UGASValidator::RunValidator();
	return errors;
}
