#pragma once

#include "Characters/ChronosCharacter.h"
#include "ChronosEnemy.generated.h"

class AChronosWeapon;
class UWeaponDataAsset;

/**
 * 敌人基类：出生自动装备武器、无相机瞄准回退（朝目标）、注册到 GameMode。
 * 死亡表现（掉武器/布娃娃）由 AChronosCharacter::HandleDeath 统一处理。
 */
UCLASS(Blueprintable, Config = Game)
class CHRONOS_NEW_API AChronosEnemy : public AChronosCharacter
{
	GENERATED_BODY()

public:
	AChronosEnemy();

	virtual void BeginPlay() override;
	virtual FVector GetAimDirection() const override;

	/**
	 * StateTree/蓝图在锁定目标时调用：无相机瞄准与扔枪的方向都基于它。
	 * 同类（另一个 AChronosEnemy）会被拒绝 —— 敌人不会把同伴设为交战目标。
	 */
	UFUNCTION(BlueprintCallable, Category = "Chronos|AI")
	void SetCombatTarget(AActor* NewTarget)
	{
		if (NewTarget && NewTarget->IsA<AChronosEnemy>())
		{
			return;
		}
		CombatTarget = NewTarget;
	}

	UFUNCTION(BlueprintPure, Category = "Chronos|AI")
	AActor* GetCombatTarget() const { return CombatTarget.Get(); }

	/**
	 * 从 AIController 的感知组件解析当前视觉目标。
	 * StateTree 的蓝图任务/条件上下文里世界上下文不可用（GetPlayerPawn 等返回 None），
	 * 目标一律经此实例函数解析，避免依赖调用方的世界上下文。
	 */
	UFUNCTION(BlueprintPure, Category = "Chronos|AI")
	AActor* GetPerceivedTarget() const;

	/**
	 * 对 CombatTarget 的开火视线判定：视锥 + 多条垂直射线，任一射线畅通即有视线。
	 * StateTree 进入条件的蓝图上下文拿不到实例数据（Character 输入为空），
	 * 视线门控由任务侧调用本函数完成。
	 */
	UFUNCTION(BlueprintPure, Category = "Chronos|AI")
	bool HasLineOfSightToCombatTarget() const;

	/** 视线检查视锥半角（度），与模板 StateTree 条件参数对齐 */
	UPROPERTY(EditDefaultsOnly, Category = "Chronos|AI")
	float LineOfSightConeAngle = 35.f;

	/**
	 * 开始持续射击：按当前武器的射速定时连发，直到 StopShooting。
	 * StateTree 任务的 EnterState 调用它，ExitState 调用 StopShooting。
	 * （模板原先每进入一次状态只打一发，重父类后 StopShooting 丢失导致任务编译失败，
	 *   这里下沉到 C++ 并顺带补上连发节奏。）
	 */
	UFUNCTION(BlueprintCallable, Category = "Chronos|AI")
	void StartShooting();

	/** 停止射击：清除连发计时器 */
	UFUNCTION(BlueprintCallable, Category = "Chronos|AI")
	void StopShooting();

	/** 视线检查的垂直射线条数，与模板 StateTree 条件参数对齐 */
	UPROPERTY(EditDefaultsOnly, Category = "Chronos|AI")
	int32 NumberOfVerticalLineOfSightChecks = 5;

	/** 出生自动装备的武器数据；为空则作为徒手占位敌人生成 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chronos|AI")
	TObjectPtr<UWeaponDataAsset> EnemyWeaponData;

	/**
	 * 敌人开火间隔（秒）。与玩家共用同一份武器数据，难度差异放在 AI 侧而不是复制数据资产 ——
	 * "手枪"就是"手枪"，敌人拿的和你捡的是同一把枪，只是 AI 打得更慢。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chronos|AI", meta = (ClampMin = "0.05"))
	float FireInterval = 1.f;

	/**
	 * 敌人出生武器的初始弹药。0 = 沿用武器数据资产的配置。
	 * 只想让敌人弹药更少时用它，不必为此再建一份"敌人版"武器数据。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chronos|AI", meta = (ClampMin = "0"))
	int32 StartingAmmoOverride = 3;

	/**
	 * 出生自动生成的武器 Actor 类。
	 * 必须指向带 FP/TP 视觉网格的蓝图武器（如 BP_ShooterWeapon_Pistol），
	 * 否则敌人手里的枪不会显示（C++ 的物理网格在装备态会被隐藏，只留枪口解算用）。
	 *
	 * 全局默认值由 DefaultEngine.ini 的 [/Script/Chronos_New.ChronosEnemy] 提供，
	 * 具体敌人蓝图（如 BP_ShooterNPC）可在类默认值里逐类型覆写。
	 *
	 * EditAnywhere：允许在关卡里逐个敌人改成不同的枪（比如一个拿步枪的敌人），
	 * 不必为此新建蓝图子类。武器数据会跟着这个类走，见 SpawnDefaultWeapon。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, config, Category = "Chronos|AI")
	TSoftClassPtr<AChronosWeapon> DefaultWeaponClass;

protected:
	/** 生成并装备 EnemyWeaponData 对应武器，蓝图可覆写扩展敌人类型 */
	UFUNCTION(BlueprintNativeEvent, Category = "Chronos|AI")
	void SpawnDefaultWeapon();

private:
	/** 连发节奏：打一发并安排下一发 */
	void FireOneShot();

	TWeakObjectPtr<AActor> CombatTarget;

	bool bIsShooting = false;
	FTimerHandle RefireTimerHandle;
};
