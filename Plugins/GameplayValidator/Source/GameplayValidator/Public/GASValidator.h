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
class UGameplayEffect;
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
};

struct FDiscoveredCue
{
	FGameplayTag Tag;
};

struct FDiscoveredEffectReference
{
	FName PropertyName; // could be Cost or Cooldown
	TSubclassOf<UGameplayEffect> EffectClass;
};

struct FDiscoveredEffect
{
	TArray<FDiscoveredCue> Cues;
};

struct FDiscoveredAbility
{
	TArray<FDiscoveredEffectReference> Effects;
};


struct GASObjects
{
	TMap<UClass*, FDiscoveredAttribute> Attributes;
	TMap<UClass*, FDiscoveredTagContainer> TagContainers;
	TMap<UClass*, FDiscoveredAbility> Abilities;
	TMap<UClass*, FDiscoveredEffect> Effects;
	TMap<UClass*, FDiscoveredEffect> Cues;
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
	static UClass* ResolveClass(FProperty* Property, UObject* Instance);
	static TMap<UClass*, FDiscoveredAttribute> FindAttributes(UAbilitySystemComponent* ASC, UClass* Class);
	static TMap<UClass*, FDiscoveredTagContainer> FindTags(UClass* CDO);
	static FDiscoveredAbility FindAbilities(UClass* CDO);
	static FDiscoveredEffect FindEffects(UClass* CDO);
	static TMap<UClass*, FDiscoveredCue> FindCues(UClass* CDO);
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
	inline const auto CONCAT(GAS_VALIDATION_RULE_, __COUNTER__) = []()                  \
	{                                                                                   \
		UGASValidator::AddRule<RuleClass>(__VA_ARGS__);									\
		return true;																	\
}();																					\
}

#define CONCAT(a, b) CONCAT_IMPL(a, b)
#define CONCAT_IMPL(a, b) a##b