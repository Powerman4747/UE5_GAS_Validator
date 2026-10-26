// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "EditorValidatorBase.h"
#include "DataStructures/ValidationResult.h"
#include "GASValidator.generated.h"

/**
 * 
 */


class IGASValidationRule;
class UBlueprint;
struct FGASObjects;

UCLASS()
class GAMEPLAYVALIDATOR_API UGASValidator : public UEditorValidatorBase
{
	GENERATED_BODY()
public:
	static void RunValidator();	
	
	template<typename T, typename... TArgs>
	static void AddRule(TArgs&&... Args);
	
	template<typename T, typename... TArgs>
	static void AddBatchRule(TArgs&&... Args);
	
	template<typename T>
    static void AddIgnoredClass();
	
protected:
	virtual bool CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const override;
	virtual EDataValidationResult ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context) override;
	
private:
	static bool CanValidate(UObject* Asset);
	static void Validate(UObject* Asset, FGASObjects& GASObjects, TSet<UClass*>& VisitedClasses);
	
	// helper function
	static void DiscoverNativeClasses (const TArray<UClass*>& Class, FGASObjects& GASObjects, TSet<UClass*>& VisitedClasses, FScopedSlowTask& SlowTask);
	
	// Find the GAS Objects
	static void FindGASObjects(UObject* Class, FGASObjects& GASObjects, TSet<UClass*>& VisitedClasses);	
	static void FindGASObjectsInStruct(UClass* Class, const void* StructInstance, UScriptStruct* StructType, FGASObjects& GASObjects, TSet<UClass*>& VisitedClasses);
	static void FindGASObjectsInBlueprint(UBlueprint* Blueprint, FGASObjects& GASObjects, TSet<UClass*>& VisitedClasses);	
	static bool HasGASProperties(UClass* Class);
	
	static bool LogResults(TArray<FGASValidationResult>& Results, FString AssetName = "");
	static TArray<TSharedRef<IGASValidationRule>> BatchRules;
	static TArray<TSharedRef<IGASValidationRule>> AssetRules;
	static TSet<const UClass*> IgnoredClasses;
};

template <typename T, typename... TArgs>
void UGASValidator::AddRule(TArgs&&... Args)
{
	AssetRules.Add(MakeShared<T>(Forward<TArgs>(Args)...));
}

template <typename T, typename... TArgs>
void UGASValidator::AddBatchRule(TArgs&&... Args)
{
	BatchRules.Add(MakeShared<T>(Forward<TArgs>(Args)...));
}

template <typename T>
void UGASValidator::AddIgnoredClass()
{
	IgnoredClasses.Add(T::StaticClass());
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

#define GAS_VALIDATION_REGISTER_BATCH_RULE(RuleClass, ...)									\
namespace																				\
{																						\
	inline const auto CONCAT(GAS_VALIDATION_BATCH_RULE_, __COUNTER__) = []()                  \
	{                                                                                   \
		UGASValidator::AddBatchRule<RuleClass>(__VA_ARGS__);									\
		return true;																	\
	}();																					\
}

#define GAS_VALIDATION_REGISTER_IGNORED_CLASS(IgnoredClass)									\
namespace																				\
{																						\
	inline const auto CONCAT(GAS_VALIDATION_IGNORED_CLASS, __COUNTER__) = []()                  \
	{                                                                                   \
		UGASValidator::AddIgnoredClass<IgnoredClass>();									\
		return true;																	\
	}();																					\
}

#define CONCAT(a, b) CONCAT_IMPL(a, b)
#define CONCAT_IMPL(a, b) a##b