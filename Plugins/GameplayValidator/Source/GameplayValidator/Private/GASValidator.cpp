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
	GetDerivedClasses(UGameplayEffect::StaticClass(), NativeClasses, true);
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

	UAbilitySystemComponent* ASC = nullptr;
	if (AActor* Actor = Cast<AActor>(Instance))
	{
		if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Actor))
		{
			ASC = ASI->GetAbilitySystemComponent();
		}
	}

	for (TFieldIterator<FProperty> PropertyIterator(Instance->GetClass()); PropertyIterator; ++PropertyIterator)
	{
		UClass* ReferencedClass = UGASValidator::ResolveClass(*PropertyIterator, Instance);
		if (!ReferencedClass)
		{
			continue;
		}

		if (ReferencedClass->IsChildOf(UAttributeSet::StaticClass()) && ASC)
		{
			Objects.Attributes.Append(FindAttributes(ASC, ReferencedClass));
		}
		else if (ReferencedClass->IsChildOf(UGameplayAbility::StaticClass()) 
			   || ReferencedClass->IsChildOf(UGameplayEffect::StaticClass()))
		{
			if (ReferencedClass->IsChildOf(UGameplayAbility::StaticClass()))
			{
				Objects.Abilities.Add(ReferencedClass, FindAbilities(ReferencedClass));
			}
			else
			{
				Objects.Effects.Add(ReferencedClass, FindEffects(ReferencedClass));
			}
			
			Objects.TagContainers.Append(FindTags(ReferencedClass));
		}
		// Future GameplayCueNotify_Actor & _Static
	}

	return Objects;
}

UClass* UGASValidator::ResolveClass(FProperty* Property, UObject* Instance)
{
	if (FClassProperty* ClassProperty = CastField<FClassProperty>(Property))
	{
		return Cast<UClass>(ClassProperty->GetObjectPropertyValue_InContainer(Instance));
	}
	if (FObjectProperty* ObjectProperty = CastField<FObjectProperty>(Property))
	{
		UObject* Value = ObjectProperty->GetObjectPropertyValue_InContainer(Instance);
		return Value ? Value->GetClass() : nullptr;
	}
	
	// TODO: SoftClass
	
	return nullptr;
}

TMap<UClass*, FDiscoveredAttribute> UGASValidator::FindAttributes(UAbilitySystemComponent* ASC, UClass* Class)
{
	TMap<UClass*, FDiscoveredAttribute> Attributes;
	
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
			Attributes.Add(Class, DiscoveredAttribute);
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
		
		Attributes.Add(Class, DiscoveredAttribute);
	}
	
	return Attributes;
}

TMap<UClass*,FDiscoveredTagContainer> UGASValidator::FindTags(UClass* Class)
{
	TMap<UClass*, FDiscoveredTagContainer> Tags;
	if (!Class)
	{
		return Tags;
	}
	
	auto* Value = Class->GetDefaultObject();
	for (TFieldIterator<FStructProperty> PropIt(Class); PropIt; ++PropIt)
	{
		FStructProperty* StructProp = *PropIt;
		if (StructProp->Struct != FGameplayTagContainer::StaticStruct())
		{
			continue;
		}
		
		FDiscoveredTagContainer TagContainer;
		TagContainer.Container = *StructProp->ContainerPtrToValuePtr<FGameplayTagContainer>(Value);
		TagContainer.PropertyName = StructProp->GetFName();		
		Tags.Add(Class, TagContainer);
	}
	
	for (TFieldIterator<FArrayProperty> PropIt(Class); PropIt; ++PropIt)
	{
		FArrayProperty* ArrayProp = *PropIt;
		FStructProperty* InnerStruct = CastField<FStructProperty>(ArrayProp->Inner);
        if (!InnerStruct || InnerStruct->Struct != FGameplayEffectCue::StaticStruct())
        {
        	continue;
        }

        FScriptArrayHelper Helper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(Value));
        for (int32 i = 0; i < Helper.Num(); ++i)
        {
        	const FGameplayEffectCue* Cue = reinterpret_cast<const FGameplayEffectCue*>(Helper.GetRawPtr(i));
        
        	FDiscoveredTagContainer TagContainer;
        	TagContainer.Container = Cue->GameplayCueTags;
        	TagContainer.PropertyName = ArrayProp->GetFName();		
        	Tags.Add(Class, TagContainer);
        }
	}
	return Tags;
}

FDiscoveredAbility UGASValidator::FindAbilities(UClass* Class)
{
	FDiscoveredAbility Ability;
	
	if (!Class)
	{
		return Ability;
	}
	
	auto* CDO = Class->GetDefaultObject();
	for (TFieldIterator<FProperty> PropIt(Class); PropIt; ++PropIt)
	{
		FProperty* Property = *PropIt;
		auto* ReferencedClass = UGASValidator::ResolveClass(Property, CDO);
		if (ReferencedClass)
		{
			continue;
		}
		
		if (ReferencedClass->IsChildOf(UGameplayEffect::StaticClass()))
		{
			FDiscoveredEffectReference EffectReference;
			EffectReference.PropertyName = Property->GetFName();
			EffectReference.EffectClass = ReferencedClass;
			
			Ability.Effects.Add(EffectReference);
		}
		
		// TODO: GEComponents
	}
	return Ability;
}

FDiscoveredEffect UGASValidator::FindEffects(UClass* Class)
{
	FDiscoveredEffect Effect;
	
	if (!Class)
	{
		return Effect;
	}
	
	auto* CDO = Class->GetDefaultObject();

	for (TFieldIterator<FProperty> PropIt(Class); PropIt; ++PropIt)
	{
		/*FProperty* Property = *PropIt;
		FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property);
		if (!ArrayProperty)
		{
			continue;
		}
		
		FStructProperty* InnerStruct = CastField<FStructProperty>(ArrayProperty->Inner);
		if (!InnerStruct || InnerStruct->Struct != FGameplayEffectCue::StaticStruct())
		{
			continue;
		}

		FScriptArrayHelper Helper(ArrayProperty, ArrayProperty->ContainerPtrToValuePtr<void>(CDO));
		for (int32 i = 0; i < Helper.Num(); ++i)
		{
			const FGameplayEffectCue* Cue = reinterpret_cast<const FGameplayEffectCue*>(Helper.GetRawPtr(i));
		
			// currently not needed
		}*/
	}
	
	return Effect;
}

TMap<UClass*, FDiscoveredCue> UGASValidator::FindCues(UClass* CDO)
{
	return TMap<UClass*, FDiscoveredCue>();
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
