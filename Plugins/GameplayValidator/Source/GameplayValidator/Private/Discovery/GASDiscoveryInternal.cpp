// Fill out your copyright notice in the Description page of Project Settings.
#include "GASDiscoveryInternal.h"
#include "UObject/UnrealType.h"
#include "GenericPlatform/GenericPlatformMisc.h"

UClass* GASDiscovery::Private::ResolvePropertyType(const FProperty* Property)
{
	if (const FClassProperty* ClassProperty = CastField<FClassProperty>(Property))
	{
		return ClassProperty->MetaClass;
	}
	
	if (const FObjectProperty* ObjectProperty = CastField<FObjectProperty>(Property))
	{
		return ObjectProperty->PropertyClass;
	}
	
	// TODO: SoftClass
	
	return nullptr;
}

UClass* GASDiscovery::Private::ResolveClassValue(const FProperty* Property, const UObject* Instance)
{
	if (const FClassProperty* ClassProperty = CastField<FClassProperty>(Property))
	{
		return Cast<UClass>(ClassProperty->GetObjectPropertyValue_InContainer(Instance));
	}
	
	if (const FObjectProperty* ObjectProperty = CastField<FObjectProperty>(Property))
	{
		const UObject* Value = ObjectProperty->GetObjectPropertyValue_InContainer(Instance);
		return Value ? Value->GetClass() : nullptr;
	}
	
	// TODO: SoftClass
	
	return nullptr;
}

UClass* GASDiscovery::Private::ResolveClassValueFromElement(const FProperty* Property, const void* ElementPtr)
{
	if (const FClassProperty* ClassProperty = CastField<FClassProperty>(Property))
	{
		return Cast<UClass>(ClassProperty->GetObjectPropertyValue(ElementPtr));
	}
	if (const FObjectProperty* ObjectProperty = CastField<FObjectProperty>(Property))
	{
		const UObject* Value = ObjectProperty->GetObjectPropertyValue(ElementPtr);
		return Value ? Value->GetClass() : nullptr;
	}

	return nullptr;
}

const FProperty* GASDiscovery::Private::GetMostInnerProperty(const FArrayProperty* ArrayProperty)
{
	const FProperty* Inner = ArrayProperty->Inner;
	while (const FArrayProperty* Nested = CastField<FArrayProperty>(Inner))
	{
		Inner = Nested->Inner;
	}
	return Inner;
}

void GASDiscovery::Private::IterateArray(const FArrayProperty* Property, const void* Instance, const TFunctionRef<void(const FProperty*, const void*)>& Function)
{
	FScriptArrayHelper Helper(Property, Instance);
	const auto ArrayProperty = CastField<FArrayProperty>(Property->Inner);
	for (int32 i = 0; i < Helper.Num(); ++i)
	{
		const uint8* ElementPtr = Helper.GetRawPtr(i);
		
		if (ArrayProperty)
        {
            IterateArray(ArrayProperty, ElementPtr, Function);
            continue;
        }

		Function(Property->Inner, ElementPtr);		
	}
}
