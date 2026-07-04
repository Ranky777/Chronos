// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ProjectilePoolSubsystem.generated.h"

/**
 * 对象池子系统
 * 负责管理子弹的预分配、复用和回收
 */
UCLASS()
class CHRONOS_API UProjectilePoolSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()
	
public:
	UProjectilePoolSubsystem();
	
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	/** Tick 更新（用于清理超时的子弹） */
	virtual void Tick(float DeltaTime) override;
	
	virtual TStatId GetStatId() const override
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(UProjectilePoolSubsystem, STATGROUP_Tickables);
	}
	
	/**
	 * 从池中获取子弹
	 * @param ProjectileClass 子弹类
	 * @param SpawnTransform 生成位置和旋转
	 * @return 获取到的子弹Actor，如果池中没有可用子弹则生成新的
	 */
	UFUNCTION(BlueprintCallable, Category = "Projectile Pool")
	AActor* GetProjectile(TSubclassOf<AActor> ProjectileClass, const FTransform& SpawnTransform);
	
	/**
	 * 回收子弹到池中
	 * @param Projectile 要回收的子弹Actor
	 */
	UFUNCTION(BlueprintCallable, Category = "Projectile Pool")
	void ReturnProjectile(AActor* Projectile);
	
	/**
	* 预分配子弹
	* @param ProjectileClass 子弹类
	* @param Count 预分配数量
	*/
	UFUNCTION(BlueprintCallable, Category = "Projectile Pool")
	void PreallocateProjectiles(TSubclassOf<AActor> ProjectileClass, int32 Count);
	
	/**
	 * 获取池中可用子弹数量
	 * @param ProjectileClass 子弹类
	 * @return 可用子弹数量
	 */
	UFUNCTION(BlueprintCallable, Category = "Projectile Pool")
	int32 GetAvailableProjectileCount(TSubclassOf<AActor> ProjectileClass) const;
	
private:
	/** 存储不同类型子弹的对象池 */
	TMap<TSubclassOf<AActor>, TArray<TObjectPtr<AActor>>> ProjectilePoolMap;
	
	/** 存储已激活的子弹及其激活时间（用于超时清理） */
	TMap<TObjectPtr<AActor>, float> ActiveProjectiles;
	
	UPROPERTY(EditAnywhere, Category = "Projectile Pool", meta = (AllowPrivateAccess = "true"))
	int32 DefaultPreallocateCount = 20;
	
	UPROPERTY(EditAnywhere, Category = "Projectile Pool", meta = (AllowPrivateAccess = "true"))
	float MaxProjectileLifetime = 10.0f;
};
