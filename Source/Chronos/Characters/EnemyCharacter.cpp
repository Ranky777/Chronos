// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyCharacter.h"

#include "Chronos/Components/HealthComponent.h"
#include "Chronos/Components/WeaponComponent.h"
#include "Chronos/DataAssets/WeaponDataAsset.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"


// Sets default values
AEnemyCharacter::AEnemyCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
	
	WeaponComponent = CreateDefaultSubobject<UWeaponComponent>(TEXT("WeaponComponent"));
	
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 300.0f, 0.0f);
	
	DefaultEnemyData.ShootingFrequency = 2.0f;
	DefaultEnemyData.MovementSpeed = 150.0f;
	DefaultEnemyData.DetectionRange = 2000.0f;
	DefaultEnemyData.AttackRange = 1500.0f;
}

// Called when the game starts or when spawned
void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	ApplyEnemyData();
	
	BindDeathEvent();
	
	bIsDead = false;
}

void AEnemyCharacter::BindDeathEvent()
{
	if (HealthComponent)
	{
		HealthComponent->OnDeath.AddDynamic(this, &AEnemyCharacter::OnEnemyDeath);
	}
}

void AEnemyCharacter::OnEnemyDeath(AActor* Killer)
{
	if (bIsDead)
	{
		return;
	}
	
	bIsDead = true;
	
	GetCharacterMovement()->DisableMovement();
	
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	if (GetMesh())
	{
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

bool AEnemyCharacter::IsAlive() const
{
	if (HealthComponent)
	{
		return HealthComponent->IsAlive();
	}
	
	return !bIsDead;
}

void AEnemyCharacter::ApplyEnemyData()
{
	const FEnemyData& EnemyData = GetEnemyData();
	
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = EnemyData.MovementSpeed;
	}
	
	if (WeaponComponent && EnemyData.WeaponDataAsset)
	{
		WeaponComponent->EquipWeapon_Implementation(EnemyData.WeaponDataAsset);
	}
}



