#include "GameplayEffect.h"
#include "GameplayEffectCalculation.h"
#include "GameplayEffectComponent.h"
#include "Discovery/GASDiscovery.h"
#include "DataStructures/DiscoveryData.h"
#include "GASDiscoveryInternal.h"

namespace {

	FDiscoveredEffectReference GetEffectReferenceFromGEComponents(const FProperty* Property, const void* ElementPtr)
	{
		FDiscoveredEffectReference Ref;
		if (const FClassProperty* ClassProp = CastField<FClassProperty>(Property))
		{
			if (ClassProp->MetaClass->IsChildOf(UGameplayEffect::StaticClass()))
			{
				UClass* ElementValue = Cast<UClass>(ClassProp->GetObjectPropertyValue(ElementPtr));
				Ref = GASDiscovery::Private::Effect::CreateEffectReference(Property, ElementValue);
			}
		}
		else if (const FStructProperty* StructProp = CastField<FStructProperty>(Property))
		{
			if (StructProp->Struct == FConditionalGameplayEffect::StaticStruct())
			{
				const auto* Conditional = static_cast<const FConditionalGameplayEffect*>(ElementPtr);
				Ref = GASDiscovery::Private::Effect::CreateEffectReference(Property, Conditional->EffectClass);
			}
			else if (StructProp->Struct == FGameplayEffectQuery::StaticStruct())
			{
				const auto* Query = static_cast<const FGameplayEffectQuery*>(ElementPtr);
				Ref = GASDiscovery::Private::Effect::CreateEffectReference(Property, Query->EffectDefinition);													
			}
		}
		
		return Ref;
	}

	FDiscoveredCalculationReference GetCustomCalculationClassFromModifier(const FStructProperty* Property, const void* Instance)
	{
		const void* CustomPtr = Property->ContainerPtrToValuePtr<void>(Instance);

		FDiscoveredCalculationReference Ref;
		for (TFieldIterator<FProperty> CustomPropIt(Property->Struct); CustomPropIt; ++CustomPropIt)
		{
			FClassProperty* ClassProp = CastField<FClassProperty>(*CustomPropIt);
			if (!ClassProp)
			{	
				continue;
			}
			
			Ref = GASDiscovery::Private::Effect::CreateCalculationReference(*CustomPropIt, CustomPtr);
			break;
		}

		return Ref;
	}

	FGameplayAttribute GetAttributeBasedModifier(const FStructProperty* Property, const void* Instance)
	{
		FGameplayAttribute Attribute;
		const void* AttrBasedPtr = Property->ContainerPtrToValuePtr<void>(Instance);

		for (TFieldIterator<FProperty> AttrPropIt(Property->Struct); AttrPropIt; ++AttrPropIt)
		{
			if (FStructProperty* CaptureDefProp = CastField<FStructProperty>(*AttrPropIt))
			{
				if (CaptureDefProp->Struct == FGameplayEffectAttributeCaptureDefinition::StaticStruct())
				{
					const FGameplayEffectAttributeCaptureDefinition* CaptureDef =
						CaptureDefProp->ContainerPtrToValuePtr<FGameplayEffectAttributeCaptureDefinition>(AttrBasedPtr);
					Attribute = CaptureDef->AttributeToCapture;
					break;
				}
			}
		}
		
		return Attribute;
	}
	
	float GetScalableFloatModifier(const FStructProperty* Property, const void* Instance)
	{
		float FloatValue = 0.0f;
		
		const void* ScalablePtr = Property->ContainerPtrToValuePtr<void>(Instance);
		for (TFieldIterator<FProperty> ScalablePropIt(Property->Struct); ScalablePropIt; ++ScalablePropIt)
		{
			FProperty* ScalableProperty = *ScalablePropIt;

			if (FFloatProperty* FloatProp = CastField<FFloatProperty>(ScalableProperty))
			{
				FloatValue = *FloatProp->ContainerPtrToValuePtr<float>(ScalablePtr);
				break;
			}
		}
		
		return FloatValue;
	}
	
	TTuple<FName, FGameplayTag> GetSetByCallerModifier(const FStructProperty* Property, const void* Instance)
	{
		FName CallableName;
		FGameplayTag CallableTag;
		
		const void* SetByCallerPtr = Property->ContainerPtrToValuePtr<void>(Instance);

		for (TFieldIterator<FProperty> SBCPropIt(Property->Struct); SBCPropIt; ++SBCPropIt)
		{
			FProperty* SBCProperty = *SBCPropIt;

			if (FStructProperty* TagProp = CastField<FStructProperty>(SBCProperty))
			{
				if (TagProp->Struct == FGameplayTag::StaticStruct())
				{
					CallableTag =
						*TagProp->ContainerPtrToValuePtr<FGameplayTag>(SetByCallerPtr);
				}
			}
			else if (FNameProperty* NameProp = CastField<FNameProperty>(SBCProperty))
			{
				CallableName =
					*NameProp->ContainerPtrToValuePtr<FName>(SetByCallerPtr);
			}
		}
		
		return MakeTuple(CallableName, CallableTag);
	}
	
	FDiscoveredModifier DiscoverModifier(const UStruct* Struct, const void* StructInstance, FGASObjects& GASObjects)
	{
		FDiscoveredModifier DiscoveredModifier;
		for (TFieldIterator<FProperty> PropIt(Struct); PropIt; ++PropIt)
		{
			FProperty* Property = *PropIt;
			
			FStructProperty* StructProperty = CastField<FStructProperty>(Property);
			if (!StructProperty)
			{
				continue;
			}
			
			// Get Attribute
			if (StructProperty->Struct == FGameplayAttribute::StaticStruct())
			{
				DiscoveredModifier.Attribute =
					*StructProperty->ContainerPtrToValuePtr<FGameplayAttribute>(StructInstance);
			}
			
			// Get Modifier Magnitude
			else if (StructProperty->Struct == FGameplayEffectModifierMagnitude::StaticStruct())
			{
				const void* MagnitudePtr = StructProperty->ContainerPtrToValuePtr<void>(StructInstance);

                for (TFieldIterator<FProperty> MagPropIt(FGameplayEffectModifierMagnitude::StaticStruct()); MagPropIt; ++MagPropIt)
                {
                    FProperty* MagProperty = *MagPropIt;

                    if (FStructProperty* MagStructProp = CastField<FStructProperty>(MagProperty))
                    {
                        // CustomCalculationClass mode
                        if (MagStructProp->Struct == FCustomCalculationBasedFloat::StaticStruct())
                        {
                            DiscoveredModifier.CalculationClassReference = GetCustomCalculationClassFromModifier(MagStructProp, MagnitudePtr);
	                        
	                        if (!DiscoveredModifier.CalculationClassReference.CalculationClass)
	                        {
		                        GASDiscovery::DiscoverCalculations(DiscoveredModifier.CalculationClassReference.CalculationClass, GASObjects);
	                        }
                        }
                        // AttributeBased mode
                        else if (MagStructProp->Struct == FAttributeBasedFloat::StaticStruct())
                        {				                        	
	                        DiscoveredModifier.BasedOnAttribute = GetAttributeBasedModifier(MagStructProp, MagnitudePtr);
                        }
                        // SetByCaller mode
                        else if (MagStructProp->Struct == FSetByCallerFloat::StaticStruct())
                        {
                            auto [Name, Tag] = GetSetByCallerModifier(MagStructProp, MagnitudePtr);
	                        DiscoveredModifier.CallableName = Name;
	                        DiscoveredModifier.CallableTag = Tag;
                        }
	                    // ScalableFloat mode
                        else if (MagStructProp->Struct == FScalableFloat::StaticStruct())
                        {
							DiscoveredModifier.FloatValue = GetScalableFloatModifier(MagStructProp, MagnitudePtr);
						}
                    }
                }								
			}
		}
		
		return DiscoveredModifier;
	}
	
	TArray<FDiscoveredEffectReference>& DiscoverGEComponents(const FObjectProperty* ObjectProperty, const void* ElementPtr, FGASObjects& GASObjects)
	{
		TArray<FDiscoveredEffectReference> EffectReferences;
		
		// Discover GameplayEffectComponent
		UObject* ComponentInstance = GASDiscovery::Private::ResolveClassValueFromElement(ObjectProperty, ElementPtr);
		if (!ComponentInstance)
		{
			return EffectReferences;
		}
						
		// Find UGameplayEffect classes on properties of a GEComponent	
		for (TFieldIterator<FProperty> PropIt(ComponentInstance->GetClass()); PropIt; ++PropIt)
		{
			FProperty* Property = *PropIt;
			UClass* PropertyType = GASDiscovery::Private::ResolvePropertyType(Property);
			if (PropertyType && PropertyType->IsChildOf(UGameplayEffect::StaticClass()))
			{
				FDiscoveredEffectReference Ref = GASDiscovery::Private::Effect::CreateEffectReference(Property, GASDiscovery::Private::ResolveClassValue(Property, ComponentInstance));
				EffectReferences.Add(Ref);
			}
			else if(FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
			{
				GASDiscovery::Private::IterateArray(ArrayProperty, ArrayProperty->ContainerPtrToValuePtr<void>(ComponentInstance), 
				                                    [&EffectReferences, &Property](const FProperty* Leaf, const void* ElementPtr)
				                                    {
					                                    auto Ref = GetEffectReferenceFromGEComponents(Leaf, ElementPtr);
					                                    Ref.PropertyName = Property->GetFName();
					                                    EffectReferences.Add(Ref);
				                                    });
			}
		}
		
		return EffectReferences;
	}
	
	TArray<FDiscoveredCalculationReference>& DiscoverExecution(FClassProperty* Property, const void* ElementPtr, FGASObjects& GASObjects)
	{
		TArray<FDiscoveredCalculationReference> Executions;
		auto* PropertyType = GASDiscovery::Private::ResolvePropertyType(Property);
        if (PropertyType->IsChildOf(UGameplayEffectCalculation::StaticClass()))
        {
        	FDiscoveredCalculationReference Execution = GASDiscovery::Private::Effect::CreateCalculationReference(Property, ElementPtr);
        	Execution.PropertyName = Property->GetFName();						
        	if (Execution.CalculationClass)
        	{
        		GASDiscovery::DiscoverCalculations(Execution.CalculationClass, GASObjects);
        	}
        	Executions.Add(Execution);							
        }
		
		return Executions;
	}
}

FDiscoveredEffect& GASDiscovery::DiscoverEffect(UClass* Class, FGASObjects& GASObjects)
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
		
		Private::IterateArray(ArrayProperty, ArrayProperty->ContainerPtrToValuePtr<void>(CDO), 
	[&Effect, &GASObjects](const FProperty* Leaf, const void* ElementPtr)
				{
					// GEComponents 
					const FObjectProperty* InnerObject = CastField<FObjectProperty>(Leaf);
					if (InnerObject && InnerObject->PropertyClass->IsChildOf(UGameplayEffectComponent::StaticClass()))
					{
						auto EffectReferences = DiscoverGEComponents(InnerObject, ElementPtr, GASObjects);
						Effect.Effects.Append(EffectReferences);
						return;
					}
		
					const FStructProperty* InnerStruct = CastField<FStructProperty>(Leaf);
		
					if (!InnerStruct) return;
					
					// Modifier
					if (InnerStruct->Struct == FGameplayModifierInfo::StaticStruct())
					{
						FDiscoveredModifier DiscoveredModifier = DiscoverModifier(InnerStruct->Struct, ElementPtr, GASObjects);
						Effect.Modifiers.Add(DiscoveredModifier);
						
					}
					// Execution
					else if (InnerStruct->Struct == FGameplayEffectExecutionDefinition::StaticStruct())
					{
						for (TFieldIterator<FProperty> PropIt(FGameplayEffectExecutionDefinition::StaticStruct()); PropIt; ++PropIt)
						{
							FProperty* Property = *PropIt;
							FClassProperty* ClassProperty = CastField<FClassProperty>(Property);
							
							if (FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
							{
								Private::IterateArray(ArrayProperty, ArrayProperty->ContainerPtrToValuePtr<void>(ElementPtr), 
												[&Effect, &Property](const FProperty* Leaf, const void* ElementPtr)
														{
																auto Ref = GetEffectReferenceFromGEComponents(Leaf, ElementPtr);
																Ref.PropertyName = Property->GetFName();
																Effect.Effects.Add(Ref);
														});
							}
							else if (!ClassProperty)
							{
								continue;
							}
							
							auto& Executions = DiscoverExecution(ClassProperty, ElementPtr, GASObjects);
							Effect.Executions.Append(Executions);							
						}						
					}
				});
	}
	
	return GASObjects.Effects.Add(Class, Effect);
}