// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "EditorValidatorBase.h"
#include "GameplayTagContainer.h"
#include "ScalableFloat.h"
#include "GASValidator.generated.h"

/**
 * 
 */


class UGameplayEffectCalculation;
enum class EGameplayEffectMagnitudeCalculation : uint8;
class UGameplayModMagnitudeCalculation;
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

struct FDiscoveredCalculationReference
{
	FName PropertyName;
	TSubclassOf<UGameplayEffectCalculation> CalculationClass;
};

struct FDiscoveredModifier
{
	FGameplayAttribute Attribute; // do not like this bu needs to have for checking... has no value
	
	EGameplayEffectMagnitudeCalculation TypeOfCalculation;
	
	// Scalable float
	float FloatValue;
	
	// Custom Calculation Class
	FDiscoveredCalculationReference CalculationClassReference;
	
	// AttributeBased
	FGameplayAttribute BasedOnAttribute;
	
	// SetByCaller
	FGameplayTag CallableTag;
	FName CallableName; // only code or blueprint
};

struct FDiscoveredCalculation
{
	TArray<FGameplayAttribute> CapturedAttributes;
};

struct FDiscoveredEffect
{
	TArray<FDiscoveredCue> Cues;
	TArray<FDiscoveredModifier> Modifiers;
	TArray<FDiscoveredCalculationReference> Executions;
	TArray<FDiscoveredEffectReference> Effects;
};

struct FDiscoveredAbility
{
	TArray<FDiscoveredEffectReference> Effects;
};


struct GASObjects
{
	TMap<UClass*, TArray<FDiscoveredAttribute>> Attributes;
	TMap<UClass*, TArray<FDiscoveredTagContainer>> TagContainers;
	TMap<UClass*, FDiscoveredAbility> Abilities;
	TMap<UClass*, FDiscoveredEffect> Effects;
	//TMap<UClass*, FDiscoveredEffect> Cues;
	TMap<UClass*, FDiscoveredCalculation> Calculations;
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
	// discover GAS Objects
	static FDiscoveredAbility& DiscoverAbility(UClass* Class, GASObjects& GASObjects);
	static FDiscoveredEffect& DiscoverEffect(UClass* Class, GASObjects& GASObjects);
	static TArray<FDiscoveredAttribute>& DiscoverAttributes(UClass* Class, GASObjects& GASObjects);
	static TArray<FDiscoveredTagContainer>& DiscoverTags(UClass* Class, const void* Instance, GASObjects& GASObjects);
	static FDiscoveredCalculation& DiscoverCalculations(UClass* Class, GASObjects& GASObjects);
	
	// Find the GAS Objects
	static void FindGASObjects(UObject* Class, GASObjects& GASObjects, TSet<UClass*>& VisitedClasses);	
	static void FindGASObjectsInStruct(UClass* Class, const void* StructInstance, UScriptStruct* StructType, GASObjects& GASObjects, TSet<UClass*>& VisitedClasses);
	static bool HasGASProperties(UClass* Class);
	
	// Resolving types and values
	static UClass* ResolvePropertyType(FProperty* Property);
	static UClass* ResolveClassValue(FProperty* Property, UObject* Instance);
	static UClass* ResolveClassValueFromElement(FProperty* Property, const void* ElementPtr);
	
	// Helper recursion function(s)
	static void RecurseArray(UClass* Class, FArrayProperty* Prop, const void* Instance, GASObjects& GASObjects, TSet<UClass*>& VisitedClasses);
	
	static bool LogResults(TArray<GASValidationResult>& Results, FString AssetName = "");
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