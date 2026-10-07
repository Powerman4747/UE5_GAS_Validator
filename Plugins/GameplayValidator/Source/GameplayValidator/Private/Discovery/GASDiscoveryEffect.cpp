#include "GameplayEffect.h"
#include "GameplayEffectCalculation.h"
#include "GameplayEffectComponent.h"
#include "Discovery/GASDiscovery.h"
#include "DataStructures/DiscoveryData.h"
#include "GASDiscoveryInternal.h"

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
						UClass* CompPropertyType = Private::ResolvePropertyType(CompProperty);
	
						if (CompPropertyType && CompPropertyType->IsChildOf(UGameplayEffect::StaticClass()))
						{
							if (UClass* ResolvedValue = Private::ResolveClassValue(CompProperty, ComponentInstance))
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
				                                    UClass* CalcClass = Private::ResolveClassValueFromElement(*CustomPropIt, CustomPtr);
				                                	
				                                	if(CalcClass)
				                                	{
				                                		GASDiscovery::DiscoverCalculations(CalcClass, GASObjects);
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
							auto* PropertyType = Private::ResolvePropertyType(ClassProperty);
							if (PropertyType->IsChildOf(UGameplayEffectCalculation::StaticClass()))
							{
								auto* CalculationClass = Private::ResolveClassValueFromElement(ClassProperty, ElementPtr);
								
								if (CalculationClass)
								{
									GASDiscovery::DiscoverCalculations(CalculationClass, GASObjects);
								}
								
								Execution.PropertyName = ExecuteProperty->GetFName();
								Execution.CalculationClass = CalculationClass;
							}
							else if (FArrayProperty* CompArrayProp = CastField<FArrayProperty>(ExecuteProperty))
							{
								FScriptArrayHelper CompHelper(CompArrayProp, Private::ResolveClassValueFromElement(CompArrayProp, ElementPtr));
        
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
