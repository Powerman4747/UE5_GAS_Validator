#pragma once
#include "AbilitySystemInterface.h"
#include "Data/WeaponTableRow.h"
#include "Runtime/Engine/Classes/GameFramework/Actor.h"
#include "WeaponBase.generated.h"

class UAbilitySystemComponent;
class UWeaponAttributeSet;

UCLASS()
class WEAPONSYSTEM_API AWeaponBase : public AActor, public IAbilitySystemInterface
{
	GENERATED_BODY()
public:
	AWeaponBase();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	UWeaponAttributeSet* GetWeaponAttributeSet() const { return WeaponAttributeSet; }
	const FGASWeaponTableRow& GetWeaponConfig() const { return WeaponConfig; }

	/** Socket on the weapon mesh the raycast originates from */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	FTransform GetMuzzleTransform() const;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, Category = "Weapon")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	UPROPERTY(EditAnywhere, Category = "Weapon")
	TObjectPtr<UWeaponAttributeSet> WeaponAttributeSet;
	UPROPERTY(EditAnywhere, Category = "Weapon")
	TObjectPtr<UDataTable> WeaponDataTable;

	/** Row name inside WeaponDataTable, e.g. "Pistol" */
	UPROPERTY(EditAnywhere, Category = "Weapon")
	FName WeaponRowName;

	UPROPERTY(EditAnywhere, Category = "Weapon")
	FName MuzzleSocketName = "Muzzle";

	UPROPERTY(VisibleAnywhere, Category = "Weapon")
	TObjectPtr<USceneComponent> WeaponRoot;
	
	UPROPERTY(EditAnywhere, Category = "Weapon")
    TSubclassOf<UGameplayAbility> FireAbility;
                                           	
	UPROPERTY(EditAnywhere, Category = "Weapon")
	TSubclassOf<UGameplayAbility> ReloadAbility;

private:
	

	FGASWeaponTableRow WeaponConfig;

	FGameplayAbilitySpecHandle FireAbilityHandle;
	FGameplayAbilitySpecHandle ReloadAbilityHandle;
	
	

	void InitializeFromDataTable();
	void GrantAbilities();
};