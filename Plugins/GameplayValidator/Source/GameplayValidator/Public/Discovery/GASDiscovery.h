#pragma once

struct FGASObjects;
struct FDiscoveredAbility;
struct FDiscoveredEffect;
struct FDiscoveredAttribute;
struct FDiscoveredTagContainer;
struct FDiscoveredCalculation;

namespace GASDiscovery
{
	FDiscoveredAbility& DiscoverAbility(UClass* Class, FGASObjects& GASObjects);
	FDiscoveredEffect& DiscoverEffect(UClass* Class, FGASObjects& GASObjects);
	TArray<FDiscoveredAttribute>& DiscoverAttributes(UClass* Class, FGASObjects& GASObjects);
	TArray<FDiscoveredTagContainer>& DiscoverTags(UClass* Class, const void* Instance, FGASObjects& GASObjects);
	FDiscoveredCalculation& DiscoverCalculations(UClass* Class, FGASObjects& GASObjects);
}
