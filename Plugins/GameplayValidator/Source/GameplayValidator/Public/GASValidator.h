// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "EditorValidatorBase.h"
#include "GASValidator.generated.h"

/**
 * 
 */

class UAttributeSet;
class IGASValidationRule;
class UAbilitySystemComponent;

struct FDiscoveredAttribute
{
	FGameplayAttribute Attribute; 
	TOptional<FName> SourceOfValue;
	TOptional<float> Value;
	TMap<FName, FString> Metadata;
};

struct GASObjects
{
	TArray<FDiscoveredAttribute> Attributes;
};

UCLASS()
class GAMEPLAYVALIDATOR_API UGASValidator : public UEditorValidatorBase
{
	GENERATED_BODY()
public:
	static void RunValidator();	
	bool CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const override;
	EDataValidationResult ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context) override;
	
private:
	GASObjects FindGASRelatedFields(UObject* Class) const;
	TArray<FDiscoveredAttribute> FindAttributes(UAbilitySystemComponent* ASC, UClass* Class) const;
	static TArray<TSharedRef<IGASValidationRule>> Rules;
};

