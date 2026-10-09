// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

struct FDiscoveredCalculationReference;
struct FDiscoveredEffectReference;
class UClass;
class FProperty;
class UObject;

namespace GASDiscovery::Private
{
	UClass* ResolvePropertyType(const FProperty* Property);
	UClass* ResolveClassValue(const FProperty* Property, const UObject* Instance);
	UClass* ResolveClassValueFromElement(const FProperty* Property, const void* ElementPtr);

	const FProperty* GetMostInnerProperty(const FArrayProperty* ArrayProperty);
	void IterateArray(const FArrayProperty* Property, const void* Instance, const TFunctionRef<void(const FProperty*, const void*)>& Function);
}

namespace GASDiscovery::Private::Effect
{
	FDiscoveredEffectReference& CreateEffectReference(const FProperty* Property, UClass* EffectClass);
	
	FDiscoveredCalculationReference& CreateCalculationReference(const FProperty* Property, const void* Instance);
}