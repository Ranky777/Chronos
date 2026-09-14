#pragma once

#include "Audio/ChronosSynthComponent.h"
#include "Engine/DataAsset.h"
#include "WeaponDataAsset.generated.h"

class AChronosProjectile;
class UAnimInstance;
class UAnimMontage;
class USkeletalMesh;
class USoundBase;

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

	/**
	 * 开火音效。留空则无声（不会报错）。
	 * 本项目音频资源极少，先保证"开火有声"这一最基本的反馈。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Audio")
	TSoftObjectPtr<USoundBase> FireSound;

	/**
	 * 开火用的程序化合成音效。
	 * 本项目没有音频文件资产（模板零音效、且无法导入），
	 * 所以 FireSound 为空时一律走这个合成器 —— 策划可直接在这里给每把枪挑音色。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Audio")
	EChronosSfx FireSfx = EChronosSfx::GunshotLight;

	/** 投掷初速 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throw", meta = (ClampMin = "0"))
	float ThrowSpeed = 1500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Anim")
	TObjectPtr<UAnimMontage> FireMontage;

	/** 入手/装备蒙太奇（第一人称手臂），可为空 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Anim")
	TObjectPtr<UAnimMontage> EquipMontage;

	/**
	 * 持枪时角色第一人称网格应切换到的动画实例。
	 * 这是 C++ 默认装备表现的动画来源 —— 裸 AChronosWeapon（无蓝图）也要能正确切换，
	 * 因此不放在蓝图变量上。蓝图武器可继续用 BP 侧变量覆盖。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Anim")
	TSubclassOf<UAnimInstance> FirstPersonAnimClass;

	/** 持枪时角色第三人称网格应切换到的动画实例 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Anim")
	TSubclassOf<UAnimInstance> ThirdPersonAnimClass;
};
