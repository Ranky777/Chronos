#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HAL/PlatformTime.h"
#include "Interactables/Interactable.h"
#include "Weapons/WeaponUser.h"
#include "ChronosWeapon.generated.h"

class AChronosCharacter;
class USkeletalMeshComponent;
class UWeaponDataAsset;

/** 武器的两种存在状态，替代布尔标记 */
UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	/** 被角色持有，附加在手上 */
	Equipped,
	/** 处于世界中（地面拾取物或飞行中的投掷物），开启物理模拟 */
	InWorld
};

/**
 * 武器 Actor：同时承担"手持武器"与"地面/投掷拾取物"两种形态。
 * 实现 IWeaponUser 接口，供 CombatComponent 统一调用。
 */
UCLASS(Blueprintable)
class CHRONOS_NEW_API AChronosWeapon : public AActor, public IInteractable, public IWeaponUser
{
	GENERATED_BODY()

public:
	AChronosWeapon();

	/** 由数据资产初始化武器。每次从池外生成新武器实例后必须先调用 */
	UFUNCTION(BlueprintCallable, Category = "Chronos|Weapon")
	void InitFromData(UWeaponDataAsset* Data);

	//~ IInteractable
	virtual void OnInteract(AChronosCharacter* Interactor) override;
	virtual bool CanInteract(const AChronosCharacter* Interactor) const override;
	virtual FText GetInteractionText() const override;

	//~ IWeaponUser （BlueprintNativeEvent 接口函数以 _Implementation 后缀实现）
	virtual void Fire_Implementation() override;
	virtual void StopFiring_Implementation() override;
	virtual int32 GetCurrentAmmo_Implementation() override;
	virtual int32 GetMaxAmmo_Implementation() override;
	virtual FName GetMuzzleSocketName_Implementation() override;
	virtual FVector GetMuzzleLocation_Implementation() override;
	virtual bool CanFire_Implementation() override;
	virtual void ThrowWeapon_Implementation(const FVector& Direction) override;
	virtual void DropWeapon_Implementation() override;

	/** 向指定目标点开火（旧签名兼容层，内部转发接口） */
	UFUNCTION(BlueprintCallable, Category = "Chronos|Weapon")
	bool FireAtTarget(const FVector& TargetPoint);

	UFUNCTION(BlueprintPure, Category = "Chronos|Weapon")
	EWeaponState GetWeaponState() const { return WeaponState; }

	/** 弹药变化委托（蓝图可绑定，实现 IWeaponUser 接口要求） */
	UPROPERTY(BlueprintAssignable, Category = "Chronos|Weapon")
	FOnWeaponAmmoChanged OnAmmoChanged;

	/** 装备到角色手上（由战斗组件调用） */
	void EquipTo(AChronosCharacter* NewHolder);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USkeletalMeshComponent> WeaponMeshComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chronos|Weapon")
	TObjectPtr<UWeaponDataAsset> WeaponData;

protected:
	virtual void BeginPlay() override;

	/** 开火表现钩子：蓝图播放 FireMontage/音效/枪口 FX。C++ 已完成弹道、弹药与冷却 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Chronos|Weapon")
	void OnFireVisuals();

	/** 入手表现钩子：蓝图播放装备动画/音效 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Chronos|Weapon")
	void OnEquipVisuals();

	virtual void Tick(float DeltaSeconds) override;

private:
	void EnterWorldState(const FVector& InitialVelocity);
	void EnterEquippedState();
	void UpdateRestState();
	void Throw(const FVector& Direction, AChronosCharacter* Thrower);
	void Drop(AChronosCharacter* Dropper);

	UFUNCTION()
	void OnWeaponHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		FVector NormalImpulse, const FHitResult& Hit);

	/** 投掷物与世界状态下的角色重叠：速度超过阈值即一击必杀 */
	UFUNCTION()
	void OnWeaponBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** 投掷命中即死的最低速度 */
	UPROPERTY(EditDefaultsOnly, Category = "Chronos|Weapon", meta = (ClampMin = "0"))
	float ThrowKillSpeedThreshold = 600.f;

	int32 RemainingAmmo = 0;
	float RefireCooldown = 0.f;
	double LastFireRealTime = 0.0;

	/** 进入世界状态后的最短静止观察时间；期间不可拾取，防止瞬间捡回刚扔的武器 */
	float MinPickupDelay = 0.5f;
	bool bAtRest = false;
	double LastWorldEnterRealTime = 0.0;

	EWeaponState WeaponState = EWeaponState::InWorld;

	/** 最近一次投掷者，用于投掷过程中免疫自伤 */
	TWeakObjectPtr<AChronosCharacter> LastThrower;
};