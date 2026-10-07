#include "Discovery/GASDiscovery.h"
#include "DataStructures/DiscoveryData.h"

TArray<FDiscoveredAttribute>& GASDiscovery::DiscoverAttributes(UClass* Class, GASObjects& GASObjects)
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
