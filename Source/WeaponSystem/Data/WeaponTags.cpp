#include "WeaponTags.h"

namespace WeaponTags
{
	UE_DEFINE_GAMEPLAY_TAG(Ability_Weapon_Fire,        "Ability.Weapon.Fire");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Weapon_Reload,      "Ability.Weapon.Reload");

	UE_DEFINE_GAMEPLAY_TAG(State_Weapon_Reloading,     "State.Weapon.Reloading");
	UE_DEFINE_GAMEPLAY_TAG(State_Weapon_Firing,        "State.Weapon.Firing");

	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Weapon_Fire,       "Cooldown.Weapon.Fire");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Weapon_Reload,     "Cooldown.Weapon.Reload");

	UE_DEFINE_GAMEPLAY_TAG(Fail_Weapon_NoAmmo,         "Fail.Weapon.NoAmmo");
	UE_DEFINE_GAMEPLAY_TAG(Fail_Weapon_OnCooldown,     "Fail.Weapon.OnCooldown");
	UE_DEFINE_GAMEPLAY_TAG(Fail_Weapon_Reloading,      "Fail.Weapon.Reloading");

	UE_DEFINE_GAMEPLAY_TAG(Event_Weapon_Hit,           "Event.Weapon.Hit");
	
	const FName SetByCaller_CooldownDuration("Data.Cooldown.Duration");
}