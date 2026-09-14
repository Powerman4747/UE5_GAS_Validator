#pragma once
#include "Abilities/GameplayAbility.h"
#include "GA_FireWeapon.generated.h"

class AWeaponBase;

UCLASS()
class UGA_FireWeapon : public UGameplayAbility
{
	GENERATED_BODY()
public:
	UGA_FireWeapon();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags,
		FGameplayTagContainer* OptionalRelevantTags) const override;

protected:
	/** How long the debug trace lines stay visible, in seconds */
	UPROPERTY(EditDefaultsOnly, Category = "Debug")
	float DebugLineDuration = 2.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Debug")
	bool bDrawDebugLines = true;

	UPROPERTY(EditDefaultsOnly, Category = "Cooldown")
	TSubclassOf<class UGE_Weapon_FireCooldown> FireCooldownEffectClass;

	void PerformSingleLineTrace(AWeaponBase* Weapon);
	void PerformSpreadTrace(AWeaponBase* Weapon);
	void ResolveHit(AWeaponBase* Weapon, const FHitResult& Hit, const FVector& TraceStart, const FVector& TraceEnd);
	void ApplyFireRateCooldown(AWeaponBase* Weapon);
};