// Fill out your copyright notice in the Description page of Project Settings.

#include "WeaponComponent.h"
#include "Chronos/Chronos.h"
#include "Chronos/DataAssets/WeaponDataAsset.h"
#include "Chronos/Interactables/WeaponPickup.h"
#include "Chronos/Subsystems/ProjectilePoolSubsystem.h"
#include "Chronos/Tags/ChronosTags.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/ProjectileMovementComponent.h"

UWeaponComponent::UWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	MuzzleComponent = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzleComponent"));
	WeaponMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMeshComponent"));
}

void UWeaponComponent::BeginPlay()
{
	Super::BeginPlay();

	if (MuzzleComponent && GetOwner())
	{
		MuzzleComponent->SetupAttachment(GetOwner()->GetRootComponent());
	}

	if (WeaponMeshComponent)
	{
		WeaponMeshComponent->SetHiddenInGame(true);
		WeaponMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void UWeaponComponent::EquipWeapon_Implementation(UWeaponDataAsset *WeaponData)
{
	// 只有当武器不同时才丢弃旧武器
	if (CurrentWeaponData != nullptr && CurrentWeaponData != WeaponData)
	{
		FVector DropLocation = GetOwner()->GetActorLocation();
		FVector DropVelocity = GetOwner()->GetActorUpVector() * 200.0f + GetOwner()->GetActorForwardVector() * 100.0f;
		DropCurrentWeapon(DropLocation, DropVelocity);
	}

	CurrentWeaponData = WeaponData;

	if (CurrentWeaponData != nullptr)
	{
		RemainingAmmo = CurrentWeaponData->WeaponData.TotalAmmo;

		if (WeaponMeshComponent)
		{
			WeaponMeshComponent->SetSkeletalMesh(CurrentWeaponData->WeaponData.WeaponMesh);
			WeaponMeshComponent->SetHiddenInGame(false);
			WeaponMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

			ACharacter *OwnerCharacter = Cast<ACharacter>(GetOwner());
			if (OwnerCharacter && OwnerCharacter->GetMesh())
			{
				// 先取消之前的附加
				WeaponMeshComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
				WeaponMeshComponent->AttachToComponent(OwnerCharacter->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, CurrentWeaponData->WeaponData.WeaponSocketName);
			}
		}

		if (MuzzleComponent && WeaponMeshComponent)
		{
			MuzzleComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
			MuzzleComponent->AttachToComponent(WeaponMeshComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale, CurrentWeaponData->WeaponData.MuzzleSocketName);
		}
		
		if (CurrentWeaponData->GetFPAnimBlueprint())
		{
			OnAnimBlueprintChanged.Broadcast(CurrentWeaponData->GetFPAnimBlueprint());
		}

		// Play equip montage if available
		if (CurrentWeaponData->GetEquipMontage())
		{
			// We'll trigger this from Blueprint
			OnWeaponChanged.Broadcast(CurrentWeaponData);
		}
	}
	else
	{
		RemainingAmmo = 0;

		if (WeaponMeshComponent)
		{
			WeaponMeshComponent->SetHiddenInGame(true);
		}

		if (MuzzleComponent)
		{
			MuzzleComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
			MuzzleComponent->SetupAttachment(GetOwner()->GetRootComponent());
		}
		
		OnAnimBlueprintChanged.Broadcast(nullptr);
	}

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

	RemainingAmmo--;

	LastFireTime = GetWorld()->GetTimeSeconds();

	SpawnProjectile();

	OnFired.Broadcast();

	// 子弹耗尽时自动丢弃武器
	if (RemainingAmmo <= 0)
	{
		FVector DropLocation = GetMuzzleLocation_Implementation();
		FVector DropVelocity = GetOwner()->GetActorForwardVector() * 50.0f + GetOwner()->GetActorUpVector() * 20.0f;
		DropCurrentWeapon(DropLocation, DropVelocity);

		CurrentWeaponData = nullptr;
		RemainingAmmo = 0;
		OnWeaponChanged.Broadcast(nullptr);
	}

	UpdateWeaponState();

	// 射击后移除 State_Shooting 标签（避免状态残留）
	OwnedTags.RemoveTag(FChronosTags::State_Shooting);
}

bool UWeaponComponent::CanFire_Implementation() const
{
	if (!CurrentWeaponData || !GetWorld())
	{
		return false;
	}

	if (RemainingAmmo <= 0)
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
	FString OwnerName = GetOwner() ? GetOwner()->GetName() : TEXT("NoOwner");
	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow,
									 FString::Printf(TEXT("[%s] ThrowWeapon_Implementation called"), *OwnerName));

	if (!CanThrow_Implementation())
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red,
										 FString::Printf(TEXT("[%s] ThrowWeapon: Cannot throw"), *OwnerName));
		return;
	}

	bIsThrowing = true;

	FVector ThrowLocation = GetMuzzleLocation_Implementation();
	FVector ThrowDirection = GetOwner()->GetActorForwardVector();
	ThrowDirection.Z += 0.1f;
	ThrowDirection.Normalize();

	FVector ThrowVelocity = ThrowDirection * ThrowForce;

	FString LocationStr = ThrowLocation.ToString();
	FString VelocityStr = ThrowVelocity.ToString();
	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan,
									 FString::Printf(TEXT("[%s] ThrowWeapon: Location=%s, Velocity=%s"),
													 *OwnerName, *LocationStr, *VelocityStr));

	DropCurrentWeapon(ThrowLocation, ThrowVelocity);

	CurrentWeaponData = nullptr;
	RemainingAmmo = 0;

	UpdateWeaponState();

	OnWeaponChanged.Broadcast(nullptr);
	OnWeaponThrown.Broadcast();

	bIsThrowing = false;

	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green,
									 FString::Printf(TEXT("[%s] ThrowWeapon completed"), *OwnerName));
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
	FString OwnerName = GetOwner() ? GetOwner()->GetName() : TEXT("NoOwner");

	if (!CurrentWeaponData || !GetWorld())
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red,
										 FString::Printf(TEXT("[%s] DropCurrentWeapon: Failed - WeaponData: %s, World: %s"),
														 *OwnerName, CurrentWeaponData ? TEXT("Valid") : TEXT("Null"), GetWorld() ? TEXT("Valid") : TEXT("Null")));
		return;
	}

	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Blue,
									 FString::Printf(TEXT("[%s] DropCurrentWeapon: Weapon=%s"),
													 *OwnerName, *CurrentWeaponData->GetName()));

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
		Pickup->SetThrower(GetOwner());

		FString ThrowerName = FString(GetOwner()->GetName());
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green,
										 FString::Printf(TEXT("[%s] DropCurrentWeapon: Pickup spawned, Thrower=%s"),
														 *OwnerName, *ThrowerName));

		USkeletalMeshComponent *MeshComp = Pickup->FindComponentByClass<USkeletalMeshComponent>();
		if (MeshComp)
		{
			MeshComp->SetPhysicsLinearVelocity(DropVelocity);
			FString VelocityStr = DropVelocity.ToString();
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan,
											 FString::Printf(TEXT("[%s] DropCurrentWeapon: Set velocity=%s"),
															 *OwnerName, *VelocityStr));
		}
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red,
										 FString::Printf(TEXT("[%s] DropCurrentWeapon: Failed to spawn pickup!"),
														 *OwnerName));
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
	if (RemainingAmmo <= 0)
	{
		OwnedTags.RemoveTag(FChronosTags::State_Shooting);
	}
}
