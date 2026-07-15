// Fill out your copyright notice in the Description page of Project Settings.

#include "WeaponPickup.h"

#include "Chronos/Characters/PlayerCharacter.h"
#include "Chronos/DataAssets/WeaponDataAsset.h"
#include "Chronos/Interfaces/Damageable.h"
#include "Chronos/Interfaces/WeaponUser.h"
#include "Components/SphereComponent.h"
#include "Components/SkeletalMeshComponent.h"

AWeaponPickup::AWeaponPickup()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	RootComponent = CollisionComponent;
	CollisionComponent->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	CollisionComponent->SetSphereRadius(50.0f);

	MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(CollisionComponent);
	MeshComponent->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	MeshComponent->SetSimulatePhysics(true);
	MeshComponent->SetEnableGravity(true);

	bHasBeenPickedUp = false;
	bHasDealtDamage = false;
	bIsPickupEnabled = false;
}

void AWeaponPickup::BeginPlay()
{
	Super::BeginPlay();

	FString ActorName = FString(GetName());
	FString ThrowerName = Thrower ? FString(Thrower->GetName()) : TEXT("Null");
	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Purple,
									 FString::Printf(TEXT("[WeaponPickup] BeginPlay - Name=%s, Thrower=%s"),
													 *ActorName, *ThrowerName));

	if (CollisionComponent)
	{
		CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &AWeaponPickup::OnOverlapBegin);
	}

	if (MeshComponent)
	{
		MeshComponent->OnComponentHit.AddDynamic(this, &AWeaponPickup::OnCollisionHit);
	}

	if (WeaponDataAsset && WeaponDataAsset->WeaponData.WeaponMesh)
	{
		MeshComponent->SetSkeletalMesh(WeaponDataAsset->WeaponData.WeaponMesh);
	}

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(
			PickupDelayTimerHandle,
			this,
			&AWeaponPickup::EnablePickup,
			PickupDelay,
			false);
	}

	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Purple,
									 FString::Printf(TEXT("[WeaponPickup] BeginPlay complete - PickupDelay=%f"),
													 PickupDelay));
}

void AWeaponPickup::OnInteract_Implementation(AActor *Interactor)
{
	if (bHasBeenPickedUp)
	{
		return;
	}

	GiveWeaponToReceiver(Interactor);

	DestroyPickup();
}

bool AWeaponPickup::CanInteract_Implementation() const
{
	return !bHasBeenPickedUp && bIsPickupEnabled && WeaponDataAsset;
}

FText AWeaponPickup::GetInteractionName_Implementation() const
{
	if (WeaponDataAsset)
	{
		return WeaponDataAsset->WeaponName;
	}

	return FText::GetEmpty();
}

void AWeaponPickup::SetWeaponDataAsset(UWeaponDataAsset *WeaponData)
{
	WeaponDataAsset = WeaponData;

	if (WeaponDataAsset && WeaponDataAsset->WeaponData.WeaponMesh && MeshComponent)
	{
		MeshComponent->SetSkeletalMesh(WeaponDataAsset->WeaponData.WeaponMesh);
	}
}

void AWeaponPickup::SetThrower(AActor *InThrower)
{
	Thrower = InThrower;
}

void AWeaponPickup::GiveWeaponToReceiver(AActor *Receiver)
{
	if (!Receiver || !WeaponDataAsset)
	{
		return;
	}

	if (IWeaponUser *WeaponUser = Cast<IWeaponUser>(Receiver))
	{
		WeaponUser->EquipWeapon(WeaponDataAsset);
	}
}

void AWeaponPickup::DestroyPickup()
{
	bHasBeenPickedUp = true;
	Destroy();
}

void AWeaponPickup::OnOverlapBegin(UPrimitiveComponent *OverlappedComp, AActor *OtherActor,
								   UPrimitiveComponent *OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult &SweepResult)
{
	if (bHasBeenPickedUp || !bIsPickupEnabled)
	{
		return;
	}

	if (!OtherActor)
	{
		return;
	}

	if (!OtherActor->IsA(APlayerCharacter::StaticClass()))
	{
		return;
	}

	GiveWeaponToReceiver(OtherActor);
	DestroyPickup();
}

void AWeaponPickup::HandleImpactDamage(AActor *HitActor)
{
	FString HitActorName = HitActor ? FString(HitActor->GetName()) : TEXT("Null");
	FString ThrowerName = Thrower ? FString(Thrower->GetName()) : TEXT("Null");

	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red,
									 FString::Printf(TEXT("[WeaponPickup] HandleImpactDamage - HitActor=%s, Thrower=%s"),
													 *HitActorName, *ThrowerName));

	if (!HitActor || !WeaponDataAsset)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red,
										 TEXT("[WeaponPickup] HandleImpactDamage: Invalid parameters"));
		return;
	}

	if (HitActor == Thrower)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green,
										 TEXT("[WeaponPickup] HandleImpactDamage: Hit actor is thrower, ignoring!"));
		return;
	}

	if (IDamageable *Damageable = Cast<IDamageable>(HitActor))
	{
		float Damage = WeaponDataAsset->WeaponData.Damage;
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red,
										 FString::Printf(TEXT("[WeaponPickup] HandleImpactDamage: Dealing %f damage to %s"),
														 Damage, *HitActorName));
		Damageable->TakeDamage(Damage, this);

		bHasDealtDamage = true;
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow,
										 FString::Printf(TEXT("[WeaponPickup] HandleImpactDamage: %s is not Damageable"),
														 *HitActorName));
	}
}

void AWeaponPickup::OnCollisionHit(UPrimitiveComponent *HitComponent, AActor *OtherActor,
								   UPrimitiveComponent *OtherComp, FVector NormalImpulse, const FHitResult &Hit)
{
	FString OtherActorName = OtherActor ? FString(OtherActor->GetName()) : TEXT("Null");

	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Magenta,
									 FString::Printf(TEXT("[WeaponPickup] OnCollisionHit - OtherActor=%s, bHasDealtDamage=%d"),
													 *OtherActorName, bHasDealtDamage));

	if (bHasDealtDamage)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow,
										 FString::Printf(TEXT("[WeaponPickup] OnCollisionHit: Already dealt damage, skipping")));
		return;
	}

	float CurrentSpeed = 0.0f;
	if (MeshComponent)
	{
		FVector Velocity = MeshComponent->GetPhysicsLinearVelocity();
		CurrentSpeed = Velocity.Size();
	}

	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Magenta,
									 FString::Printf(TEXT("[WeaponPickup] OnCollisionHit: Speed=%f, Threshold=%f"),
													 CurrentSpeed, DamageSpeedThreshold));

	if (CurrentSpeed < DamageSpeedThreshold)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow,
										 FString::Printf(TEXT("[WeaponPickup] OnCollisionHit: Speed too low, skipping")));
		return;
	}

	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Orange,
									 FString::Printf(TEXT("[WeaponPickup] OnCollisionHit: Calling HandleImpactDamage for %s"),
													 *OtherActorName));

	HandleImpactDamage(OtherActor);

	if (CurrentSpeed > DamageSpeedThreshold * 2)
	{
		FTimerHandle ResetTimer;
		GetWorld()->GetTimerManager().SetTimer(ResetTimer, [this]()
											   { bHasDealtDamage = false; }, 1.0f, false);
	}
}

void AWeaponPickup::EnablePickup()
{
	bIsPickupEnabled = true;
}
