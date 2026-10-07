#pragma once

struct GASObjects;
struct FDiscoveredAbility;
struct FDiscoveredEffect;
struct FDiscoveredAttribute;
struct FDiscoveredTagContainer;
struct FDiscoveredCalculation;

namespace GASDiscovery
{
	FDiscoveredAbility& DiscoverAbility(UClass* Class, GASObjects& GASObjects);
	FDiscoveredEffect& DiscoverEffect(UClass* Class, GASObjects& GASObjects);
	TArray<FDiscoveredAttribute>& DiscoverAttributes(UClass* Class, GASObjects& GASObjects);
	TArray<FDiscoveredTagContainer>& DiscoverTags(UClass* Class, const void* Instance, GASObjects& GASObjects);
	FDiscoveredCalculation& DiscoverCalculations(UClass* Class, GASObjects& GASObjects);
}
