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
	virtual void OnInteract_Implementation(AActor *Interactor) override;
	virtual bool CanInteract_Implementation() const override;
	virtual FText GetInteractionName_Implementation() const override;

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void SetWeaponDataAsset(UWeaponDataAsset *WeaponData);

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void SetThrower(AActor *InThrower);

protected:
	void GiveWeaponToReceiver(AActor *Receiver);
	void DestroyPickup();

	/**
	 * 处理武器撞击伤害
	 * 当武器以足够速度撞击可伤害目标时触发
	 */
	void HandleImpactDamage(AActor *HitActor);

private:
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent *OverlappedComp, AActor *OtherActor,
						UPrimitiveComponent *OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult &SweepResult);

	/**
	 * 物理碰撞回调 - 用于检测武器撞击
	 */
	UFUNCTION()
	void OnCollisionHit(UPrimitiveComponent *HitComponent, AActor *OtherActor,
						UPrimitiveComponent *OtherComp, FVector NormalImpulse, const FHitResult &Hit);

	/**
	 * 启用拾取功能（延迟后调用）
	 */
	void EnablePickup();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> MeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWeaponDataAsset> WeaponDataAsset;

	/**
	 * 撞击伤害速度阈值
	 * 武器速度超过此值时才会造成伤害
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	float DamageSpeedThreshold = 150.0f;

	/**
	 * 拾取延迟时间（秒）- 武器生成后需要等待这段时间才能被拾取
	 * 防止玩家立即拾取刚扔出的武器
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	float PickupDelay = 0.5f;

	/**
	 * 是否已经造成过伤害（防止一次投掷多次伤害）
	 */
	bool bHasDealtDamage = false;
	bool bHasBeenPickedUp = false;
	bool bIsPickupEnabled = false;

	TObjectPtr<AActor> Thrower;

	FTimerHandle PickupDelayTimerHandle;
};
