#include "GA_ReloadWeapon.h"
#include "WeaponBase.h"
#include "Attributes/WeaponAttributes.h"
#include "Data/WeaponTags.h"
#include "GE_Weapon_Cooldown.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"

UGA_ReloadWeapon::UGA_ReloadWeapon()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

    AbilityTags.AddTag(WeaponTags::Ability_Weapon_Reload.GetTag());

    ActivationBlockedTags.AddTag(WeaponTags::State_Weapon_Reloading.GetTag());
    ActivationBlockedTags.AddTag(WeaponTags::Cooldown_Weapon_Reload.GetTag());

    ActivationOwnedTags.AddTag(WeaponTags::State_Weapon_Reloading.GetTag());
}

bool UGA_ReloadWeapon::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags,
    FGameplayTagContainer* OptionalRelevantTags) const
{
    if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
    {
        return false;
    }

    const AWeaponBase* Weapon = Cast<AWeaponBase>(ActorInfo->AvatarActor.Get());
    if (!Weapon || !Weapon->GetWeaponAttributeSet())
    {
        return false;
    }

    // Already full - nothing to do.
    if (Weapon->GetWeaponAttributeSet()->GetCurrentAmmo() >= Weapon->GetWeaponAttributeSet()->GetMaxAmmo())
    {
        return false;
    }

    return true;
}

void UGA_ReloadWeapon::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    AWeaponBase* Weapon = Cast<AWeaponBase>(ActorInfo->AvatarActor.Get());
    if (!Weapon)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    CachedWeapon = Weapon;
    CachedHandle = Handle;
    CachedActorInfo = *ActorInfo;
    CachedActivationInfo = ActivationInfo;

    const float ReloadSeconds = Weapon->GetWeaponAttributeSet()->GetReloadDuration();

    // NOTE: play your reload montage here via UAbilityTask_PlayMontageAndWait if desired -
    // that task's OnCompleted delegate can call the same OnReloadComplete logic.
    UAbilityTask_WaitDelay* WaitTask = UAbilityTask_WaitDelay::WaitDelay(this, ReloadSeconds);
    WaitTask->OnFinish.AddDynamic(this, &UGA_ReloadWeapon::OnReloadComplete);
    WaitTask->ReadyForActivation();
}

void UGA_ReloadWeapon::OnReloadComplete()
{
    if (AWeaponBase* Weapon = CachedWeapon.Get())
    {
        UWeaponAttributeSet* Attributes = Weapon->GetWeaponAttributeSet();
        Attributes->SetCurrentAmmo(Attributes->GetMaxAmmo());

        ApplyReloadCooldown(Weapon);
    }

    EndAbility(CachedHandle, &CachedActorInfo, CachedActivationInfo, true, false);
}

void UGA_ReloadWeapon::ApplyReloadCooldown(AWeaponBase* Weapon)
{
    if (!ReloadCooldownEffectClass) return;

    UAbilitySystemComponent* ASC = Weapon->GetAbilitySystemComponent();

    // A short, fixed cooldown so the player can't spam Reload the instant it finishes;
    // separate from ReloadDuration itself (which is the time the reload animation/wait takes).
    const float PostReloadCooldown = 0.25f;

    FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
    FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(ReloadCooldownEffectClass, 1.f, Context);

    if (Spec.IsValid())
    {
        Spec.Data->SetSetByCallerMagnitude(WeaponTags::SetByCaller_CooldownDuration, PostReloadCooldown);
        ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data);
    }
}