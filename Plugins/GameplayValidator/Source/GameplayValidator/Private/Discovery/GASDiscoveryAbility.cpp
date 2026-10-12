#include "Discovery/GASDiscovery.h"
#include "GameplayEffect.h"
#include "GASDiscoveryInternal.h"
#include "K2Node_CallFunction.h"
#include "K2Node_Event.h"
#include "DataStructures/DiscoveryData.h"

FDiscoveredAbility& GASDiscovery::DiscoverAbility(UClass* Class, FGASObjects& GASObjects)
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
		const auto* PropertyType = Private::ResolvePropertyType(Property);
		if (!PropertyType)
		{
			continue;
		}
		
		if (PropertyType->IsChildOf(UGameplayEffect::StaticClass()))
		{
			auto* ReferencedClass = Private::ResolveClassValue(Property, CDO);
			FDiscoveredEffectReference EffectReference = GASDiscovery::Private::Effect::CreateEffectReference(Property, ReferencedClass);			
			Ability.Effects.Add(EffectReference);
		}
	}
	
	Ability.bHasActivateAbilityEvent = true;
	Ability.bActivateAbilityHasLogic = GASDiscovery::Private::DoesClassOverrideFunction("ActivateAbility", Class);
	
	return GASObjects.Abilities.Add(Class, Ability);
}

FDiscoveredAbility& GASDiscovery::DiscoverAbilityInBlueprint(UBlueprint* Blueprint, FGASObjects& GASObjects)
{
	FDiscoveredAbility& DiscoveredAbility = GASDiscovery::DiscoverAbility(Blueprint->GeneratedClass->GetClass(), GASObjects);

	TArray<FName> GameplayEffectFunctions = { "ApplyGameplayEffectToTarget", "ApplyGameplayEffectToOwner", "ApplyGameplayEffectSpecToTarget", "BP_ApplyGameplayEffectToTarget", "ApplyGameplayEffectSpecToSelf" };
	
	TArray<UEdGraph*> AllGraphs;
	Blueprint->GetAllGraphs(AllGraphs);
	for (const auto& Graph : AllGraphs)
	{
		for (const auto& Node: Graph->Nodes)
		{
			if (Node.IsA(UK2Node_Event::StaticClass()))
			{
				const auto* EventNode = Cast<UK2Node_Event>(Node);
				if (EventNode->EventReference.GetMemberName() == "ActivateAbility")
				{
					DiscoveredAbility.bHasActivateAbilityEvent = true;

					const UEdGraphPin* Pin = Node->FindPin(FName("exec"), EGPD_Output);
					if (Pin && !Pin->LinkedTo.IsEmpty())
					{
						// has logic
						DiscoveredAbility.bActivateAbilityHasLogic = true;
					}
				}
				continue;
			}
			
			if (Node.IsA(UK2Node_CallFunction::StaticClass()))
			{
				const auto* FunctionNode = Cast<UK2Node_CallFunction>(Node);
				if (!GameplayEffectFunctions.Find(FunctionNode->GetFunctionName()))
				{
					continue;
				}
				const UEdGraphPin* Pin = Node->FindPin(FName("GameplayEffectClass"), EGPD_Input);
				if (!Pin)
				{
					continue;
				}
				
				FDiscoveredEffectReference Ref;
				Ref.PropertyName = FunctionNode->GetFunctionName();
				
				if (Pin->LinkedTo.IsEmpty())
				{
					const auto& ResolvedClass = Pin->DefaultObject ? Pin->DefaultObject->GetClass() : nullptr;
					Ref.EffectClass = ResolvedClass;
					Ref.bWasDeclaredButUnresolved = !ResolvedClass;
					Ref.bStaticallyUnresolvable = false;
				}
				else
				{
					Ref.EffectClass = nullptr;
					Ref.bWasDeclaredButUnresolved = false;
					Ref.bStaticallyUnresolvable = true;
				}
				
				DiscoveredAbility.Effects.Add(Ref);
				
				if (Ref.EffectClass)
				{
					GASDiscovery::DiscoverEffect(Ref.EffectClass, GASObjects);
				}
			}
		}
	}
	
	return DiscoveredAbility;
}
