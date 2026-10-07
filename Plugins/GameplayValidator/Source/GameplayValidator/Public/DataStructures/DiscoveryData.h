#pragma once
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "AttributeSet.h"

enum class EGameplayEffectMagnitudeCalculation : uint8;
class UGameplayEffectCalculation;
class UGameplayEffect;

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


struct FGASObjects
{
	TMap<UClass*, TArray<FDiscoveredAttribute>> Attributes;
	TMap<UClass*, TArray<FDiscoveredTagContainer>> TagContainers;
	TMap<UClass*, FDiscoveredAbility> Abilities;
	TMap<UClass*, FDiscoveredEffect> Effects;
	//TMap<UClass*, FDiscoveredEffect> Cues;
	TMap<UClass*, FDiscoveredCalculation> Calculations;
};