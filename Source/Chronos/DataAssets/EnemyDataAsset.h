// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EnemyDataAsset.generated.h"


class UWeaponDataAsset;

USTRUCT(BlueprintType)
struct FEnemyData
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	float ShootingFrequency = 2.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	float MovementSpeed = 150.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	float DetectionRange = 2000.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	float AttackRange = 1500.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	TObjectPtr<UWeaponDataAsset> WeaponDataAsset;
};


/**
 * 
 */
UCLASS()
class CHRONOS_API UEnemyDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	UEnemyDataAsset();
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy Data")
	FEnemyData EnemyData;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy Data")
	FText EnemyName;
};
