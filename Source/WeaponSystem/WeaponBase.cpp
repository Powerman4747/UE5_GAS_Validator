#include "WeaponBase.h"
#include "AbilitySystemComponent.h"
#include "Attributes/WeaponAttributes.h"
#include "GA_FireWeapon.h"
#include "GA_ReloadWeapon.h"

AWeaponBase::AWeaponBase()
{
    PrimaryActorTick.bCanEverTick = false;

    WeaponRoot = CreateDefaultSubobject<USceneComponent>("WeaponRoot");
    SetRootComponent(WeaponRoot);

    AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>("AbilitySystemComponent");
    AbilitySystemComponent->SetIsReplicated(true);
    AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

    WeaponAttributeSet = CreateDefaultSubobject<UWeaponAttributeSet>("WeaponAttributeSet");
}
void AWeaponBase::BeginPlay()
{
    Super::BeginPlay();

    AbilitySystemComponent->InitAbilityActorInfo(this, this);
    InitializeFromDataTable();
    GrantAbilities();
}

void AWeaponBase::InitializeFromDataTable()
{
    if (!WeaponDataTable) return;

    if (const FGASWeaponTableRow* Row = WeaponDataTable->FindRow<FGASWeaponTableRow>(WeaponRowName, TEXT("WeaponInit")))
    {
        WeaponConfig = *Row;
    }

    // Push the config values into the real GameplayAttributes.
    WeaponAttributeSet->SetMaxAmmo(static_cast<float>(WeaponConfig.MagazineSize));
    WeaponAttributeSet->SetCurrentAmmo(static_cast<float>(WeaponConfig.MagazineSize)); // start full
    WeaponAttributeSet->SetDamage(WeaponConfig.Damage);
    WeaponAttributeSet->SetRange(WeaponConfig.Range);
    WeaponAttributeSet->SetFireRateCooldown(WeaponConfig.FireRateCooldown);
    WeaponAttributeSet->SetReloadDuration(WeaponConfig.ReloadDuration);
}

void AWeaponBase::GrantAbilities()
{
    if (!AbilitySystemComponent) return;

    TSubclassOf<UGA_FireWeapon> FireClass = WeaponConfig.FireAbilityClass
        ? TSubclassOf<UGA_FireWeapon>(WeaponConfig.FireAbilityClass)
        : TSubclassOf<UGA_FireWeapon>(UGA_FireWeapon::StaticClass());

    TSubclassOf<UGA_ReloadWeapon> ReloadClass = WeaponConfig.ReloadAbilityClass
        ? TSubclassOf<UGA_ReloadWeapon>(WeaponConfig.ReloadAbilityClass)
        : TSubclassOf<UGA_ReloadWeapon>(UGA_ReloadWeapon::StaticClass());

    FireAbilityHandle = AbilitySystemComponent->GiveAbility(
        FGameplayAbilitySpec(FireClass, 1, INDEX_NONE, this));

    ReloadAbilityHandle = AbilitySystemComponent->GiveAbility(
        FGameplayAbilitySpec(ReloadClass, 1, INDEX_NONE, this));
}

FTransform AWeaponBase::GetMuzzleTransform() const
{
    /*if (WeaponMeshComponent && WeaponMeshComponent->DoesSocketExist(MuzzleSocketName))
    {
        return WeaponMeshComponent->GetSocketTransform(MuzzleSocketName);
    }*/
    return GetActorTransform();
}

UAbilitySystemComponent* AWeaponBase::GetAbilitySystemComponent() const
{
    return AbilitySystemComponent;
}
