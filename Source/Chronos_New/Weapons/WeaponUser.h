#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "WeaponUser.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponAmmoChanged, int32, RemainingAmmo, int32, MaxAmmo);

class UWeaponDataAsset;

UINTERFACE(MinimalAPI, BlueprintType)
class UWeaponUser : public UInterface
{
	GENERATED_BODY()
};

/**
 * 统一武器使用接口：C++ 只依赖这层，完全解耦具体武器蓝图实现。
 *
 * Variant_Shooter 的 BP_ShooterWeaponBase 及其子类（Pistol/Rifle/GrenadeLauncher）
 * 已包含以下现成能力，只需在蓝图里实现本接口并转调即可：
 *   - StartFiring / StopFiring
 *   - GetCurrentBullets / GetMagSize
 *   - GetMuzzleSocketName / GetMuzzleLocation
 *   - DeactivateWeapon / ActivateWeapon
 *   - OnAmmoChanged 委托
 */
class CHRONOS_NEW_API IWeaponUser
{
	GENERATED_BODY()

public:
	/** 开火一次（半自动）或开始连发（全自动） */
	UFUNCTION(BlueprintNativeEvent, Category = "Chronos|Weapon")
	void Fire();

	/** 停止连发 */
	UFUNCTION(BlueprintNativeEvent, Category = "Chronos|Weapon")
	void StopFiring();

	/** 当前弹药数 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Chronos|Weapon")
	int32 GetCurrentAmmo();

	/** 弹匣容量 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Chronos|Weapon")
	int32 GetMaxAmmo();

	/** 枪口插槽名 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Chronos|Weapon")
	FName GetMuzzleSocketName();

	/** 枪口世界位置 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Chronos|Weapon")
	FVector GetMuzzleLocation();

	/** 是否可开火（弹药>0 且冷却就绪） */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Chronos|Weapon")
	bool CanFire();

	/** 投掷武器 */
	UFUNCTION(BlueprintNativeEvent, Category = "Chronos|Weapon")
	void ThrowWeapon(const FVector& Direction);

	/** 丢弃武器（非攻击性） */
	UFUNCTION(BlueprintNativeEvent, Category = "Chronos|Weapon")
	void DropWeapon();
};