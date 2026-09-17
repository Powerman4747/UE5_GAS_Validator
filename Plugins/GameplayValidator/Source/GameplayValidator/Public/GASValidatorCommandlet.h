// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Runtime/Engine/Classes/Commandlets/Commandlet.h"
#include "GASValidatorCommandlet.generated.h"

/**
 * 
 */
UCLASS()
class GAMEPLAYVALIDATOR_API UGASValidatorCommandlet : public UCommandlet
{
	GENERATED_BODY()
	
public:
	UGASValidatorCommandlet();
	int32 Main(const FString &params) override;
	
};
