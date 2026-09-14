// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Runtime/GameplayTags/Public/NativeGameplayTags.h"

namespace WeaponTags
{
	// Abilities
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Weapon_Fire);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Weapon_Reload);

	// States that block other abilities
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Weapon_Reloading);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Weapon_Firing);

	// Cooldown tags (one per ability, applied via a GameplayEffect duration)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Weapon_Fire);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Weapon_Reload);

	// Failure reasons (for UI feedback)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Fail_Weapon_NoAmmo);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Fail_Weapon_OnCooldown);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Fail_Weapon_Reloading);

	// Damage / hit event
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Weapon_Hit);
}
