#include "Discovery/GASDiscovery.h"
#include "DataStructures/DiscoveryData.h"

TArray<FDiscoveredTagContainer>& GASDiscovery::DiscoverTags(UClass* Class, const void* Instance, GASObjects& GASObjects)
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
