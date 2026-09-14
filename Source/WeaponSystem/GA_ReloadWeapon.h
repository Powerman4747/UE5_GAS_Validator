#pragma once
#include "Abilities/GameplayAbility.h"
#include "GA_ReloadWeapon.generated.h"

class AWeaponBase;

UCLASS()
class UGA_ReloadWeapon : public UGameplayAbility
{
	GENERATED_BODY()
public:
	UGA_ReloadWeapon();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags,
		FGameplayTagContainer* OptionalRelevantTags) const override;

	UFUNCTION()
	void OnReloadComplete();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Cooldown")
	TSubclassOf<class UGE_Weapon_ReloadCooldown> ReloadCooldownEffectClass;

private:
	TWeakObjectPtr<AWeaponBase> CachedWeapon;
	FGameplayAbilitySpecHandle CachedHandle;
	FGameplayAbilityActorInfo CachedActorInfo;
	FGameplayAbilityActivationInfo CachedActivationInfo;

	void ApplyReloadCooldown(AWeaponBase* Weapon);
};