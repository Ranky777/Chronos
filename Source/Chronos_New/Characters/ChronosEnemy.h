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
