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
#include "GameplayCueSet.h"
#include "GameplayEffectComponent.h"
#include "GameplayModMagnitudeCalculation.h"
#include "Rules/CalculationAttributesValidationRule.h"
#include "Rules/ConditionalEffectResolveValidationRule.h"
#include "Rules/ExecutionValidationRule.h"
#include "Rules/GASValidationRule.h"
#include "Rules/ModifierValidationRule.h"
#include "Rules/NonZeroValidationRule.h"
#include "Rules/TagRegistryValidationRule.h"

TArray<TSharedRef<IGASValidationRule>> UGASValidator::Rules;

#ifndef USE_NO_STANDARD_RULES
GAS_VALIDATION_REGISTER_RULE(NonZeroValidationRule); 
GAS_VALIDATION_REGISTER_RULE(TagRegistryValidationRule); 
GAS_VALIDATION_REGISTER_RULE(ConditionalEffectResolveValidationRule); 
GAS_VALIDATION_REGISTER_RULE(ModifierValidationRule); 
GAS_VALIDATION_REGISTER_RULE(ExecutionValidationRule); 
GAS_VALIDATION_REGISTER_RULE(CalculationAttributesValidationRule);
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
	GetDerivedClasses(UGameplayAbility::StaticClass(), NativeClasses, true);
	
	GASObjects Objects;
	TSet<UClass*> VisitedClasses;
	for (UClass* Class : NativeClasses)
	{
		if (Class->ClassGeneratedBy != nullptr) continue; // skip Blueprint-generated
		
		UGASValidator::FindGASObjects(Class->GetDefaultObject(), Objects, VisitedClasses);
	}
	
	NativeClasses.Empty();
	GetDerivedClasses(UGameplayEffect::StaticClass(), NativeClasses, true);
	for (UClass* Class : NativeClasses)
	{
		if (Class->ClassGeneratedBy != nullptr) continue; // skip Blueprint-generated
		
		UGASValidator::FindGASObjects(Class->GetDefaultObject(), Objects, VisitedClasses);
	}
	
	TArray<GASValidationResult> ValidationResults;
	for (auto Rule : Rules)
	{
		Rule->Validate(Objects, ValidationResults);
	}
	
	UGASValidator::LogResults(ValidationResults);
	
	FValidateAssetsResults Results;
	ValidatorSubsystem->ValidateAssetsWithSettings(AssetDataList, Settings, Results);
	
	UE_LOG(LogGASValidator, Log, TEXT("GAS Validator ended"));
}

bool UGASValidator::CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset,
	FDataValidationContext& InContext) const
{
	UClass* ClassToCheck = InAsset->GetClass();

    if (const UBlueprint* Blueprint = Cast<UBlueprint>(InAsset))
    {
        ClassToCheck = Blueprint->GeneratedClass;
    }

    if (!ClassToCheck)
    {
        return false;
    }

    if (ClassToCheck->IsChildOf(UGameplayAbility::StaticClass())
        || ClassToCheck->IsChildOf(UGameplayEffect::StaticClass())
        || ClassToCheck->IsChildOf(UAttributeSet::StaticClass())
        || ClassToCheck->ImplementsInterface(UAbilitySystemInterface::StaticClass()))
    {
        return true;
    }
	
	return UGASValidator::HasGASProperties(ClassToCheck);
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
	GASObjects Objects;
	TSet<UClass*> VisitedSet;
	FindGASObjects(InAsset, Objects, VisitedSet);
	
	TArray<GASValidationResult> Results;
	for (auto Rule : Rules)
	{
		Rule->Validate(Objects, Results);
	}
	
	bool bHasError = UGASValidator::LogResults(Results, InAsset->GetName());
	
	if (bHasError)
	{
		AssetFails(InAsset, FText::FromString(TEXT("GAS validation failed - see errors above")));
		return EDataValidationResult::Invalid;
	}
		
	AssetPasses(InAsset);
	return 	EDataValidationResult::Valid;
}

FDiscoveredAbility& UGASValidator::DiscoverAbility(UClass* Class, GASObjects& GASObjects)
{	
	if (auto* DiscoveredAbility = GASObjects.Abilities.Find(Class))
	{
		return *DiscoveredAbility;
	}
	
	FDiscoveredAbility Ability;
	
	auto* CDO = Class->GetDefaultObject();
	for (TFieldIterator<FProperty> PropIt(Class); PropIt; ++PropIt)
	{
		FProperty* Property = *PropIt;
		auto* PropertyType = UGASValidator::ResolvePropertyType(Property);
		if (!PropertyType)
		{
			continue;
		}
		
		if (PropertyType->IsChildOf(UGameplayEffect::StaticClass()))
		{
			auto* ReferencedClass = UGASValidator::ResolveClassValue(Property, CDO);
			FDiscoveredEffectReference EffectReference;
			EffectReference.PropertyName = Property->GetFName();
			EffectReference.EffectClass = ReferencedClass;
			
			Ability.Effects.Add(EffectReference);
		}
		
		// TODO: GEComponents
	}
	return GASObjects.Abilities.Add(Class, Ability);
}

FDiscoveredEffect& UGASValidator::DiscoverEffect(UClass* Class, GASObjects& GASObjects)
{
	if (auto* DiscoveredEffect = GASObjects.Effects.Find(Class))
	{
		return *DiscoveredEffect;
	}
	
	FDiscoveredEffect Effect;
	auto* CDO = Class->GetDefaultObject();
	for (TFieldIterator<FProperty> PropIt(Class); PropIt; ++PropIt)
	{
		FProperty* Property = *PropIt;
		FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property);
		if (!ArrayProperty)
		{
		    continue;
		}
		
		if (FObjectProperty* InnerObject = CastField<FObjectProperty>(ArrayProperty->Inner))
		{
			if (InnerObject->PropertyClass->IsChildOf(UGameplayEffectComponent::StaticClass()))
			{
				FScriptArrayHelper Helper(ArrayProperty, ArrayProperty->ContainerPtrToValuePtr<void>(CDO));
				for (int32 i = 0; i < Helper.Num(); ++i)
				{
					uint8* ElementPtr = Helper.GetRawPtr(i);
					UObject* ComponentInstance = InnerObject->GetObjectPropertyValue(ElementPtr);
	
					if (!ComponentInstance)
					{
						continue;
					}
	
					for (TFieldIterator<FProperty> CompPropIt(ComponentInstance->GetClass()); CompPropIt; ++CompPropIt)
					{
						FProperty* CompProperty = *CompPropIt;
						UClass* CompPropertyType = UGASValidator::ResolvePropertyType(CompProperty);
	
						if (CompPropertyType && CompPropertyType->IsChildOf(UGameplayEffect::StaticClass()))
						{
							UClass* ResolvedValue = UGASValidator::ResolveClassValue(CompProperty, ComponentInstance);
							if (ResolvedValue)
							{
								FDiscoveredEffectReference Ref;
								Ref.PropertyName = CompProperty->GetFName();
								Ref.EffectClass = ResolvedValue;
								Effect.Effects.Add(Ref);
							}
							continue;
						}
	
						if (FArrayProperty* CompArrayProp = CastField<FArrayProperty>(CompProperty))
						{
							FScriptArrayHelper CompHelper(CompArrayProp, CompArrayProp->ContainerPtrToValuePtr<void>(ComponentInstance));
	
							if (FClassProperty* ArrayInnerClass = CastField<FClassProperty>(CompArrayProp->Inner))
							{
								if (ArrayInnerClass->MetaClass->IsChildOf(UGameplayEffect::StaticClass()))
								{
									for (int32 j = 0; j < CompHelper.Num(); ++j)
									{
										UClass* ElementValue = Cast<UClass>(ArrayInnerClass->GetObjectPropertyValue(CompHelper.GetRawPtr(j)));
										FDiscoveredEffectReference Ref;
										Ref.PropertyName = CompProperty->GetFName();
										Ref.EffectClass = ElementValue;
										Effect.Effects.Add(Ref);
									}
								}
							}
							else if (FStructProperty* ArrayInnerStruct = CastField<FStructProperty>(CompArrayProp->Inner))
							{
								if (ArrayInnerStruct->Struct == FConditionalGameplayEffect::StaticStruct())
								{
									for (int32 j = 0; j < CompHelper.Num(); ++j)
									{
										const FConditionalGameplayEffect* Conditional =
											reinterpret_cast<const FConditionalGameplayEffect*>(CompHelper.GetRawPtr(j));
	
										FDiscoveredEffectReference Ref;
										Ref.PropertyName = CompProperty->GetFName();
										Ref.EffectClass = Conditional->EffectClass;
										Effect.Effects.Add(Ref);
									}
								}
								else if (ArrayInnerStruct->Struct == FGameplayEffectQuery::StaticStruct())
								{
									for (int32 j = 0; j < CompHelper.Num(); ++j)
									{
										const FGameplayEffectQuery* Query =
											reinterpret_cast<const FGameplayEffectQuery*>(CompHelper.GetRawPtr(j));
	
										FDiscoveredEffectReference Ref;
										Ref.PropertyName = CompProperty->GetFName();
										Ref.EffectClass = Query->EffectDefinition;
										Effect.Effects.Add(Ref);
									}
								}
							}
						}
					}
				}
			}
		}
		 
		if (FStructProperty* InnerStruct = CastField<FStructProperty>(ArrayProperty->Inner))
		{
			FScriptArrayHelper Helper(ArrayProperty, ArrayProperty->ContainerPtrToValuePtr<void>(CDO));
			for (int32 i = 0; i < Helper.Num(); ++i)
			{
				uint8* ElementPtr = Helper.GetRawPtr(i);
				if (InnerStruct->Struct == FGameplayModifierInfo::StaticStruct())
				{
					FDiscoveredModifier DiscoveredModifier;
					for (TFieldIterator<FProperty> ModPropIt(FGameplayModifierInfo::StaticStruct()); ModPropIt; ++ModPropIt)
				    {
				        FProperty* ModifierProperty = *ModPropIt;

				        if (FStructProperty* StructProp = CastField<FStructProperty>(ModifierProperty))
				        {
				            if (StructProp->Struct == FGameplayAttribute::StaticStruct())
				            {
				                DiscoveredModifier.Attribute =
				                    *StructProp->ContainerPtrToValuePtr<FGameplayAttribute>(ElementPtr);
				            }
				            else if (StructProp->Struct == FGameplayEffectModifierMagnitude::StaticStruct())
				            {
				                const void* MagnitudePtr = StructProp->ContainerPtrToValuePtr<void>(ElementPtr);
				                UScriptStruct* MagStruct = FGameplayEffectModifierMagnitude::StaticStruct();

				                for (TFieldIterator<FProperty> MagPropIt(MagStruct); MagPropIt; ++MagPropIt)
				                {
				                    FProperty* MagProperty = *MagPropIt;

				                    if (FStructProperty* MagStructProp = CastField<FStructProperty>(MagProperty))
				                    {
				                        // CustomCalculationClass mode
				                        if (MagStructProp->Struct == FCustomCalculationBasedFloat::StaticStruct())
				                        {
				                            const void* CustomPtr = MagStructProp->ContainerPtrToValuePtr<void>(MagnitudePtr);
				                            UScriptStruct* CustomStruct = FCustomCalculationBasedFloat::StaticStruct();

				                            for (TFieldIterator<FProperty> CustomPropIt(CustomStruct); CustomPropIt; ++CustomPropIt)
				                            {
				                                if (FClassProperty* ClassProp = CastField<FClassProperty>(*CustomPropIt))
				                                {	
				                                    UClass* CalcClass = UGASValidator::ResolveClassValueFromElement(*CustomPropIt, CustomPtr);
				                                	
				                                	if(CalcClass)
				                                	{
				                                		UGASValidator::DiscoverCalculations(CalcClass, GASObjects);
				                                	}
				                                	
				                                    FDiscoveredCalculationReference Ref;
				                                    Ref.PropertyName = CustomPropIt->GetFName();
				                                    Ref.CalculationClass = CalcClass;
				                                    DiscoveredModifier.CalculationClassReference = Ref;
				                                }
				                            }
				                        }
				                        // AttributeBased mode
				                        else if (MagStructProp->Struct == FAttributeBasedFloat::StaticStruct())
				                        {
				                            const void* AttrBasedPtr = MagStructProp->ContainerPtrToValuePtr<void>(MagnitudePtr);
				                            UScriptStruct* AttrBasedStruct = FAttributeBasedFloat::StaticStruct();

				                            for (TFieldIterator<FProperty> AttrPropIt(AttrBasedStruct); AttrPropIt; ++AttrPropIt)
				                            {
				                                if (FStructProperty* CaptureDefProp = CastField<FStructProperty>(*AttrPropIt))
				                                {
				                                    if (CaptureDefProp->Struct == FGameplayEffectAttributeCaptureDefinition::StaticStruct())
				                                    {
				                                        const FGameplayEffectAttributeCaptureDefinition* CaptureDef =
				                                            CaptureDefProp->ContainerPtrToValuePtr<FGameplayEffectAttributeCaptureDefinition>(AttrBasedPtr);
				                                        DiscoveredModifier.BasedOnAttribute = CaptureDef->AttributeToCapture;
				                                    }
				                                }
				                            }
				                        }
				                        // SetByCaller mode
				                        else if (MagStructProp->Struct == FSetByCallerFloat::StaticStruct())
				                        {
				                            const void* SetByCallerPtr = MagStructProp->ContainerPtrToValuePtr<void>(MagnitudePtr);
				                            UScriptStruct* SetByCallerStruct = FSetByCallerFloat::StaticStruct();

				                            for (TFieldIterator<FProperty> SBCPropIt(SetByCallerStruct); SBCPropIt; ++SBCPropIt)
				                            {
				                                FProperty* SBCProperty = *SBCPropIt;

				                                if (FStructProperty* TagProp = CastField<FStructProperty>(SBCProperty))
				                                {
				                                    if (TagProp->Struct == FGameplayTag::StaticStruct())
				                                    {
				                                        DiscoveredModifier.CallableTag =
				                                            *TagProp->ContainerPtrToValuePtr<FGameplayTag>(SetByCallerPtr);
				                                    }
				                                }
				                                else if (FNameProperty* NameProp = CastField<FNameProperty>(SBCProperty))
				                                {
				                                    DiscoveredModifier.CallableName =
				                                        *NameProp->ContainerPtrToValuePtr<FName>(SetByCallerPtr);
				                                }
				                            }
				                        }
				                    	// ScalableFloat mode
				                        else if (MagStructProp->Struct == FScalableFloat::StaticStruct())
				                        {
				                        	const void* ScalablePtr = MagStructProp->ContainerPtrToValuePtr<void>(MagnitudePtr);
				                        	UScriptStruct* ScalableStruct = FScalableFloat::StaticStruct();

				                        	for (TFieldIterator<FProperty> ScalablePropIt(ScalableStruct); ScalablePropIt; ++ScalablePropIt)
				                        	{
				                        		FProperty* ScalableProperty = *ScalablePropIt;

				                        		if (FFloatProperty* FloatProp = CastField<FFloatProperty>(ScalableProperty))
				                        		{
				                        			DiscoveredModifier.FloatValue = *FloatProp->ContainerPtrToValuePtr<float>(ScalablePtr);
				                        		}
				                        	}
										}
				                    }
				                }
				            }
				        }
				    }
				
					Effect.Modifiers.Add(DiscoveredModifier);
				}
				else if (InnerStruct->Struct == FGameplayEffectExecutionDefinition::StaticStruct())
				{
					FDiscoveredCalculationReference Execution;
					for (TFieldIterator<FProperty> ExePropIt(FGameplayEffectExecutionDefinition::StaticStruct()); ExePropIt; ++ExePropIt)
					{
						FProperty* ExecuteProperty = *ExePropIt;	
						
						if (FClassProperty* ClassProperty = CastField<FClassProperty>(ExecuteProperty))
						{
							auto* PropertyType = ResolvePropertyType(ClassProperty);
							if (PropertyType->IsChildOf(UGameplayEffectCalculation::StaticClass()))
							{
								auto* CalculationClass = ResolveClassValueFromElement(ClassProperty, ElementPtr);
								
								if (CalculationClass)
								{
									UGASValidator::DiscoverCalculations(CalculationClass, GASObjects);
								}
								
								Execution.PropertyName = ExecuteProperty->GetFName();
								Execution.CalculationClass = CalculationClass;
							}
							else if (FArrayProperty* CompArrayProp = CastField<FArrayProperty>(ExecuteProperty))
							{
								FScriptArrayHelper CompHelper(CompArrayProp, ResolveClassValueFromElement(CompArrayProp, ElementPtr));
        
								if (FClassProperty* ArrayInnerClass = CastField<FClassProperty>(CompArrayProp->Inner))
								{
									if (ArrayInnerClass->MetaClass->IsChildOf(UGameplayEffect::StaticClass()))
									{
										for (int32 j = 0; j < CompHelper.Num(); ++j)
										{
											UClass* ElementValue = Cast<UClass>(ArrayInnerClass->GetObjectPropertyValue(CompHelper.GetRawPtr(j)));
											FDiscoveredEffectReference Ref;
											Ref.PropertyName = ExecuteProperty->GetFName();
											Ref.EffectClass = ElementValue;
											Effect.Effects.Add(Ref);
										}
									}
								}
								else if (FStructProperty* ArrayInnerStruct = CastField<FStructProperty>(CompArrayProp->Inner))
								{
									if (ArrayInnerStruct->Struct == FConditionalGameplayEffect::StaticStruct())
									{
										for (int32 j = 0; j < CompHelper.Num(); ++j)
										{
											const FConditionalGameplayEffect* Conditional =
												reinterpret_cast<const FConditionalGameplayEffect*>(CompHelper.GetRawPtr(j));
        
											FDiscoveredEffectReference Ref;
											Ref.PropertyName = ExecuteProperty->GetFName();
											Ref.EffectClass = Conditional->EffectClass;
											Effect.Effects.Add(Ref);
										}
									}
								}
							}
						}
					}
					
					Effect.Executions.Add(Execution);
				}
			}
		}
	}
	
	return GASObjects.Effects.Add(Class, Effect);
}

TArray<FDiscoveredAttribute>& UGASValidator::DiscoverAttributes(UClass* Class, GASObjects& GASObjects)
{
	if (auto* DiscoveredAttribute = GASObjects.Attributes.Find(Class))
	{
		return *DiscoveredAttribute;
	}
	
	TArray<FDiscoveredAttribute> Attributes;
	
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
		Attributes.Add(DiscoveredAttribute);
	}
	
	return GASObjects.Attributes.Add(Class, Attributes);
}

TArray<FDiscoveredTagContainer>& UGASValidator::DiscoverTags(UClass* Class, const void* Instance, GASObjects& GASObjects)
{
	if (auto* DiscoveredTag = GASObjects.TagContainers.Find(Class))
	{
		return *DiscoveredTag;
	}

	TArray<FDiscoveredTagContainer> TagContainers;
	for (TFieldIterator<FStructProperty> PropIt(Class); PropIt; ++PropIt)
	{
		FStructProperty* StructProp = *PropIt;
		if (StructProp->Struct != FGameplayTagContainer::StaticStruct())
		{
			continue;
		}
		
		FDiscoveredTagContainer TagContainer;
		TagContainer.Container = *StructProp->ContainerPtrToValuePtr<FGameplayTagContainer>(Instance);
		TagContainer.PropertyName = StructProp->GetFName();		
		TagContainers.Add(TagContainer);
	}
	
	GASObjects.TagContainers.FindOrAdd(Class).Append(TagContainers);
	return *GASObjects.TagContainers.Find(Class);
}

FDiscoveredCalculation& UGASValidator::DiscoverCalculations(UClass* Class, GASObjects& GASObjects)
{
	if (auto* DiscoveredCalculation = GASObjects.Calculations.Find(Class))
	{
		return *DiscoveredCalculation;
	}

	FDiscoveredCalculation Calculation;
	auto* CDO = Class->GetDefaultObject();
	for (TFieldIterator<FArrayProperty> PropIt(Class); PropIt; ++PropIt)
	{
		FStructProperty* Struct = CastField<FStructProperty>(PropIt->Inner);
		
		if (!Struct || Struct->Struct != FGameplayEffectAttributeCaptureDefinition::StaticStruct())
		{
			continue;
		}
		
		FScriptArrayHelper Helper(*PropIt, PropIt->ContainerPtrToValuePtr<void>(CDO));

		for (int i = 0; i < Helper.Num(); ++i)
		{
			auto* Instance = Helper.GetRawPtr(i);
			for (TFieldIterator<FStructProperty> StructPropIt(FGameplayEffectAttributeCaptureDefinition::StaticStruct()); StructPropIt; ++StructPropIt)
			{
				FStructProperty* StructProp = *StructPropIt;
				if (StructPropIt->Struct != FGameplayAttribute::StaticStruct())
				{
					continue;
				}
				
				Calculation.CapturedAttributes.Add(*StructProp->ContainerPtrToValuePtr<FGameplayAttribute>(Instance));
			}
		}
	}
	
	GASObjects.Calculations.Add(Class, Calculation);
	return *GASObjects.Calculations.Find(Class);
}

void UGASValidator::FindGASObjects(UObject* Object, GASObjects& GASObjects, TSet<UClass*>& VisitedClasses)
{
	auto Class = Object->GetClass();
	if (VisitedClasses.Contains(Class))
	{
		return;
	}
	
	VisitedClasses.Add(Class);
	
	if (Object->IsA(UGameplayAbility::StaticClass()))
	{
		DiscoverAbility(Class, GASObjects);
	}
	else if (Object->IsA(UGameplayEffect::StaticClass()))
	{
		DiscoverEffect(Class, GASObjects);
	}
	else if (Object->IsA(UAttributeSet::StaticClass()))
	{
		DiscoverAttributes(Class, GASObjects);
	}
	else if (Object->IsA(UGameplayEffectComponent::StaticClass()))
    {
        FindGASObjects(Object, GASObjects, VisitedClasses);
    }
	
	for (TFieldIterator<FProperty> PropIt(Class); PropIt; ++PropIt)
	{
		FProperty* Property = *PropIt;
		UClass* PropertyType = UGASValidator::ResolvePropertyType(Property);
		if (!PropertyType)
		{
			if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
			{
				if (StructProp->Struct == FGameplayTagContainer::StaticStruct())
				{
					// discover tags
					DiscoverTags(Class, Object, GASObjects);
				}
				else
				{
					FindGASObjectsInStruct(Class, StructProp->ContainerPtrToValuePtr<void>(Object),StructProp->Struct,GASObjects, VisitedClasses);
				}
			}
			else if (FArrayProperty* ArrayProp = CastField<FArrayProperty>(Property))
			{
				UGASValidator::RecurseArray(Class, ArrayProp, Object, GASObjects, VisitedClasses);
			}
			continue;
		}
		
		UClass* Value = UGASValidator::ResolveClassValue(Property, Object);
		
		if (!Value || !IsValid(Value) || VisitedClasses.Contains(Value))
		{
			continue;
		}
		
		// any class
		if (auto* CDO = Value->GetDefaultObject())
		{
			FindGASObjects(CDO, GASObjects, VisitedClasses);
		}
	}
	
	if (Class->ImplementsInterface(UAbilitySystemInterface::StaticClass()))
	{
		const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Object);
		if (!ASI)
		{
			return; // ImplementsInterface said yes, but Cast failed — shouldn't normally happen, defensive only
		}

		UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent(); // the guarded virtual call

		if (!ASC)
		{
			return;
		}

		for (const FAttributeDefaults& Entry : ASC->DefaultStartingData)
		{
			
			if (!Entry.Attributes)
			{
				continue;
			}

			if (!VisitedClasses.Contains(Entry.Attributes))
			{
				DiscoverAttributes(Entry.Attributes, GASObjects);
			}

			if (!Entry.DefaultStartingTable)
		    {
		        continue;
		    }
			
			TArray<FDiscoveredAttribute>& Attributes = GASObjects.Attributes[Entry.Attributes];

			for (auto& Attribute : Attributes)
			{
				Attribute.SourceOfValue = Entry.DefaultStartingTable->GetFName();
				
				const FString AttributeName = Attribute.Attribute.AttributeName;
				const FString RowNameString = FString::Printf(TEXT("%s.%s"), *Entry.Attributes->GetName(), *AttributeName);
				const FName RowName(*RowNameString);
				const FAttributeMetaData* Row = Entry.DefaultStartingTable->FindRow<FAttributeMetaData>(RowName, TEXT("GASValidator"));
                                   						
				if (Row)
				{
					float BaseValue = Row->BaseValue;
					Attribute.Value = BaseValue;
				}
			}
		}
		// Check attributes if exists, if not add anyway add the data to the Discovered attribute
	}
}

void UGASValidator::FindGASObjectsInStruct(UClass* Class, const void* StructInstance, UScriptStruct* StructType,
	GASObjects& GASObjects, TSet<UClass*>& VisitedClasses)
{
    if (!StructInstance || !StructType)
    {
        return;
    }

    for (TFieldIterator<FProperty> PropIt(StructType); PropIt; ++PropIt)
    {
        FProperty* Property = *PropIt;
        UClass* PropertyType = ResolvePropertyType(Property);

        if (!PropertyType)
        {
            if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
            {
                if (StructProp->Struct == FGameplayTagContainer::StaticStruct())
                {
                    DiscoverTags(Class, StructInstance, GASObjects);
                }
                else
                {
                    FindGASObjectsInStruct(
                    	Class,
                        StructProp->ContainerPtrToValuePtr<void>(StructInstance),
                        StructProp->Struct,
                        GASObjects,
                        VisitedClasses);
                }
            }
            else if (FArrayProperty* ArrayProp = CastField<FArrayProperty>(Property))
            {
            	UGASValidator::RecurseArray(Class, ArrayProp, StructInstance, GASObjects, VisitedClasses);
            }
            continue;
        }

    	const void* ValuePtr = Property->ContainerPtrToValuePtr<void>(StructInstance);
        UClass* Value = ResolveClassValueFromElement(Property, ValuePtr);
        if (!Value || !IsValid(Value) || VisitedClasses.Contains(Value))
        {
            continue;
        }

    	if (auto* CDO = Value->GetDefaultObject())
    	{
    		FindGASObjects(CDO, GASObjects, VisitedClasses);
    	}
    }
}

bool UGASValidator::HasGASProperties(UClass* Class)
{
	for (TFieldIterator<FProperty> PropIt(Class); PropIt; ++PropIt)
	{
		FProperty* Property = *PropIt;
		UClass* PropertyType = UGASValidator::ResolvePropertyType(Property);
		if (!PropertyType)
		{
			if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
			{
				if (StructProp->Struct == FGameplayTagContainer::StaticStruct())
				{
					return true;
				}
			}
			else if (FArrayProperty* ArrayProp = CastField<FArrayProperty>(Property))
			{
				// Has GAS Property in array
				UClass* InnerType = ResolvePropertyType(ArrayProp->Inner);
				if (InnerType && (InnerType->IsChildOf(UAttributeSet::StaticClass())
					|| InnerType->IsChildOf(UGameplayAbility::StaticClass())
					|| InnerType->IsChildOf(UGameplayEffect::StaticClass())
					|| InnerType->IsChildOf(UAbilitySystemComponent::StaticClass())))
				{
					return true;
				}
			}
			continue;
		}
		
		if (PropertyType->IsChildOf(UAttributeSet::StaticClass()) 
			|| PropertyType->IsChildOf(UGameplayAbility::StaticClass()) 
			|| PropertyType->IsChildOf(UGameplayEffect::StaticClass()))
		{
			return true;
		}
	}
	
	return false;
}

UClass* UGASValidator::ResolvePropertyType(FProperty* Property)
{
	if (FClassProperty* ClassProperty = CastField<FClassProperty>(Property))
	{
		return ClassProperty->MetaClass;
	}
	
	if (FObjectProperty* ObjectProperty = CastField<FObjectProperty>(Property))
	{
		return ObjectProperty->PropertyClass;
	}
	
	// TODO: SoftClass
	
	return nullptr;
}

UClass* UGASValidator::ResolveClassValue(FProperty* Property, UObject* Instance)
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

UClass* UGASValidator::ResolveClassValueFromElement(FProperty* Property, const void* ElementPtr)
{
	if (FClassProperty* ClassProperty = CastField<FClassProperty>(Property))
	{
		return Cast<UClass>(ClassProperty->GetObjectPropertyValue(ElementPtr));
	}
	if (FObjectProperty* ObjectProperty = CastField<FObjectProperty>(Property))
	{
		UObject* Value = ObjectProperty->GetObjectPropertyValue(ElementPtr);
		return Value ? Value->GetClass() : nullptr;
	}

	return nullptr;
}

void UGASValidator::RecurseArray(UClass* Class, FArrayProperty* Prop, const void* Instance, GASObjects& GASObjects, TSet<UClass*>& VisitedClasses)
{
	FScriptArrayHelper Helper(Prop, Prop->ContainerPtrToValuePtr<void>(Instance));

	if (FStructProperty* InnerStruct = CastField<FStructProperty>(Prop->Inner))
	{
		for (int32 i = 0; i < Helper.Num(); ++i)
		{
			FindGASObjectsInStruct(Class, Helper.GetRawPtr(i), InnerStruct->Struct, GASObjects, VisitedClasses);
		}
	}
	else if (ResolvePropertyType(Prop->Inner))
	{
		for (int32 i = 0; i < Helper.Num(); ++i)
		{
			UClass* Value = ResolveClassValueFromElement(Prop->Inner, Helper.GetRawPtr(i));
			if (Value && !VisitedClasses.Contains(Value))
			{
				FindGASObjects(Value->GetDefaultObject(), GASObjects, VisitedClasses);
			}
		}
	}
}

bool UGASValidator::LogResults(TArray<GASValidationResult>& Results, FString AssetName)
{
	bool bHasError = false;
	
	for (auto Result : Results)
	{
		UE_LOG(LogGASValidator, Log, TEXT("%s: %s"), *AssetName, *Result.Message)
		
		if (Result.Severity == EGASValidationSeverity::ERROR)
		{
			bHasError = true;
		}
	}
	
	return bHasError;
}