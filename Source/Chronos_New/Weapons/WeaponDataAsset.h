#pragma once

#include "Engine/DataAsset.h"
#include "WeaponDataAsset.generated.h"

class AChronosProjectile;
class UAnimMontage;
class USkeletalMesh;

/**
 * 武器配置数据资产：策划在编辑器中为每把武器建一个实例（DA_Pistol 等），
 * 武器 Actor 与战斗组件的全部数值/引用均由它驱动，代码不硬编码任何武器差异。
 */
UCLASS(BlueprintType)
class CHRONOS_NEW_API UWeaponDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display")
	FText WeaponName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display")
	TObjectPtr<USkeletalMesh> WeaponMesh;

	/** 枪口插槽名，开火位置与方向基于它计算 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName MuzzleSocketName = TEXT("Muzzle");

	/** 手持时武器附加到角色骨骼网格的插槽名 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName HandSocketName = TEXT("hand_rWeaponSocket");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0"))
	int32 AmmoCount = 8;

	/** 每秒最大射速（半自动下受点击频率限制） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0.1"))
	float FireRate = 5.f;

	/** 全自动按住连发 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	bool bAutomatic = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	float BulletSpeed = 6000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<AChronosProjectile> ProjectileClass;

	/** 投掷初速 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throw", meta = (ClampMin = "0"))
	float ThrowSpeed = 1500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Anim")
	TObjectPtr<UAnimMontage> FireMontage;
};
