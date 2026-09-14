#include "GA_FireWeapon.h"
#include "WeaponBase.h"
#include "Attributes/WeaponAttributes.h"
#include "Data/WeaponTags.h"
#include "GE_Weapon_Cooldown.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"

namespace { const FName SetByCaller_CooldownDuration("Data.Cooldown.Duration"); }

UGA_FireWeapon::UGA_FireWeapon()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

    AbilityTags.AddTag(WeaponTags::Ability_Weapon_Fire.GetTag());

    // Can't fire while reloading, or while the fire-rate cooldown tag is active.
    ActivationBlockedTags.AddTag(WeaponTags::State_Weapon_Reloading.GetTag());
    ActivationBlockedTags.AddTag(WeaponTags::Cooldown_Weapon_Fire.GetTag());

    ActivationOwnedTags.AddTag(WeaponTags::State_Weapon_Firing.GetTag());
}

bool UGA_FireWeapon::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
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

    if (Weapon->GetWeaponAttributeSet()->GetCurrentAmmo() <= 0.f)
    {
        if (OptionalRelevantTags)
        {
            OptionalRelevantTags->AddTag(WeaponTags::Fail_Weapon_NoAmmo.GetTag());
        }
        return false;
    }

    return true;
}

void UGA_FireWeapon::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    AWeaponBase* Weapon = Cast<AWeaponBase>(ActorInfo->AvatarActor.Get());
    if (!Weapon)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    const FGASWeaponTableRow& Config = Weapon->GetWeaponConfig();

    if (Config.TraceType == EWeaponTraceType::Spread)
    {
        PerformSpreadTrace(Weapon);
    }
    else
    {
        PerformSingleLineTrace(Weapon);
    }

    // Consume ammo
    UWeaponAttributeSet* Attributes = Weapon->GetWeaponAttributeSet();
    const float NewAmmo = FMath::Max(0.f, Attributes->GetCurrentAmmo() - Config.AmmoCostPerShot);
    Attributes->SetCurrentAmmo(NewAmmo);

    ApplyFireRateCooldown(Weapon);

    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UGA_FireWeapon::PerformSingleLineTrace(AWeaponBase* Weapon)
{
    const FTransform Muzzle = Weapon->GetMuzzleTransform();
    const float Range = Weapon->GetWeaponAttributeSet()->GetRange();

    const FVector Start = Muzzle.GetLocation();
    const FVector End = Start + Muzzle.GetRotation().GetForwardVector() * Range;

    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(WeaponFireTrace), /*bTraceComplex=*/true, Weapon);
    Params.AddIgnoredActor(Weapon->GetOwner());

    const bool bHit = Weapon->GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);

    ResolveHit(Weapon, Hit, Start, bHit ? Hit.ImpactPoint : End);
}

void UGA_FireWeapon::PerformSpreadTrace(AWeaponBase* Weapon)
{
    const FGASWeaponTableRow& Config = Weapon->GetWeaponConfig();
    const FTransform Muzzle = Weapon->GetMuzzleTransform();
    const float Range = Weapon->GetWeaponAttributeSet()->GetRange();
    const FVector Start = Muzzle.GetLocation();

    FCollisionQueryParams Params(SCENE_QUERY_STAT(WeaponFireTraceSpread), true, Weapon);
    Params.AddIgnoredActor(Weapon->GetOwner());

    for (int32 i = 0; i < Config.PelletCount; ++i)
    {
        const FVector Direction = FMath::VRandCone(
            Muzzle.GetRotation().GetForwardVector(),
            FMath::DegreesToRadians(Config.SpreadAngleDegrees));

        const FVector End = Start + Direction * Range;

        FHitResult Hit;
        const bool bHit = Weapon->GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);

        ResolveHit(Weapon, Hit, Start, bHit ? Hit.ImpactPoint : End);
    }
}

void UGA_FireWeapon::ResolveHit(AWeaponBase* Weapon, const FHitResult& Hit, const FVector& TraceStart, const FVector& TraceEnd)
{
    const bool bHit = Hit.bBlockingHit;

    if (bDrawDebugLines)
    {
        DrawDebugLine(Weapon->GetWorld(), TraceStart, TraceEnd,
            bHit ? FColor::Green : FColor::Red,
            /*bPersistentLines=*/false, DebugLineDuration, /*DepthPriority=*/0, /*Thickness=*/1.5f);

        if (bHit)
        {
            DrawDebugPoint(Weapon->GetWorld(), Hit.ImpactPoint, 8.f, FColor::Yellow, false, DebugLineDuration);
        }
    }

    if (bHit)
    {
        const float Damage = Weapon->GetWeaponAttributeSet()->GetDamage();

        // Objects don't need a health attribute for this system — we just route the
        // damage number to whatever the raycast found. Two easy ways to consume it:

        // (A) Generic Actor damage event (works even on actors with no custom interface):
        UGameplayStatics::ApplyPointDamage(
            Hit.GetActor(), Damage, (TraceEnd - TraceStart).GetSafeNormal(), Hit,
            Weapon->GetInstigatorController(), Weapon, nullptr);

        // (B) If the hit actor implements the Ability System interface, you can instead
        // send a gameplay event / apply a GameplayEffect to its ASC here, e.g.:
        //
        // if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Hit.GetActor()))
        // {
        //     FGameplayEventData EventData;
        //     EventData.EventTag = WeaponTags::Event_Weapon_Hit.GetTag();
        //     EventData.EventMagnitude = Damage;
        //     ASI->GetAbilitySystemComponent()->HandleGameplayEvent(EventData.EventTag, &EventData);
        // }

        UE_LOG(LogTemp, Log, TEXT("[Weapon] Hit %s for %.1f damage"), *GetNameSafe(Hit.GetActor()), Damage);
    }
}

void UGA_FireWeapon::ApplyFireRateCooldown(AWeaponBase* Weapon)
{
    if (!FireCooldownEffectClass) return;

    UAbilitySystemComponent* ASC = Weapon->GetAbilitySystemComponent();
    const float CooldownSeconds = Weapon->GetWeaponAttributeSet()->GetFireRateCooldown();

    FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
    FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(FireCooldownEffectClass, 1.f, Context);

    if (Spec.IsValid())
    {
        Spec.Data->SetSetByCallerMagnitude(SetByCaller_CooldownDuration, CooldownSeconds);
        ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data);
    }
}