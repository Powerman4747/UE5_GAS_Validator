// Fill out your copyright notice in the Description page of Project Settings.

#include "GASValidator.h"
#include "GASValidatorLog.h"
#include "EditorValidatorSubsystem.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AttributeSet.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayEffect.h"
#include "GameplayTagContainer.h"
#include "Engine/Blueprint.h"
#include "Editor.h"
#include "GASValidationRule.h"
#include "NonZeroValidationRule.h"

TArray<TSharedRef<IGASValidationRule>> UGASValidator::Rules;

void UGASValidator::RunValidator()
{
	UE_LOG(LogGASValidator, Log, TEXT("GAS Validator started"));

	UEditorValidatorSubsystem* ValidatorSubsystem = GEditor->GetEditorSubsystem<UEditorValidatorSubsystem>();

	Rules.Add(MakeShared<NonZeroValidationRule>());
	FValidateAssetsSettings Settings;
	Settings.bShowIfNoFailures = true;

	FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	TArray<FAssetData> AssetDataList;
	AssetRegistryModule.Get().GetAssetsByPath(FName("/Game"), AssetDataList, true);
	
	if (AssetDataList.Num() <= 0)
	{
		UE_LOG(LogGASValidator, Log, TEXT("No assets to validate"));
	}
	
	FValidateAssetsResults Results;
	ValidatorSubsystem->ValidateAssetsWithSettings(AssetDataList, Settings, Results);
	
	UE_LOG(LogGASValidator, Log, TEXT("GAS Validator ended"));
}

bool UGASValidator::CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset,
	FDataValidationContext& InContext) const
{
	if (UBlueprint* Blueprint = Cast<UBlueprint>(InAsset))
	{
		if (Blueprint->GeneratedClass)
		{
			InAsset = Blueprint->GeneratedClass->GetDefaultObject();
		}
	}
	auto GASObjects = this->FindGASRelatedFields(InAsset);
	
	return !GASObjects.AttributeSets.IsEmpty();
}

EDataValidationResult UGASValidator::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset,
                                                         FDataValidationContext& Context)
{
	if (UBlueprint* Blueprint = Cast<UBlueprint>(InAsset))
	{
		if (Blueprint->GeneratedClass)
		{
			InAsset = Blueprint->GeneratedClass->GetDefaultObject();
		}
	}
	auto GASRelatedFields = FindGASRelatedFields(InAsset);
	TArray<GASValidationResult> Results;
	for (auto Rule : Rules)
	{
		Rule->Validate(InAsset, Results);
	}
	
	for (auto Result : Results)
	{
		UE_LOG(LogGASValidator, Log, TEXT("%s"), *Result.Message)
	}
	AssetPasses(InAsset);
	return 	EDataValidationResult::Valid;
}

GASObjects UGASValidator::FindGASRelatedFields(UObject* Class) const
{
	GASObjects Objects;

	if (!Class)
	{
		return Objects;
	}
	
	for (TFieldIterator<FProperty> PropertyIterator(Class->GetClass()); PropertyIterator; ++PropertyIterator)
	{
		FProperty* Property = *PropertyIterator;
		
		if(FClassProperty* ClassProperty = CastField<FClassProperty>(Property))
		{
			if (ClassProperty->MetaClass->IsChildOf(UAttributeSet::StaticClass()))
			{
				UObject* ClassValue = ClassProperty->GetObjectPropertyValue_InContainer(Class);
				if (UClass* Class = Cast<UClass>(ClassValue))
				{
					if (UAttributeSet* CDO = Cast<UAttributeSet>(Class->GetDefaultObject()))
					{
						Objects.AttributeSets.Add(CDO);
					}
				}
			}
		}
		else if (FObjectProperty* ObjectProperty = CastField<FObjectProperty>(Property))
		{
			if (ObjectProperty->PropertyClass->IsChildOf(UAttributeSet::StaticClass()))
			{
				UObject* Value = ObjectProperty->GetObjectPropertyValue_InContainer(Class);
				if (UAttributeSet* AttributeSet = Cast<UAttributeSet>(Value))
				{
					Objects.AttributeSets.Add(AttributeSet);
				}
			}
		}
	}
	
	return Objects;
}

