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

int32 UGASValidatorCommandlet::Main(const FString& params)
{
	GLog->Log(TEXT("RAW MARKER: GAS Validator started"));
	UGASValidator::RunValidator();
	return 0;
}
