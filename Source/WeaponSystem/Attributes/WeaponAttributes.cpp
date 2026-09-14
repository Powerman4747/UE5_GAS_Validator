#include "WeaponAttributes.h"

void UWeaponAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetCurrentAmmoAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxAmmo());
	}
}