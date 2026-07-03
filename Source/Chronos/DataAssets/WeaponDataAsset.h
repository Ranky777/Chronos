// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/Texture2D.h"
#include "WeaponDataAsset.generated.h"

USTRUCT(BlueprintType)
struct FWeaponData
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float Damage = 1.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float FireRate = 0.5f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	int32 MagazineSize = 12;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float ReloadTime = 1.5f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float BulletSpeed = 1000.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TSubclassOf<AActor> ProjectileClass;
};

/**
 * 
 */
UCLASS()
class CHRONOS_API UWeaponDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	UWeaponDataAsset();
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Data")
	FWeaponData WeaponData;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Data")
	FText WeaponName;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Data")
	TObjectPtr<UTexture2D> WeaponIcon;
};
