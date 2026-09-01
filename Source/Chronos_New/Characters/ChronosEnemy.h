#pragma once

#include "Characters/ChronosCharacter.h"
#include "ChronosEnemy.generated.h"

class UWeaponDataAsset;

/**
 * 敌人基类：出生自动装备武器、无相机瞄准回退（朝目标）、注册到 GameMode。
 * 死亡表现（掉武器/布娃娃）由 AChronosCharacter::HandleDeath 统一处理。
 */
UCLASS(Blueprintable)
class CHRONOS_NEW_API AChronosEnemy : public AChronosCharacter
{
	GENERATED_BODY()

public:
	AChronosEnemy();

	virtual void BeginPlay() override;
	virtual FVector GetAimDirection() const override;

	/** StateTree/蓝图在锁定目标时调用：无相机瞄准与扔枪的方向都基于它 */
	UFUNCTION(BlueprintCallable, Category = "Chronos|AI")
	void SetCombatTarget(AActor* NewTarget) { CombatTarget = NewTarget; }

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

	/** 视线检查的垂直射线条数，与模板 StateTree 条件参数对齐 */
	UPROPERTY(EditDefaultsOnly, Category = "Chronos|AI")
	int32 NumberOfVerticalLineOfSightChecks = 5;

	/** 出生自动装备的武器数据；为空则作为徒手占位敌人生成 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chronos|AI")
	TObjectPtr<UWeaponDataAsset> EnemyWeaponData;

protected:
	/** 生成并装备 EnemyWeaponData 对应武器，蓝图可覆写扩展敌人类型 */
	UFUNCTION(BlueprintNativeEvent, Category = "Chronos|AI")
	void SpawnDefaultWeapon();

private:
	TWeakObjectPtr<AActor> CombatTarget;
};
