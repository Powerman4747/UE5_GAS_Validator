// Copyright Epic Games, Inc. All Rights Reserved.

#include "WeaponSystemCharacter.h"
#include "WeaponBase.h" 
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "WeaponsAbilitySystemComponent.h"
#include "WeaponSystem.h"
#include "WeaponSystemPlayerController.h"
#include "Data/WeaponTags.h"

AWeaponSystemCharacter::AWeaponSystemCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
	
	// Create the first person mesh that will be viewed only by this character's owner
	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("First Person Mesh"));

	FirstPersonMesh->SetupAttachment(GetMesh());
	FirstPersonMesh->SetOnlyOwnerSee(true);
	FirstPersonMesh->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
	FirstPersonMesh->SetCollisionProfileName(FName("NoCollision"));

	// Create the Camera Component	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("First Person Camera"));
	FirstPersonCameraComponent->SetupAttachment(FirstPersonMesh, FName("head"));
	FirstPersonCameraComponent->SetRelativeLocationAndRotation(FVector(-2.8f, 5.89f, 0.0f), FRotator(0.0f, 90.0f, -90.0f));
	FirstPersonCameraComponent->bUsePawnControlRotation = true;
	FirstPersonCameraComponent->bEnableFirstPersonFieldOfView = true;
	FirstPersonCameraComponent->bEnableFirstPersonScale = true;
	FirstPersonCameraComponent->FirstPersonFieldOfView = 70.0f;
	FirstPersonCameraComponent->FirstPersonScale = 0.6f;

	// configure the character comps
	GetMesh()->SetOwnerNoSee(true);
	GetMesh()->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::WorldSpaceRepresentation;

	GetCapsuleComponent()->SetCapsuleSize(34.0f, 96.0f);

	// Configure character movement
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;
	GetCharacterMovement()->AirControl = 0.5f;
}

void AWeaponSystemCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AWeaponSystemCharacter::DoJumpStart);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AWeaponSystemCharacter::DoJumpEnd);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AWeaponSystemCharacter::MoveInput);

		// Looking/Aiming
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AWeaponSystemCharacter::LookInput);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AWeaponSystemCharacter::LookInput);
		
		// Left mouse button -> Fire. IA_Fire should be bound to Left Mouse Button in its Input Action asset.
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &AWeaponSystemCharacter::OnFirePressed);

		// R -> Reload. IA_Reload should be bound to the R key.
		EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &AWeaponSystemCharacter::OnReloadPressed);
	}
	else
	{
		UE_LOG(LogWeaponSystem, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AWeaponSystemCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	if (AWeaponSystemPlayerController* PC = Cast<AWeaponSystemPlayerController>(GetController()))
        {
            if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
                ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
            {
                Subsystem->AddMappingContext(DefaultMappingContext, 0);
            }
        }
    
        if (StartingWeaponClass)
        {
            FActorSpawnParameters SpawnParams;
            SpawnParams.Owner = this;
            SpawnParams.Instigator = this;
            SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    
            EquippedWeapon = GetWorld()->SpawnActor<AWeaponBase>(StartingWeaponClass, GetActorTransform(), SpawnParams);
        	EquippedWeapon->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
        }
}

void AWeaponSystemCharacter::OnFirePressed()
{
	if (EquippedWeapon)
	{
		if (UAbilitySystemComponent* ASC = EquippedWeapon->GetAbilitySystemComponent())
		{
			ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(WeaponTags::Ability_Weapon_Fire.GetTag()));
		}
	}
}

void AWeaponSystemCharacter::OnReloadPressed()
{
	if (EquippedWeapon)
	{
		if (UAbilitySystemComponent* ASC = EquippedWeapon->GetAbilitySystemComponent())
		{
			ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(WeaponTags::Ability_Weapon_Reload.GetTag()));
		}
	}
}

void AWeaponSystemCharacter::MoveInput(const FInputActionValue& Value)
{
	// get the Vector2D move axis
	FVector2D MovementVector = Value.Get<FVector2D>();

	// pass the axis values to the move input
	DoMove(MovementVector.X, MovementVector.Y);

}

void AWeaponSystemCharacter::LookInput(const FInputActionValue& Value)
{
	// get the Vector2D look axis
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// pass the axis values to the aim input
	DoAim(LookAxisVector.X, LookAxisVector.Y);

}

void AWeaponSystemCharacter::DoAim(float Yaw, float Pitch)
{
	if (GetController())
	{
		// pass the rotation inputs
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void AWeaponSystemCharacter::DoMove(float Right, float Forward)
{
	if (GetController())
	{
		// pass the move inputs
		AddMovementInput(GetActorRightVector(), Right);
		AddMovementInput(GetActorForwardVector(), Forward);
	}
}

void AWeaponSystemCharacter::DoJumpStart()
{
	// pass Jump to the character
	Jump();
}

void AWeaponSystemCharacter::DoJumpEnd()
{
	// pass StopJumping to the character
	StopJumping();
}
