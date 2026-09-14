#include "GE_Weapon_Cooldown.h"
#include "Data/WeaponTags.h"

namespace { const FName SetByCaller_CooldownDuration("Data.Cooldown.Duration"); }

UGE_Weapon_FireCooldown::UGE_Weapon_FireCooldown()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	FSetByCallerFloat SetByCallerMagnitude;
	SetByCallerMagnitude.DataName = SetByCaller_CooldownDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(SetByCallerMagnitude);

	FInheritedTagContainer TagContainer;
	TagContainer.CombinedTags.AddTag(WeaponTags::Cooldown_Weapon_Fire.GetTag());
	InheritableOwnedTagsContainer = TagContainer;
}

UGE_Weapon_ReloadCooldown::UGE_Weapon_ReloadCooldown()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	FSetByCallerFloat SetByCallerMagnitude;
	SetByCallerMagnitude.DataName = SetByCaller_CooldownDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(SetByCallerMagnitude);

	FInheritedTagContainer TagContainer;
	TagContainer.CombinedTags.AddTag(WeaponTags::Cooldown_Weapon_Reload.GetTag());
	InheritableOwnedTagsContainer = TagContainer;
}