#include "Discovery/GASDiscovery.h"
#include "GameplayEffect.h"
#include "GASDiscoveryInternal.h"
#include "DataStructures/DiscoveryData.h"

FDiscoveredAbility& GASDiscovery::DiscoverAbility(UClass* Class, GASObjects& GASObjects)
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
		auto* PropertyType = Private::ResolvePropertyType(Property);
		if (!PropertyType)
		{
			continue;
		}
		
		if (PropertyType->IsChildOf(UGameplayEffect::StaticClass()))
		{
			auto* ReferencedClass = Private::ResolveClassValue(Property, CDO);
			FDiscoveredEffectReference EffectReference;
			EffectReference.PropertyName = Property->GetFName();
			EffectReference.EffectClass = ReferencedClass;
			
			Ability.Effects.Add(EffectReference);
		}
	}
	return GASObjects.Abilities.Add(Class, Ability);
}
