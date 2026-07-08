// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Chronos/Interfaces/Interactable.h"
#include "GameFramework/Actor.h"
#include "WeaponPickup.generated.h"

class UWeaponDataAsset;
class USphereComponent;
class USkeletalMeshComponent;

UCLASS()
class CHRONOS_API AWeaponPickup : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	AWeaponPickup();

protected:
	virtual void BeginPlay() override;

public:
	virtual void OnInteract_Implementation(AActor* Interactor) override;
	virtual bool CanInteract_Implementation() const override;
	virtual FText GetInteractionName_Implementation() const override;

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void SetWeaponDataAsset(UWeaponDataAsset* WeaponData);

protected:
	void GiveWeaponToReceiver(AActor* Receiver);
	void DestroyPickup();

private:
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, 
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> MeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWeaponDataAsset> WeaponDataAsset;

	bool bHasBeenPickedUp = false;
};
