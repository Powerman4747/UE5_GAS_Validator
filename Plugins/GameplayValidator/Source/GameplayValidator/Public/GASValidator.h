// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "EditorValidatorBase.h"
#include "GameplayTagContainer.h"
#include "GASValidator.generated.h"

/**
 * 
 */

struct GASValidationResult;
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

struct FDiscoveredTagContainer
{
	FName PropertyName;
	FGameplayTagContainer Container;
	FName Class;
};

struct GASObjects
{
	TArray<FDiscoveredAttribute> Attributes;
	TArray<FDiscoveredTagContainer> TagContainers;
};

UCLASS()
class GAMEPLAYVALIDATOR_API UGASValidator : public UEditorValidatorBase
{
	GENERATED_BODY()
public:
	static void RunValidator();	
	template<typename T, typename... TArgs>
	static void AddRule(TArgs&&... Args);
	bool CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const override;
	EDataValidationResult ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context) override;
	
private:
	static GASObjects FindGASRelatedFields(UObject* Class);
	static TArray<FDiscoveredAttribute> FindAttributes(UAbilitySystemComponent* ASC, UClass* Class);
	static TArray<FDiscoveredTagContainer> FindTags(UClass* CDO);
	static bool LogResults(TArray<GASValidationResult>& Results);
	static TArray<TSharedRef<IGASValidationRule>> Rules;
};

template <typename T, typename... TArgs>
void UGASValidator::AddRule(TArgs&&... Args)
{
	Rules.Add(MakeShared<T>(Forward<TArgs>(Args)...));
}

#define GAS_VALIDATION_REGISTER_RULE(RuleClass, ...)									\
namespace																				\
{																						\
	inline const auto CONCAT(GAS_VALIDATION_RULE_, __COUNTER__) = []()                             \
	{                                                                                   \
		UGASValidator::AddRule<RuleClass>(__VA_ARGS__);									\
		return true;																	\
}();																					\
}

#define CONCAT(a, b) CONCAT_IMPL(a, b)
#define CONCAT_IMPL(a, b) a##b