// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Runtime/Engine/Classes/Engine/DataTable.h"
#include "Templates/SubclassOf.h"
#include "UObject/Object.h"
#include "../GA_FireWeapon.h"
#include "WeaponTableRow.generated.h"

class UGA_FireWeapon;
class UGA_ReloadWeapon;

UENUM(BlueprintType)
enum class EWeaponTraceType : uint8
{
	SingleLine,   // pistol / rifle / sniper - one raycast
	Spread        // shotgun - multiple raycasts within a cone
};

USTRUCT(BlueprintType)
struct WEAPONSYSTEM_API FGASWeaponTableRow : public FTableRowBase
{
	GENERATED_BODY()

    /** Display name, e.g. "Pistol" */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
    FText WeaponName;

    /** Damage applied per successful raycast hit */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
    float Damage = 10.f;

    /** Max distance the raycast travels, in cm (Unreal units) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
    float Range = 5000.f;

    /** Minimum seconds between shots. This is the fire-rate cooldown. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0.01"))
    float FireRateCooldown = 0.15f;

    /** Seconds the Reload ability's cooldown/duration takes */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0.1"))
    float ReloadDuration = 1.6f;

    /** Rounds per magazine */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "1"))
    int32 MagazineSize = 12;

    /** Single raycast, or a shotgun-style spread of several */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
    EWeaponTraceType TraceType = EWeaponTraceType::SingleLine;

    /** Only used when TraceType == Spread */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (EditCondition = "TraceType == EWeaponTraceType::Spread"))
    int32 PelletCount = 8;

    /** Half-angle of the spread cone, in degrees */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (EditCondition = "TraceType == EWeaponTraceType::Spread"))
    float SpreadAngleDegrees = 6.f;

    /** Ammo consumed per trigger pull (1 for most, PelletCount is separate for shotgun) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "1"))
    int32 AmmoCostPerShot = 1;

    /** Static mesh / skeletal mesh to spawn on the weapon actor */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
    TSoftObjectPtr<UObject> WeaponMesh;

    /** Ability classes granted to this weapon's ASC - lets you override fire/reload logic per weapon if needed */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
    TSubclassOf<UGA_FireWeapon> FireAbilityClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
    TSubclassOf<UGA_ReloadWeapon> ReloadAbilityClass;
};
