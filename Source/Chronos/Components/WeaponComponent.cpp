// Fill out your copyright notice in the Description page of Project Settings.

#include "WeaponComponent.h"
#include "Chronos/Chronos.h"
#include "Chronos/DataAssets/WeaponDataAsset.h"
#include "Chronos/Interactables/WeaponPickup.h"
#include "Chronos/Subsystems/ProjectilePoolSubsystem.h"
#include "Chronos/Tags/ChronosTags.h"
#include "Components/SceneComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

UWeaponComponent::UWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	MuzzleComponent = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzleComponent"));
}

void UWeaponComponent::BeginPlay()
{
	Super::BeginPlay();

	if (MuzzleComponent && GetOwner())
	{
		MuzzleComponent->SetupAttachment(GetOwner()->GetRootComponent());
	}

	if (CurrentWeaponData != nullptr)
	{
		Execute_EquipWeapon(this, CurrentWeaponData);
	}
}

void UWeaponComponent::EquipWeapon_Implementation(UWeaponDataAsset *WeaponData)
{
	if (!WeaponData)
	{
		return;
	}

	if (CurrentWeaponData != nullptr)
	{
		FVector DropLocation = GetOwner()->GetActorLocation();
		FVector DropVelocity = GetOwner()->GetActorUpVector() * 200.0f + GetOwner()->GetActorForwardVector() * 100.0f;

		DropCurrentWeapon(DropLocation, DropVelocity);
	}

	CurrentWeaponData = WeaponData;

	MaxAmmo = CurrentWeaponData->WeaponData.MagazineSize;
	CurrentAmmo = MaxAmmo;

	UpdateWeaponState();

	OnWeaponChanged.Broadcast(CurrentWeaponData);
}

UWeaponDataAsset *UWeaponComponent::GetCurrentWeapon_Implementation() const
{
	return CurrentWeaponData;
}

void UWeaponComponent::Fire_Implementation()
{
	if (!CanFire_Implementation())
	{
		return;
	}

	OwnedTags.AddTag(FChronosTags::State_Shooting);

	CurrentAmmo--;

	LastFireTime = GetWorld()->GetTimeSeconds();

	SpawnProjectile();

	OnFired.Broadcast();

	UpdateWeaponState();
}

bool UWeaponComponent::CanFire_Implementation() const
{
	if (!CurrentWeaponData || !GetWorld())
	{
		return false;
	}

	if (CurrentAmmo <= 0)
	{
		return false;
	}

	float FireRate = CurrentWeaponData->WeaponData.FireRate;
	float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastFireTime < FireRate)
	{
		return false;
	}

	return true;
}

void UWeaponComponent::ThrowWeapon_Implementation()
{
	if (!CanThrow_Implementation())
	{
		return;
	}

	bIsThrowing = true;

	FVector ThrowLocation = GetMuzzleLocation_Implementation();
	FVector ThrowDirection = GetOwner()->GetActorForwardVector();
	ThrowDirection.Z += 0.1f;
	ThrowDirection.Normalize();

	FVector ThrowVelocity = ThrowDirection * ThrowForce;

	DropCurrentWeapon(ThrowLocation, ThrowVelocity);

	CurrentWeaponData = nullptr;
	CurrentAmmo = 0;
	MaxAmmo = 0;

	UpdateWeaponState();

	OnWeaponChanged.Broadcast(nullptr);
	OnWeaponThrown.Broadcast();

	bIsThrowing = false;
}

bool UWeaponComponent::CanThrow_Implementation() const
{
	if (!CurrentWeaponData || !GetOwner() || !GetWorld())
	{
		return false;
	}

	if (bIsThrowing)
	{
		return false;
	}

	return true;
}

FVector UWeaponComponent::GetMuzzleLocation_Implementation() const
{
	if (MuzzleComponent)
	{
		return MuzzleComponent->GetComponentLocation();
	}

	if (const AActor *Owner = GetOwner())
	{
		return Owner->GetActorLocation() + Owner->GetActorForwardVector() * 100.0f;
	}
	return FVector::ZeroVector;
}

FRotator UWeaponComponent::GetMuzzleRotation_Implementation() const
{
	if (MuzzleComponent)
	{
		return MuzzleComponent->GetComponentRotation();
	}

	if (const AActor *Owner = GetOwner())
	{
		return Owner->GetActorRotation();
	}
	return FRotator::ZeroRotator;
}

int32 UWeaponComponent::GetMagazineSize() const
{
	return CurrentWeaponData ? CurrentWeaponData->WeaponData.MagazineSize : 0;
}

float UWeaponComponent::GetFireRate() const
{
	return CurrentWeaponData ? CurrentWeaponData->WeaponData.FireRate : 0.0f;
}

bool UWeaponComponent::IsShooting() const
{
	return OwnedTags.HasTag(FChronosTags::State_Shooting);
}

bool UWeaponComponent::HasWeapon() const
{
	return CurrentWeaponData != nullptr;
}

void UWeaponComponent::DropCurrentWeapon(const FVector &DropLocation, const FVector &DropVelocity)
{
	if (!CurrentWeaponData || !GetWorld())
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	UWeaponDataAsset *WeaponToDrop = CurrentWeaponData;

	AWeaponPickup *Pickup = GetWorld()->SpawnActor<AWeaponPickup>(
		AWeaponPickup::StaticClass(),
		DropLocation,
		GetOwner()->GetActorRotation(),
		SpawnParams);

	if (Pickup)
	{
		Pickup->SetWeaponDataAsset(WeaponToDrop);

		UStaticMeshComponent *MeshComp = Pickup->FindComponentByClass<UStaticMeshComponent>();
		if (MeshComp)
		{
			MeshComp->SetPhysicsLinearVelocity(DropVelocity);
		}
	}
}

void UWeaponComponent::SpawnProjectile() const
{
	if (!CurrentWeaponData || !GetWorld())
	{
		return;
	}

	UProjectilePoolSubsystem *PoolSubsystem = ChronosSubsystems::GetProjectilePoolSubsystem(GetWorld());

	if (!PoolSubsystem)
	{
		return;
	}

	const FWeaponData &WeaponData = CurrentWeaponData->WeaponData;

	FTransform SpawnTransform;
	SpawnTransform.SetLocation(GetMuzzleLocation_Implementation());
	SpawnTransform.SetRotation(GetMuzzleRotation_Implementation().Quaternion());

	AActor *Projectile = PoolSubsystem->GetProjectile(WeaponData.ProjectileClass, SpawnTransform);

	if (Projectile)
	{
		UProjectileMovementComponent *ProjectileMovement = Projectile->FindComponentByClass<UProjectileMovementComponent>();

		if (ProjectileMovement)
		{
			ProjectileMovement->InitialSpeed = WeaponData.BulletSpeed;
			ProjectileMovement->MaxSpeed = WeaponData.BulletSpeed;
		}
	}
}

void UWeaponComponent::UpdateWeaponState()
{
	if (CurrentAmmo <= 0)
	{
		OwnedTags.RemoveTag(FChronosTags::State_Shooting);
	}
}
