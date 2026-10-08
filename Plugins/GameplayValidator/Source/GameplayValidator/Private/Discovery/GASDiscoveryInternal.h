// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

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
