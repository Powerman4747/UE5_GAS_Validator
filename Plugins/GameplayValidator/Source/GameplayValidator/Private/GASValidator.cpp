// Fill out your copyright notice in the Description page of Project Settings.

#include "GASValidator.h"
#include "EditorValidatorSubsystem.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AttributeSet.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayEffect.h"
#include "GameplayTagContainer.h"
#include "Editor.h"

void UGASValidator::RunValidator()
{
	UEditorValidatorSubsystem* ValidatorSubsystem = 
	GEditor->GetEditorSubsystem<UEditorValidatorSubsystem>();

	FValidateAssetsSettings Settings;
	Settings.bShowIfNoFailures = true;

	FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	TArray<FAssetData> AssetDataList;
	AssetRegistryModule.Get().GetAllAssets(AssetDataList);
	
	FValidateAssetsResults Results;
	ValidatorSubsystem->ValidateAssetsWithSettings(
		AssetDataList, Settings, Results);
}

EDataValidationResult UGASValidator::ValidateLoadedAsset(const FAssetData& InAssetData, UObject* InAsset,
	FDataValidationContext& Context)
{
	
	return 	EDataValidationResult::NotValidated;
}

TArray<UObject*> UGASValidator::FindGASRelatedFields(UClass* Class)
{
	for (TFieldIterator<FProperty> PropertyIterator(Class); PropertyIterator; ++PropertyIterator)
	{
		FProperty* Property = *PropertyIterator;
		
		if(FClassProperty* ClassProperty = CastField<FClassProperty>(Property))
		{
			if (ClassProperty->MetaClass->IsChildOf(UAttributeSet::StaticClass()))
			{
				
			}
		}
	}
	
	return TArray<UObject*>();
}

