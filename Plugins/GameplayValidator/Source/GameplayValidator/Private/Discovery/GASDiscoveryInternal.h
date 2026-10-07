// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

class UClass;
class FProperty;
class UObject;

namespace GASDiscovery::Private
{
	UClass* ResolvePropertyType(FProperty* Property);
	UClass* ResolveClassValue(FProperty* Property, UObject* Instance);
	UClass* ResolveClassValueFromElement(FProperty* Property, const void* ElementPtr);
}
