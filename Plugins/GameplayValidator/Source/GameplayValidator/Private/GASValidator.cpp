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
#include "TagRegistryValidationRule.h"

TArray<TSharedRef<IGASValidationRule>> UGASValidator::Rules;

#ifndef USE_NO_STANDARD_RULES
GAS_VALIDATION_REGISTER_RULE(NonZeroValidationRule); 
GAS_VALIDATION_REGISTER_RULE(TagRegistryValidationRule); 
#endif

void UGASValidator::RunValidator()
{
	UE_LOG(LogGASValidator, Log, TEXT("GAS Validator started"));

	UEditorValidatorSubsystem* ValidatorSubsystem = GEditor->GetEditorSubsystem<UEditorValidatorSubsystem>();
	
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
	
	// NEW: separately handle native classes, which the above will never reach
	TArray<UClass*> NativeClasses;
	TArray<GASValidationResult> ValidationResults;
	GetDerivedClasses(UGameplayAbility::StaticClass(), NativeClasses, true);
	
	
	for (UClass* Class : NativeClasses)
	{
		if (Class->ClassGeneratedBy != nullptr) continue; // skip Blueprint-generated
		GASObjects Objects = UGASValidator::FindGASRelatedFields(Class->GetDefaultObject());
		
		for (auto Rule : Rules)
		{
			Rule->Validate(Objects, ValidationResults);
		}
	}
	
	NativeClasses.Empty();
	GetDerivedClasses(UGameplayAbility::StaticClass(), NativeClasses, true);
	for (UClass* Class : NativeClasses)
	{
		if (Class->ClassGeneratedBy != nullptr) continue; // skip Blueprint-generated
		GASObjects Objects = UGASValidator::FindGASRelatedFields(Class->GetDefaultObject());
		
		for (auto Rule : Rules)
		{
			Rule->Validate(Objects, ValidationResults);
		}
	}
	
	UGASValidator::LogResults(ValidationResults);
	
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
	
	bool bIsAbility = InAsset->IsA(UGameplayAbility::StaticClass());
	bool bIsEffect = InAsset->IsA(UGameplayEffect::StaticClass());
	
	return bIsAbility || bIsEffect || !GASObjects.Attributes.IsEmpty() || !GASObjects.TagContainers.IsEmpty();
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
	
	bool bHasError = UGASValidator::LogResults(Results);
	
	if (bHasError)
	{
		AssetFails(InAsset, FText::FromString(TEXT("GAS validation failed - see errors above")));
		return EDataValidationResult::Invalid;
	}
		
	AssetPasses(InAsset);
	return 	EDataValidationResult::Valid;
}

GASObjects UGASValidator::FindGASRelatedFields(UObject* Instance)
{
	GASObjects Objects;

	if (!Instance)
	{
		return Objects;
	}
	
	if (Instance->IsA(UGameplayAbility::StaticClass()) || Instance->IsA(UGameplayEffect::StaticClass()))
	{
		Objects.TagContainers.Append(FindTags(Instance->GetClass()));
		return Objects; 
	}	
	
	AActor* Actor = Cast<AActor>(Instance);	
	UAbilitySystemComponent* ASC = nullptr;
	if (Actor)
	{
		IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Actor);
		if (ASI)
		{
			ASC = ASI->GetAbilitySystemComponent();
		}
	}

	for (TFieldIterator<FProperty> PropertyIterator(Instance->GetClass()); PropertyIterator; ++PropertyIterator)
	{
		FProperty* Property = *PropertyIterator;
		
		if(FClassProperty* ClassProperty = CastField<FClassProperty>(Property))
		{
			if (ClassProperty->MetaClass->IsChildOf(UAttributeSet::StaticClass()))
			{
				UObject* ClassValue = ClassProperty->GetObjectPropertyValue_InContainer(Instance);
				if (UClass* Class = Cast<UClass>(ClassValue))
				{
					/*if (UAttributeSet* CDO = Cast<UAttributeSet>(Class->GetDefaultObject()))
					{
						Objects.AttributeSets.Add(CDO);
					}*/
					if (ASC)
					{
						Objects.Attributes.Append(UGASValidator::FindAttributes(ASC, Class));
					}
				}
			}
			else if (ClassProperty->MetaClass->IsChildOf(UGameplayAbility::StaticClass()) 
						|| ClassProperty->MetaClass->IsChildOf(UGameplayEffect::StaticClass()))
			{
				UObject* ClassValue = ClassProperty->GetObjectPropertyValue_InContainer(Instance);
				if (UClass* Class = Cast<UClass>(ClassValue))
				{
					Objects.TagContainers.Append(UGASValidator::FindTags(Class));
				}
			}
		}
		else if (FObjectProperty* ObjectProperty = CastField<FObjectProperty>(Property))
		{
			if (ObjectProperty->PropertyClass->IsChildOf(UAttributeSet::StaticClass()))
			{
				UObject* Value = ObjectProperty->GetObjectPropertyValue_InContainer(Instance);
				
				/*if (UAttributeSet* AttributeSet = Cast<UAttributeSet>(Value))
				{
					Objects.AttributeSets.Add(AttributeSet);
				}*/
				
				if (ASC)
				{
					Objects.Attributes.Append(UGASValidator::FindAttributes(ASC, Value->GetClass()));
				}
			}
			else if (ObjectProperty->PropertyClass->IsChildOf(UGameplayAbility::StaticClass()) 
						|| ObjectProperty->PropertyClass->IsChildOf(UGameplayEffect::StaticClass()))
			{
				UObject* Value = ObjectProperty->GetObjectPropertyValue_InContainer(Instance);

				Objects.TagContainers.Append(UGASValidator::FindTags(Value->GetClass()));
			}
		}
	}
	
	return Objects;
}

TArray<FDiscoveredAttribute> UGASValidator::FindAttributes(UAbilitySystemComponent* ASC, UClass* Class)
{
	TArray<FDiscoveredAttribute> Attributes;
	
	if (!Class)
	{
		return Attributes;
	}
	
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

TArray<FDiscoveredTagContainer> UGASValidator::FindTags(UClass* Class)
{
	TArray<FDiscoveredTagContainer> Tags;
	if (!Class)
	{
		return Tags;
	}
	
	for (TFieldIterator<FStructProperty> PropIt(Class); PropIt; ++PropIt)
	{
		FStructProperty* StructProp = *PropIt;
		if (StructProp->Struct != FGameplayTagContainer::StaticStruct())
		{
			continue;
		}
		
		FDiscoveredTagContainer TagContainer;
		
		auto* Value = Class->GetDefaultObject();
		TagContainer.Container = *StructProp->ContainerPtrToValuePtr<FGameplayTagContainer>(Value);
		TagContainer.PropertyName = StructProp->GetFName();
		TagContainer.Class = Class->GetFName();
		
		Tags.Add(TagContainer);
	}
	return Tags;
}

bool UGASValidator::LogResults(TArray<GASValidationResult>& Results)
{
	bool bHasError = false;
	for (auto Result : Results)
	{
		UE_LOG(LogGASValidator, Log, TEXT("%s"), *Result.Message)
		
		if (Result.Severity == EGASValidationSeverity::ERROR)
		{
			bHasError = true;
		}
	}
	
	return bHasError;
}
