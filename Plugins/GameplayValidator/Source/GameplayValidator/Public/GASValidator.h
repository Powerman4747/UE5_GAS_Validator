// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EditorValidatorBase.h"
#include "GASValidator.generated.h"

/**
 * 
 */
UCLASS()
class GAMEPLAYVALIDATOR_API UGASValidator : public UEditorValidatorBase
{
	GENERATED_BODY()
public:
	static void RunValidator();
	EDataValidationResult ValidateLoadedAsset(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context);
	
private:
	TArray<UObject*> FindGASRelatedFields(UClass* Class);
	
};

