#pragma once
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "WeaponAttributes.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class WEAPONSYSTEM_API UWeaponAttributeSet : public UAttributeSet
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadOnly, Category = "Weapon|Ammo")
	FGameplayAttributeData CurrentAmmo;
	ATTRIBUTE_ACCESSORS(UWeaponAttributeSet, CurrentAmmo)

	UPROPERTY(BlueprintReadOnly, Category = "Weapon|Ammo")
	FGameplayAttributeData MaxAmmo;
	ATTRIBUTE_ACCESSORS(UWeaponAttributeSet, MaxAmmo)

	UPROPERTY(BlueprintReadOnly, Category = "Weapon|Stats")
	FGameplayAttributeData Damage;
	ATTRIBUTE_ACCESSORS(UWeaponAttributeSet, Damage)

	UPROPERTY(BlueprintReadOnly, Category = "Weapon|Stats")
	FGameplayAttributeData Range;
	ATTRIBUTE_ACCESSORS(UWeaponAttributeSet, Range)

	UPROPERTY(BlueprintReadOnly, Category = "Weapon|Stats")
	FGameplayAttributeData FireRateCooldown;
	ATTRIBUTE_ACCESSORS(UWeaponAttributeSet, FireRateCooldown)

	UPROPERTY(BlueprintReadOnly, Category = "Weapon|Stats")
	FGameplayAttributeData ReloadDuration;
	ATTRIBUTE_ACCESSORS(UWeaponAttributeSet, ReloadDuration)

	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
};