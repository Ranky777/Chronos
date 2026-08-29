#include "Characters/ChronosEnemy.h"

#include "Components/CombatComponent.h"
#include "EngineUtils.h"
#include "GameFlow/ChronosGameMode.h"
#include "Weapons/ChronosWeapon.h"

AChronosEnemy::AChronosEnemy()
{
	// 基类 Tick 只做玩家准星焦点扫描，敌人无需
	PrimaryActorTick.bCanEverTick = false;
}

void AChronosEnemy::BeginPlay()
{
	Super::BeginPlay();

	if (EnemyWeaponData)
	{
		SpawnDefaultWeapon();
	}

	// 正常运行走 GetAuthGameMode；迭代器兜底兼容自动化测试等
	// "GameMode 是普通 SpawnActor 而非世界注册 GameMode" 的场景
	AChronosGameMode* GM = GetWorld()->GetAuthGameMode<AChronosGameMode>();
	if (!GM)
	{
		for (TActorIterator<AChronosGameMode> It(GetWorld()); It; ++It)
		{
			GM = *It;
			break;
		}
	}
	if (GM)
	{
		GM->RegisterEnemy(this);
	}
}

void AChronosEnemy::SpawnDefaultWeapon_Implementation()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.Owner = this;
	AChronosWeapon* Weapon = World->SpawnActor<AChronosWeapon>(AChronosWeapon::StaticClass(), GetActorTransform(), Params);
	if (!Weapon)
	{
		return;
	}

	Weapon->InitFromData(EnemyWeaponData);
	// Task 1 修复后 EquipWeapon 内部会调用 EquipTo 完成挂接与状态切换
	CombatComponent->EquipWeapon(Weapon);
}

FVector AChronosEnemy::GetAimDirection() const
{
	if (const AActor* Target = CombatTarget.Get())
	{
		FVector Direction = Target->GetActorLocation() - GetActorLocation();
		if (!Direction.IsNearlyZero())
		{
			return Direction.GetSafeNormal();
		}
	}

	return Super::GetAimDirection();
}
