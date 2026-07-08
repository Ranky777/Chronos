// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "WeaponUser.generated.h"

class UWeaponDataAsset;

UINTERFACE(MinimalAPI, BlueprintType)
class UWeaponUser : public UInterface
{
	GENERATED_BODY()
};

class CHRONOS_API IWeaponUser
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Weapon")
	void EquipWeapon(UWeaponDataAsset* WeaponData);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Weapon")
	UWeaponDataAsset* GetCurrentWeapon() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Weapon")
	void Fire();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Weapon")
	bool CanFire() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Weapon")
	FVector GetMuzzleLocation() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Weapon")
	FRotator GetMuzzleRotation() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Weapon")
	void ThrowWeapon();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Weapon")
	bool CanThrow() const;
};
