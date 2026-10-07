// Fill out your copyright notice in the Description page of Project Settings.
#include "GASDiscoveryInternal.h"
#include "UObject/UnrealType.h"

UClass* GASDiscovery::Private::ResolvePropertyType(FProperty* Property)
{
	if (FClassProperty* ClassProperty = CastField<FClassProperty>(Property))
	{
		return ClassProperty->MetaClass;
	}
	
	if (FObjectProperty* ObjectProperty = CastField<FObjectProperty>(Property))
	{
		return ObjectProperty->PropertyClass;
	}
	
	// TODO: SoftClass
	
	return nullptr;
}

UClass* GASDiscovery::Private::ResolveClassValue(FProperty* Property, UObject* Instance)
{
	if (FClassProperty* ClassProperty = CastField<FClassProperty>(Property))
	{
		return Cast<UClass>(ClassProperty->GetObjectPropertyValue_InContainer(Instance));
	}
	
	if (FObjectProperty* ObjectProperty = CastField<FObjectProperty>(Property))
	{
		const UObject* Value = ObjectProperty->GetObjectPropertyValue_InContainer(Instance);
		return Value ? Value->GetClass() : nullptr;
	}
	
	// TODO: SoftClass
	
	return nullptr;
}

UClass* GASDiscovery::Private::ResolveClassValueFromElement(FProperty* Property, const void* ElementPtr)
{
	if (FClassProperty* ClassProperty = CastField<FClassProperty>(Property))
	{
		return Cast<UClass>(ClassProperty->GetObjectPropertyValue(ElementPtr));
	}
	if (FObjectProperty* ObjectProperty = CastField<FObjectProperty>(Property))
	{
		const UObject* Value = ObjectProperty->GetObjectPropertyValue(ElementPtr);
		return Value ? Value->GetClass() : nullptr;
	}

	return nullptr;
}
