// Fill out your copyright notice in the Description page of Project Settings.

#include "PlayerCharacter.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Camera/CameraComponent.h"
#include "Chronos/Chronos.h"
#include "Chronos/Components/HealthComponent.h"
#include "Chronos/Components/WeaponComponent.h"
#include "Chronos/Subsystems/TimeDilationSubsystem.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"

APlayerCharacter::APlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
	WeaponComponent = CreateDefaultSubobject<UWeaponComponent>(TEXT("WeaponComponent"));
	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComponent"));
	SpringArmComponent->SetupAttachment(RootComponent);
	SpringArmComponent->TargetArmLength = 0.0f;
	SpringArmComponent->bUsePawnControlRotation = true;

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetupAttachment(SpringArmComponent);
	CameraComponent->bUsePawnControlRotation = false;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
}

void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	BindDeathEvent();

	if (APlayerController *PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem *InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			InputSubsystem->AddMappingContext(InputMappingContext, 0);
		}
	}

	bHasInputActivity = false;
	bIsDead = false;

	if (UTimeDilationSubsystem *TimeSubsystem = ChronosSubsystems::GetTimeDilationSubsystem(GetWorld()))
	{
		TimeSubsystem->SetPlayerDead(false);
	}
}

void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsDead)
	{
		return;
	}

	UpdateInputActivity();
}

void APlayerCharacter::SetupPlayerInputComponent(UInputComponent *PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent *EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlayerCharacter::HandleMoveInput);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &APlayerCharacter::HandleLookInput);
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &APlayerCharacter::HandleFireInput);
		EnhancedInputComponent->BindAction(ThrowAction, ETriggerEvent::Started, this, &APlayerCharacter::HandleThrowInput);
	}
}

bool APlayerCharacter::IsAlive() const
{
	return HealthComponent ? HealthComponent->IsAlive() : true;
}

void APlayerCharacter::OnPlayerDeath(AActor *Killer)
{
	if (bIsDead)
	{
		return;
	}

	bIsDead = true;

	GetCharacterMovement()->DisableMovement();

	if (APlayerController *PlayerController = Cast<APlayerController>(GetController()))
	{
		PlayerController->DisableInput(PlayerController);
		SetGameInputMode();
	}

	if (UTimeDilationSubsystem *TimeSubsystem = ChronosSubsystems::GetTimeDilationSubsystem(GetWorld()))
	{
		TimeSubsystem->SetPlayerDead(true);
	}
}

void APlayerCharacter::RespawnPlayer()
{
	bIsDead = false;

	if (HealthComponent)
	{
		HealthComponent->Revive();
	}

	if (WeaponComponent && WeaponComponent->GetCurrentWeapon_Implementation())
	{
		WeaponComponent->EquipWeapon_Implementation(WeaponComponent->GetCurrentWeapon_Implementation());
	}

	GetCharacterMovement()->SetMovementMode(MOVE_Walking);

	if (APlayerController *PlayerController = Cast<APlayerController>(GetController()))
	{
		PlayerController->EnableInput(PlayerController);
		SetGameInputMode();
	}

	CurrentPitch = 0.0f;
	Controller->SetControlRotation(FRotator::ZeroRotator);

	if (UTimeDilationSubsystem *TimeSubsystem = ChronosSubsystems::GetTimeDilationSubsystem(GetWorld()))
	{
		TimeSubsystem->SetPlayerDead(false);
	}
}

void APlayerCharacter::HandleMoveInput(const FInputActionValue &Value)
{
	if (bIsDead)
	{
		return;
	}

	bHasInputActivity = true;

	const FVector2D MoveDirection = Value.Get<FVector2D>();
	const FRotator YawRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(ForwardDirection, MoveDirection.Y);
	AddMovementInput(RightDirection, MoveDirection.X);
}

void APlayerCharacter::HandleLookInput(const FInputActionValue &Value)
{
	if (bIsDead)
	{
		return;
	}

	bHasInputActivity = true;

	const FVector2D LookDelta = Value.Get<FVector2D>() * MouseSensitivity;
	const float YAxisValue = bInvertYAxis ? -LookDelta.Y : LookDelta.Y;

	CurrentPitch = FMath::Clamp(CurrentPitch + YAxisValue, -MaxPitchAngle, MaxPitchAngle);

	AddControllerYawInput(LookDelta.X);
	AddControllerPitchInput(YAxisValue);
}

void APlayerCharacter::HandleFireInput()
{
	if (bIsDead)
	{
		return;
	}

	bHasInputActivity = true;
	bIsFiring = true;

	if (WeaponComponent && WeaponComponent->CanFire_Implementation())
	{
		WeaponComponent->Fire_Implementation();
	}
}

void APlayerCharacter::HandleThrowInput()
{
	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Orange, TEXT("[Player] HandleThrowInput called"));
	
	if (bIsDead)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("[Player] HandleThrowInput: Player is dead!"));
		return;
	}

	bHasInputActivity = true;

	if (WeaponComponent && WeaponComponent->CanThrow_Implementation())
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, TEXT("[Player] HandleThrowInput: Calling ThrowWeapon"));
		WeaponComponent->ThrowWeapon_Implementation();
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, TEXT("[Player] HandleThrowInput: Cannot throw"));
	}
}

void APlayerCharacter::BindDeathEvent()
{
	if (HealthComponent)
	{
		HealthComponent->OnDeath.AddDynamic(this, &APlayerCharacter::OnPlayerDeath);
	}
}

void APlayerCharacter::UpdateInputActivity()
{
	bool bCurrentInputActivity = bHasInputActivity || bIsFiring;

	if (UTimeDilationSubsystem *TimeSubsystem = ChronosSubsystems::GetTimeDilationSubsystem(GetWorld()))
	{
		TimeSubsystem->SetInputActive(bCurrentInputActivity);
	}

	bHasInputActivity = false;
	bIsFiring = false;
}

void APlayerCharacter::SetGameInputMode()
{
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		FInputModeGameOnly InputMode;
		PlayerController->SetInputMode(InputMode);
		PlayerController->bShowMouseCursor = false;
	}
}

void APlayerCharacter::SetUIInputMode()
{
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		FInputModeUIOnly InputMode;
		PlayerController->SetInputMode(InputMode);
		PlayerController->bShowMouseCursor = true;
	}
}
