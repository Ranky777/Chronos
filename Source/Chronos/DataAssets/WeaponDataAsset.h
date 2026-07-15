// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/Texture2D.h"
#include "WeaponDataAsset.generated.h"

// TODO: 可以添加更多武器类型
UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	None,
	Pistol,
	Rifle,
	Shotgun,
	Unarmed
};

USTRUCT(BlueprintType)
struct FWeaponData
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float Damage = 1.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float FireRate = 0.5f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	int32 TotalAmmo = 12;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float BulletSpeed = 1000.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TSubclassOf<AActor> ProjectileClass;
	
	/** 武器类型 - 用于区分手枪/步枪/霰弹枪等 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	EWeaponType WeaponType = EWeaponType::Pistol;
	
	/** 武器骨骼网格 - 用于第一人称和第三人称显示 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TObjectPtr<USkeletalMesh> WeaponMesh;
	
	/** 武器插槽名称 - 武器附着到角色的Socket名称 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FName WeaponSocketName = FName("WeaponSocket");
	
	/** 枪口插槽名称 - 子弹发射位置的Socket名称 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FName MuzzleSocketName = FName("MuzzleSocket");
	
	/** 第一人称角色AnimBlueprint - 如果使用第一人称视角 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animations")
	TSubclassOf<UAnimInstance> FPAnimBlueprint;
	
	/** 第一人称射击动画蒙太奇 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animations")
	TObjectPtr<UAnimMontage> FPFireMontage;

	/** 第一人称装备动画蒙太奇 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animations")
	TObjectPtr<UAnimMontage> FPEquipMontage;
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
	/**
	 * 获取武器类型
	 * @return 当前武器类型
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	EWeaponType GetWeaponType() const { return WeaponData.WeaponType; }
	
	/**
	 * 获取武器网格
	 * @return 武器骨骼网格指针
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	USkeletalMesh* GetWeaponMesh() const { return WeaponData.WeaponMesh; }

	/**
	 * 获取武器插槽名称
	 * @return 插槽名称
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	FName GetWeaponSocketName() const { return WeaponData.WeaponSocketName; }

	/**
	 * 获取枪口插槽名称
	 * @return 枪口插槽名称
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	FName GetMuzzleSocketName() const { return WeaponData.MuzzleSocketName; }
	
	// 新增：获取武器总子弹数
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	int32 GetTotalAmmo() const { return WeaponData.TotalAmmo; }
	
	/**
	 * 获取第一人称AnimBlueprint
	 * @return AnimBlueprint类
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	TSubclassOf<UAnimInstance> GetFPAnimBlueprint() const { return WeaponData.FPAnimBlueprint; }
	
	/**
 * 获取装备动画蒙太奇
 * @return 装备动画蒙太奇
 */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	UAnimMontage* GetEquipMontage() const { return WeaponData.FPEquipMontage; }
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Data")
	FWeaponData WeaponData;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Data")
	FText WeaponName;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Data")
	TObjectPtr<UTexture2D> WeaponIcon;
};
