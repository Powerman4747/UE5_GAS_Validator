#include "Discovery/GASDiscovery.h"
#include "DataStructures/DiscoveryData.h"

TArray<FDiscoveredTagContainer>& GASDiscovery::DiscoverTags(UClass* Class, const FStructProperty* Property, const void* Instance, FGASObjects& GASObjects)
{
	auto& TagContainers = GASObjects.TagContainers.FindOrAdd(Class);
	
	if (Property->Struct != FGameplayTagContainer::StaticStruct())
	{
		return TagContainers;
	}
		
	FDiscoveredTagContainer TagContainer;
	TagContainer.Container = *static_cast<const FGameplayTagContainer*>(Instance);
	TagContainer.PropertyName = Property->GetFName();		
	
	TagContainers.Add(TagContainer);
	
	return TagContainers;
}
