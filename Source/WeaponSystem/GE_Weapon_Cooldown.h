// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameplayEffect.h"
#include "GE_Weapon_Cooldown.generated.h"

/**
 * 
 */
UCLASS()
class WEAPONSYSTEM_API UGE_Weapon_FireCooldown : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_Weapon_FireCooldown();
};

UCLASS()
class UGE_Weapon_ReloadCooldown : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_Weapon_ReloadCooldown();
};
