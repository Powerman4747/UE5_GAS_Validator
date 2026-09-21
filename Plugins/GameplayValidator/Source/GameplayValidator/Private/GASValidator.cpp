// Fill out your copyright notice in the Description page of Project Settings.

#include "GASValidator.h"

#include "AbilitySystemInterface.h"
#include "GASValidatorLog.h"
#include "EditorValidatorSubsystem.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"

#include "Engine/Blueprint.h"
#include "Editor.h"
#include "GASValidationRule.h"
#include "NonZeroValidationRule.h"

TArray<TSharedRef<IGASValidationRule>> UGASValidator::Rules;

void UGASValidator::RunValidator()
{
	UE_LOG(LogGASValidator, Log, TEXT("GAS Validator started"));

	UEditorValidatorSubsystem* ValidatorSubsystem = GEditor->GetEditorSubsystem<UEditorValidatorSubsystem>();

	if (Rules.IsEmpty())
	{
		Rules.Add(MakeShared<NonZeroValidationRule>());
	}
	
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
	
	return !GASObjects.Attributes.IsEmpty();
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
		Rule->Validate(GASRelatedFields, Results);
	}
	
	bool bHasError = false;
	for (auto Result : Results)
	{
		UE_LOG(LogGASValidator, Log, TEXT("%s"), *Result.Message)
		
		if (Result.Severity == EGASValidationSeverity::ERROR)
		{
			bHasError = true;
		}
	}
	
	if (bHasError)
	{
		AssetFails(InAsset, FText::FromString(TEXT("GAS validation failed - see errors above")));
		return EDataValidationResult::Invalid;
	}
		
	AssetPasses(InAsset);
	return 	EDataValidationResult::Valid;
}

GASObjects UGASValidator::FindGASRelatedFields(UObject* Class) const
{
	bool bHasASC = false;
	GASObjects Objects;

	if (!Class)
	{
		return Objects;
	}
	
	AActor* Actor = Cast<AActor>(Class);
	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Actor);
	UAbilitySystemComponent* ASC = nullptr;
	if (Actor && ASI)
	{
		ASC = ASI->GetAbilitySystemComponent();
		if (ASC)
		{
			bHasASC = true;
		}
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
					/*if (UAttributeSet* CDO = Cast<UAttributeSet>(Class->GetDefaultObject()))
					{
						Objects.AttributeSets.Add(CDO);
					}*/
					
					Objects.Attributes.Append(this->FindAttributes(ASC, Class));
				}
			}
		}
		else if (FObjectProperty* ObjectProperty = CastField<FObjectProperty>(Property))
		{
			if (ObjectProperty->PropertyClass->IsChildOf(UAttributeSet::StaticClass()))
			{
				UObject* Value = ObjectProperty->GetObjectPropertyValue_InContainer(Class);
				/*if (UAttributeSet* AttributeSet = Cast<UAttributeSet>(Value))
				{
					Objects.AttributeSets.Add(AttributeSet);
				}*/
				Objects.Attributes.Append(this->FindAttributes(ASC, Value->GetClass()));

			}
		}
	}
	
	return Objects;
}

TArray<FDiscoveredAttribute> UGASValidator::FindAttributes(UAbilitySystemComponent* ASC, UClass* Class) const
{
	TArray<FDiscoveredAttribute> Attributes;
	
	UDataTable* MetaTable = nullptr;
	if (!ASC->DefaultStartingData.IsEmpty())
	{
		
		for (auto Defaults : ASC->DefaultStartingData)
		{
			if (Defaults.Attributes.Get() == Class)
			{
				MetaTable = Defaults.DefaultStartingTable;
				break;
			}
		}
	}	
	
	for (TFieldIterator<FStructProperty> PropIt(Class); PropIt; ++PropIt)
	{							
		FStructProperty* StructProp = *PropIt;
		if (StructProp->Struct != FGameplayAttributeData::StaticStruct())
		{
			continue;
		}
						
		FDiscoveredAttribute DiscoveredAttribute;
		DiscoveredAttribute.Attribute = FGameplayAttribute(StructProp);
		if (auto MetaData = StructProp->GetMetaDataMap())
        {
        	DiscoveredAttribute.Metadata = *MetaData;
        }
		
		if (!MetaTable)
		{
			Attributes.Add(DiscoveredAttribute);
			continue;
		}
		DiscoveredAttribute.SourceOfValue = MetaTable->GetFName();

		//Get value
		const FName AttributeName = StructProp->GetFName();
		const FString RowNameString = FString::Printf(TEXT("%s.%s"), *Class->GetName(), *AttributeName.ToString());
		const FName RowName(*RowNameString);
		const FAttributeMetaData* Row = MetaTable->FindRow<FAttributeMetaData>(RowName, TEXT("GASValidator"));
						
		if (Row)
		{
			float BaseValue = Row->BaseValue;
			DiscoveredAttribute.Value = BaseValue;
		}
		
		Attributes.Add(DiscoveredAttribute);
	}
	
	return Attributes;
}

