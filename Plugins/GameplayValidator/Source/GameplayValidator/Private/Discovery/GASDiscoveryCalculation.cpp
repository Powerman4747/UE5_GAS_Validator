#include "Discovery/GASDiscovery.h"
#include "GameplayEffectAttributeCaptureDefinition.h"
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
		const FStructProperty* Struct = CastField<FStructProperty>(PropIt->Inner);
		
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