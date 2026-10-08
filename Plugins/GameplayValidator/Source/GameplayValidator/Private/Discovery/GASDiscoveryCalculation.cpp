#include "Discovery/GASDiscovery.h"
#include "GameplayEffectAttributeCaptureDefinition.h"
#include "GASDiscoveryInternal.h"
#include "DataStructures/DiscoveryData.h"

FDiscoveredCalculation& GASDiscovery::DiscoverCalculations(UClass* Class, FGASObjects& GASObjects)
{
	if (auto* DiscoveredCalculation = GASObjects.Calculations.Find(Class))
	{
		return *DiscoveredCalculation;
	}

	FDiscoveredCalculation Calculation;
	auto* CDO = Class->GetDefaultObject();
	for (TFieldIterator<FArrayProperty> PropIt(Class); PropIt; ++PropIt)
	{
		Private::IterateArray(*PropIt, PropIt->ContainerPtrToValuePtr<void>(CDO), 
			[&Calculation](const FProperty* Property, const void* ElementPtr)
						{
							const FStructProperty* Struct = CastField<FStructProperty>(Property);
							if (!Struct)
							{
								return;
							}
				
							for (TFieldIterator<FStructProperty> StructPropIt(Struct->Struct); StructPropIt; ++StructPropIt)
							{
								FStructProperty* StructProp = *StructPropIt;
								if (StructPropIt->Struct != FGameplayAttribute::StaticStruct())
								{
									continue;
								}
							
								Calculation.CapturedAttributes.Add(*StructProp->ContainerPtrToValuePtr<FGameplayAttribute>(ElementPtr));
							}
						});
	}
	
	GASObjects.Calculations.Add(Class, Calculation);
	return *GASObjects.Calculations.Find(Class);
}