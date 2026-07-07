// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Chronos/DataAssets/EnemyDataAsset.h"
#include "EnemyCharacter.generated.h"

class UHealthComponent;
class UWeaponComponent;

UCLASS()
class CHRONOS_API AEnemyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AEnemyCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	/**
	 * 获取生命值组件
	 * @return 生命值组件指针
	 */
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	UHealthComponent* GetHealthComponent() const { return HealthComponent; }

	/**
	 * 获取武器组件
	 * @return 武器组件指针
	 */
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	UWeaponComponent* GetWeaponComponent() const { return WeaponComponent; }
	
	/**
	 * 检查敌人是否存活
	 * @return 如果存活返回true，否则返回false
	 */
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	bool IsAlive() const;

	/**
	 * 敌人死亡处理
	 * 停止AI、禁用武器、触发死亡效果
	 * @param Killer 杀死敌人的Actor
	 */
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void OnEnemyDeath(AActor* Killer);

	/**
	 * 获取敌人数据资产
	 * @return 敌人数据资产指针
	 */
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	UEnemyDataAsset* GetEnemyDataAsset() const { return EnemyDataAsset; }

	/**
	 * 获取敌人数据配置
	 * @return 敌人数据结构
	 */
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	const FEnemyData& GetEnemyData() const { return EnemyDataAsset ? EnemyDataAsset->EnemyData : DefaultEnemyData; }

protected:
	void BindDeathEvent();
	
	/**
	 * 应用敌人数据资产配置
	 * 将数据资产中的配置应用到角色属性
	 */
	void ApplyEnemyData();
	
private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UHealthComponent> HealthComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWeaponComponent> WeaponComponent;

	/** 敌人数据资产 - 配置敌人属性 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UEnemyDataAsset> EnemyDataAsset;

	/** 默认敌人数据 - 当没有配置数据资产时使用 */
	FEnemyData DefaultEnemyData;

	bool bIsDead = false;
};
