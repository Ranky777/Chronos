#pragma once

#include "Components/ActorComponent.h"
#include "Weapons/WeaponUser.h"
#include "CombatComponent.generated.h"

class AChronosCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponChanged, TScriptInterface<IWeaponUser>, OldWeapon, TScriptInterface<IWeaponUser>, NewWeapon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCombatAmmoChanged, int32, RemainingAmmo, int32, MaxAmmo);

/**
 * 单武器槽战斗组件 —— 现在持有 IWeaponUser 接口，完全不依赖具体 Actor 类。
 */
UCLASS(ClassGroup = (Chronos), meta = (BlueprintSpawnableComponent))
class CHRONOS_NEW_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatComponent();

	/** 拾取/更换武器。若已持有武器则旧武器被丢到世界 */
	UFUNCTION(BlueprintCallable, Category = "Chronos|Combat")
	void EquipWeapon(TScriptInterface<IWeaponUser> NewWeapon);

	/** 向视线方向投掷当前武器 */
	UFUNCTION(BlueprintCallable, Category = "Chronos|Combat")
	void ThrowCurrentWeapon(const FVector& Direction);

	/** 丢弃当前武器 */
	UFUNCTION(BlueprintCallable, Category = "Chronos|Combat")
	void DropCurrentWeapon();

	UFUNCTION(BlueprintPure, Category = "Chronos|Combat")
	TScriptInterface<IWeaponUser> GetCurrentWeapon() const { return CurrentWeapon; }

	UPROPERTY(BlueprintAssignable, Category = "Chronos|Combat")
	FOnWeaponChanged OnWeaponChanged;

	/**
	 * 当前武器弹药变化（对外转发版，HUD 订阅它）。
	 * 武器的 OnAmmoChanged 只广播给持有者内部，外部观察者用这个。
	 */
	UPROPERTY(BlueprintAssignable, Category = "Chronos|Combat")
	FOnCombatAmmoChanged OnAmmoChanged;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleAmmoChanged(int32 RemainingAmmo, int32 MaxAmmo);

	UPROPERTY()
	TScriptInterface<IWeaponUser> CurrentWeapon;
};
