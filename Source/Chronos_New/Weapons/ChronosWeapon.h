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

	/**
	 * 覆写剩余弹药。敌人出生时按难度给一个与数据资产不同的初始值，
	 * 这样"敌人版武器"不需要再复制一份数据资产 —— 武器属性与 AI 难度是两回事。
	 */
	UFUNCTION(BlueprintCallable, Category = "Chronos|Weapon")
	void SetRemainingAmmo(int32 NewAmmo);

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

	/** 播放数据资产上配置的开火音效；未配置则无声 */
	void PlayFireSound();

	/**
	 * 入手表现钩子：把模板的 FP/TP 武器网格挂到角色骨骼、
	 * 切换武器动画实例（ABP_FP_Pistol / ABP_TP_Pistol 等）、播放装备蒙太奇。
	 * 持有者可从 GetOwner() 取得（C++ 在调用前已 SetOwner）。
	 *
	 * C++ 提供默认实现，动画实例取自 WeaponData 的 FirstPerson/ThirdPersonAnimClass。
	 * 这样裸 AChronosWeapon（不带任何蓝图逻辑）也能正确表现 —— 敌人若因配置丢失
	 * 生成了裸武器，玩家拾取后依然会切换持枪 ABP。蓝图可覆写以加入自己的表现。
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Chronos|Weapon")
	void OnEquipVisuals();

	/**
	 * 脱手表现钩子（投掷 / 丢弃 / 死亡掉落）：收起 FP/TP 武器网格并把
	 * PreviousHolder 的动画实例还原为无武器姿态。
	 * C++ 默认实现与 OnEquipVisuals 对称，蓝图可覆写。
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Chronos|Weapon")
	void OnUnequipVisuals(AChronosCharacter* PreviousHolder);

	virtual void Tick(float DeltaSeconds) override;

private:
	void EnterWorldState(const FVector& InitialVelocity);
	void EnterEquippedState();
	void UpdateRestState();
	void Throw(const FVector& Direction, AChronosCharacter* Thrower);
	void Drop(AChronosCharacter* Dropper);

	/**
	 * 显隐蓝图提供的视觉网格（模板的 FP_Weapon / TP_Weapon）。
	 * C++ 不硬编码组件名：除自身的 WeaponMeshComponent 外一律视为视觉网格。
	 * 装备态显示它们、世界态隐藏它们，避免与物理网格重复渲染成"两把枪"。
	 *
	 * @return 受影响���视觉网格数量；为 0 表示这把武器没有视觉网格（裸 C++ 武器）
	 */
	int32 SetVisualMeshesHidden(bool bShouldHide);

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

	/**
	 * 投掷者对自身的免疫窗口（真实秒）。武器出手瞬间仍与投掷者胶囊重叠，
	 * 这段窗口内一律不结算对投掷者的伤害。
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Chronos|Weapon", meta = (ClampMin = "0"))
	float ThrowerImmunityDuration = 0.35f;

	/** 当前持有者：投掷/丢弃时决定自伤免疫对象与表现还原目标 */
	TWeakObjectPtr<AChronosCharacter> CurrentHolder;

	/**
	 * 进入世界状态后是否已经撞击过环境。
	 * 未撞击前投掷者永久免疫（武器刚出手、仍在身边）；撞击后免疫只剩时间窗口，
	 * 于是"扔出去撞墙弹回来砸中自己"依然成立。
	 */
	bool bHasHitWorldSinceThrown = false;

	int32 RemainingAmmo = 0;
	float RefireCooldown = 0.f;
	double LastFireRealTime = 0.0;

	/**
	 * 玩家是否正按住开火键。全自动武器在 Tick 里据此按射速持续开火，
	 * 半自动武器只用它在"松开前不重复触发"上（按一次打一发）。
	 */
	bool bIsFiring = false;

	/** 进入世界状态后的最短静止观察时间；期间不可拾取，防止瞬间捡回刚扔的武器 */
	float MinPickupDelay = 0.5f;
	bool bAtRest = false;

	/**
	 * 脱手后的阻尼。武器网格没有 PhysicsAsset，物理引擎会退化成简单形状，
	 * 没阻尼的话会像球一样一直滚、一直转，玩家很难瞄准拾取。
	 * 角阻尼给大一些：SUPERHOT 里丢出去的枪应该很快定住，方便下一秒捡起再扔。
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Chronos|Weapon|Physics")
	float LinearDamping = 0.8f;

	UPROPERTY(EditDefaultsOnly, Category = "Chronos|Weapon|Physics")
	float AngularDamping = 6.f;

	/** 判定"静止"的角速度阈值（度/秒）：低于此值且线速度也够小才算停稳 */
	UPROPERTY(EditDefaultsOnly, Category = "Chronos|Weapon|Physics")
	float RestAngularSpeedDeg = 30.f;
	double LastWorldEnterRealTime = 0.0;

	EWeaponState WeaponState = EWeaponState::InWorld;

	/** 最近一次投掷者，用于投掷过程中免疫自伤 */
	TWeakObjectPtr<AChronosCharacter> LastThrower;
};