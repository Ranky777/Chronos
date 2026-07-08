// Fill out your copyright notice in the Description page of Project Settings.

#include "WeaponPickup.h"

#include "Chronos/DataAssets/WeaponDataAsset.h"
#include "Chronos/Interfaces/WeaponUser.h"
#include "Components/SphereComponent.h"
#include "Components/SkeletalMeshComponent.h"

AWeaponPickup::AWeaponPickup()
{
	PrimaryActorTick.bCanEverTick = false;
	
	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	CollisionComponent->SetSphereRadius(80.0f);
	CollisionComponent->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &AWeaponPickup::OnOverlapBegin);
	RootComponent = CollisionComponent;
	
	MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	MeshComponent->SetSimulatePhysics(true);
}

void AWeaponPickup::BeginPlay()
{
	Super::BeginPlay();
	
	if (WeaponDataAsset && WeaponDataAsset->WeaponData.WeaponMesh)
	{
		MeshComponent->SetSkeletalMesh(WeaponDataAsset->WeaponData.WeaponMesh);
	}
}

void AWeaponPickup::OnInteract_Implementation(AActor* Interactor)
{
	if (bHasBeenPickedUp || !WeaponDataAsset)
	{
		return;
	}
	
	bHasBeenPickedUp = true;
	
	GiveWeaponToReceiver(Interactor);
	
	DestroyPickup();
}

bool AWeaponPickup::CanInteract_Implementation() const
{
	return !bHasBeenPickedUp && WeaponDataAsset;
}

FText AWeaponPickup::GetInteractionName_Implementation() const
{
	if (WeaponDataAsset)
	{
		return WeaponDataAsset->WeaponName;
	}
	
	return FText::FromString(TEXT("拾取武器"));
}

void AWeaponPickup::SetWeaponDataAsset(UWeaponDataAsset* WeaponData)
{
	WeaponDataAsset = WeaponData;
	
	if (WeaponDataAsset && WeaponDataAsset->WeaponData.WeaponMesh && MeshComponent)
	{
		MeshComponent->SetSkeletalMesh(WeaponDataAsset->WeaponData.WeaponMesh);
	}
}

void AWeaponPickup::GiveWeaponToReceiver(AActor* Receiver)
{
	if (!Receiver || !WeaponDataAsset)
	{
		return;
	}
	
	if (IWeaponUser* WeaponUser = Cast<IWeaponUser>(Receiver))
	{
		WeaponUser->EquipWeapon(WeaponDataAsset);
	}
}

void AWeaponPickup::DestroyPickup()
{
	Destroy();
}

void AWeaponPickup::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, 
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bHasBeenPickedUp || !WeaponDataAsset)
	{
		return;
	}
	
	if (OtherActor && OtherActor != this)
	{
		if (IWeaponUser* WeaponUser = Cast<IWeaponUser>(OtherActor))
		{
			bHasBeenPickedUp = true;
			GiveWeaponToReceiver(OtherActor);
			DestroyPickup();
		}
	}
}
