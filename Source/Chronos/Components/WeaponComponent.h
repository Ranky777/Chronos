// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Chronos/Interfaces/WeaponUser.h"
#include "Components/ActorComponent.h"
#include "WeaponComponent.generated.h"

/**
 * 射击事件委托 - 当武器射击时触发
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFired);

/**
 * 换弹开始事件委托
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnReloadStarted);

/**
 * 换弹完成事件委托
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnReloadCompleted);


UCLASS(ClassGroup=(Chronos), meta=(BlueprintSpawnableComponent))
class CHRONOS_API UWeaponComponent : public UActorComponent, public IWeaponUser
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UWeaponComponent();

	virtual void EquipWeapon_Implementation(UWeaponDataAsset* WeaponData) override;
	virtual UWeaponDataAsset* GetCurrentWeapon_Implementation() const override;
	virtual void Fire_Implementation() override;
	virtual void Reload_Implementation() override;
	virtual bool CanFire_Implementation() const override;
	virtual bool CanReload_Implementation() const override;
	virtual FVector GetMuzzleLocation_Implementation() const override;
	virtual FRotator GetMuzzleRotation_Implementation() const override;
	
	/**
	* 获取当前弹药数量
	*/
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	int32 GetCurrentAmmo() const { return CurrentAmmo; }

	/**
	 * 获取弹匣容量
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	int32 GetMagazineSize() const;

	/**
	 * 获取换弹时间
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	float GetReloadTime() const;

	/**
	 * 获取射速
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	float GetFireRate() const;

	/**
	 * 是否正在换弹
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool IsReloading() const;

	/**
	 * 是否正在射击
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool IsShooting() const;
	
protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	
private:
	/**
	 * 生成子弹
	 */
	void SpawnProjectile() const;

	/**
	 * 处理换弹完成
	 */
	void OnReloadTimerFinished();

	/**
	 * 更新GameplayTags状态
	 */
	void UpdateWeaponState();
	
public:
	UPROPERTY(BlueprintAssignable, Category = "Weapon")
	FOnFired OnFired;

	UPROPERTY(BlueprintAssignable, Category = "Weapon")
	FOnReloadStarted OnReloadStarted;

	UPROPERTY(BlueprintAssignable, Category = "Weapon")
	FOnReloadCompleted OnReloadCompleted;

private:
	/** 当前装备的武器数据 */
	UPROPERTY(EditAnywhere, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWeaponDataAsset> CurrentWeaponData;

	/** 当前弹药数量 */
	int32 CurrentAmmo = 0;

	/** 上次射击时间 */
	float LastFireTime = 0.0f;

	/** 换弹计时器 */
	FTimerHandle ReloadTimerHandle;

	/** 存储武器状态的GameplayTag容器 */
	FGameplayTagContainer OwnedTags;
};
