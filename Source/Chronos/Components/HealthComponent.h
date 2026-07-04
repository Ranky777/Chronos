// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Chronos/Interfaces/Damageable.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"

/**
 * 伤害事件委托 - 当角色受到伤害时触发
 * @param DamageAmount 伤害值
 * @param DamageCauser 造成伤害的Actor
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDamageTaken, float, DamageAmount, AActor*, DamageCauser);

/**
 * 死亡事件委托 - 当角色死亡时触发
 * @param Killer 杀死角色的Actor
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDeath, AActor*, Killer);

UCLASS(ClassGroup=(Chronos), meta=(BlueprintSpawnableComponent))
class CHRONOS_API UHealthComponent : public UActorComponent, public IDamageable
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UHealthComponent();
	
protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	
public:
	virtual void TakeDamage_Implementation(float DamageAmount, AActor* DamageCauser) override;
	virtual bool IsAlive_Implementation() const override;
	virtual void Die_Implementation(AActor* Killer) override;
	
	/**
	* 重置生命状态
	* 移除死亡标签，恢复存活状态
	*/
	UFUNCTION(BlueprintCallable, Category = "Health")
	void Revive();
	
	/**
	 * 检查角色是否已死亡
	 * @return 如果角色已死亡返回true，否则返回false
	 */
	UFUNCTION(BlueprintCallable, Category = "Health")
	bool IsDead() const;

	/**
	 * 检查是否有指定的GameplayTag
	 * @param Tag 要检查的标签
	 * @return 如果有该标签返回true，否则返回false
	 */
	UFUNCTION(BlueprintCallable, Category = "Health")
	bool HasGameplayTag(const FGameplayTag& Tag) const;

	/**
	 * 添加GameplayTag
	 * @param Tag 要添加的标签
	 */
	UFUNCTION(BlueprintCallable, Category = "Health")
	void AddGameplayTag(const FGameplayTag& Tag);

	/**
	 * 移除GameplayTag
	 * @param Tag 要移除的标签
	 */
	UFUNCTION(BlueprintCallable, Category = "Health")
	void RemoveGameplayTag(const FGameplayTag& Tag);
	
public:
	/** 伤害事件 - 角色受到伤害时触发 */
	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnDamageTaken OnDamageTaken;

	/** 死亡事件 - 角色死亡时触发 */
	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnDeath OnDeath;
	
private:
	/** 存储角色状态的GameplayTag容器 */
	FGameplayTagContainer OwnedTags;
};
