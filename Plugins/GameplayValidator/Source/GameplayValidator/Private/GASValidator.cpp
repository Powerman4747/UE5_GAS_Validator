// Fill out your copyright notice in the Description page of Project Settings.

#include "GASValidator.h"
#include "GASValidatorLog.h"
#include "EditorValidatorSubsystem.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AttributeSet.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayEffect.h"
#include "GameplayTagContainer.h"
#include "Editor.h"

void UGASValidator::RunValidator()
{
	UE_LOG(LogGASValidator, Log, TEXT( "GAS Validator started" ));

	UEditorValidatorSubsystem* ValidatorSubsystem = 
	GEditor->GetEditorSubsystem<UEditorValidatorSubsystem>();

	FValidateAssetsSettings Settings;
	Settings.bShowIfNoFailures = false;

	FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	TArray<FAssetData> AssetDataList;
	AssetRegistryModule.Get().GetAssetsByPath(FName("/Game"), AssetDataList, true);
	
	if (AssetDataList.Num() <= 0)
	{
		UE_LOG(LogGASValidator, Log, TEXT("No assets to validate"));
	}
	
	FValidateAssetsResults Results;
	ValidatorSubsystem->ValidateAssetsWithSettings(
		AssetDataList, Settings, Results);
	
	UE_LOG(LogGASValidator, Log, TEXT("GAS Validator ended"));
}

bool UGASValidator::CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset,
	FDataValidationContext& InContext) const
{
	auto GASObjects = this->FindGASRelatedFields(InAsset);
	
	return !GASObjects.AttributeSets.IsEmpty();
}

EDataValidationResult UGASValidator::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset,
                                                         FDataValidationContext& Context)
{
	auto GASRelatedFields = FindGASRelatedFields(InAsset);
	
	for (UAttributeSet* Field: GASRelatedFields.AttributeSets)
	{
		for (TFieldIterator<FProperty> It(Field->GetClass()); It; ++It)
		{
			if (FStructProperty* StructProperty = CastField<FStructProperty>(*It))
			{
				if (StructProperty->Struct == FGameplayAttributeData::StaticStruct())
				{
					FGameplayAttributeData* AttrData = StructProperty->ContainerPtrToValuePtr<FGameplayAttributeData>(Field);
					const float Base = AttrData->GetBaseValue();
					UE_LOG(LogGASValidator, Log, TEXT("%s: Base=%.2f"), *StructProperty->GetName(), Base);

				}
			}
		}
	}
	AssetPasses(InAsset);
	return 	EDataValidationResult::Valid;
}

GASObjects UGASValidator::FindGASRelatedFields(UObject* Class) const
{
	GASObjects Objects;
	
	if (UBlueprint* Blueprint = Cast<UBlueprint>(Class))
	{
		if (Blueprint->GeneratedClass)
		{
			Class = Blueprint->GeneratedClass->GetDefaultObject();
		}
		else
		{
			return Objects; // no generated class yet (e.g. Blueprint has compile errors)
		}
	}

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

