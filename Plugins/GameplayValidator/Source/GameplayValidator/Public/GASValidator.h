// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "EditorValidatorBase.h"
#include "DataStructures/ValidationResult.h"
#include "GASValidator.generated.h"

/**
 * 
 */

struct GASObjects;
class IGASValidationRule;
struct FDiscoveredCalculation;
struct FDiscoveredTagContainer;
struct FDiscoveredAttribute;
struct FDiscoveredEffect;
struct FDiscoveredAbility;
struct GASValidationResult;

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
	// Find the GAS Objects
	static void FindGASObjects(UObject* Class, GASObjects& GASObjects, TSet<UClass*>& VisitedClasses);	
	static void FindGASObjectsInStruct(UClass* Class, const void* StructInstance, UScriptStruct* StructType, GASObjects& GASObjects, TSet<UClass*>& VisitedClasses);
	static bool HasGASProperties(UClass* Class);
	
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