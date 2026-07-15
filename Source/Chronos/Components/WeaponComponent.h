// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Chronos/DataAssets/WeaponDataAsset.h"
#include "Chronos/Interfaces/WeaponUser.h"
#include "Components/ActorComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "WeaponComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFired);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponChanged, UWeaponDataAsset *, NewWeapon);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWeaponThrown);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAnimBlueprintChanged, TSubclassOf<UAnimInstance>, NewAnimBlueprint);

UCLASS(ClassGroup = (Chronos), meta = (BlueprintSpawnableComponent))
class CHRONOS_API UWeaponComponent : public UActorComponent, public IWeaponUser
{
	GENERATED_BODY()

public:
	UWeaponComponent();

	virtual void EquipWeapon_Implementation(UWeaponDataAsset *WeaponData) override;
	virtual UWeaponDataAsset *GetCurrentWeapon_Implementation() const override;
	virtual void Fire_Implementation() override;
	virtual bool CanFire_Implementation() const override;
	virtual FVector GetMuzzleLocation_Implementation() const override;
	virtual FRotator GetMuzzleRotation_Implementation() const override;
	virtual void ThrowWeapon_Implementation() override;
	virtual bool CanThrow_Implementation() const override;

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	float GetFireRate() const;

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool IsShooting() const;

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool HasWeapon() const;

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void DropCurrentWeapon(const FVector &DropLocation, const FVector &DropVelocity = FVector::ZeroVector);

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	USceneComponent *GetMuzzleComponent() const { return MuzzleComponent; }

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	int32 GetRemainingAmmo() const { return RemainingAmmo; };

protected:
	virtual void BeginPlay() override;

private:
	void SpawnProjectile() const;
	void UpdateWeaponState();

public:
	UPROPERTY(BlueprintAssignable, Category = "Weapon")
	FOnFired OnFired;

	UPROPERTY(BlueprintAssignable, Category = "Weapon")
	FOnWeaponChanged OnWeaponChanged;

	UPROPERTY(BlueprintAssignable, Category = "Weapon")
	FOnWeaponThrown OnWeaponThrown;
	
	UPROPERTY(BlueprintAssignable, Category = "Weapon")
	FOnAnimBlueprintChanged OnAnimBlueprintChanged;

private:
	UPROPERTY(EditAnywhere, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWeaponDataAsset> CurrentWeaponData = nullptr;

	int32 RemainingAmmo = 0;

	float LastFireTime = 0.0f;
	FGameplayTagContainer OwnedTags;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> MuzzleComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> WeaponMeshComponent;

	UPROPERTY(EditAnywhere, Category = "Weapon", meta = (ClampMin = 100.0f, ClampMax = 2000.0f))
	float ThrowForce = 500.0f;

	bool bIsThrowing = false;
};
