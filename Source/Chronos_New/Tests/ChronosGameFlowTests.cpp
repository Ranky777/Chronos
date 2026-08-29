#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Characters/ChronosCharacter.h"
#include "Characters/ChronosEnemy.h"
#include "Components/CombatComponent.h"
#include "Components/HealthComponent.h"
#include "Components/SceneComponent.h"
#include "GameFlow/ChronosGameMode.h"
#include "Projectiles/Projectile.h"
#include "Tests/ChronosTestListener.h"
#include "Weapons/ChronosWeapon.h"
#include "Weapons/WeaponDataAsset.h"

static UWorld* CreateTestWorld()
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	return World;
}

/**
 * AActor::ProcessEvent 会在世界未完成 Actor 初始化时静默丢弃所有反射调用
 * （Engine/Private/Actor.cpp 中 AreActorsInitialized 门禁），因此动态委托
 * OnDeath → HandleDeath → OnCharacterDeath 的派发依赖 bActorsInitialized 置位。
 * 本测试世界无 AuthorityGameMode（GameMode 由 SpawnActor 创建而非世界注册），
 * 不会走引擎常规的 StartPlay 链路，这里按引擎真实顺序手动补齐：
 * InitializeActorsForPlay（置位 bActorsInitialized）→ BeginPlay（子系统）
 * → NotifyBeginPlay（逐 Actor DispatchBeginPlay）。
 */
static void BeginPlayTestWorld(UWorld* World)
{
	World->InitializeActorsForPlay(FURL());
	World->BeginPlay();
	World->GetWorldSettings()->NotifyBeginPlay();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChronosGameFlow_RegisterAndClear, "Chronos.GameFlow.RegisterAndClear",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FChronosGameFlow_RegisterAndClear::RunTest(const FString& Parameters)
{
	UWorld* World = CreateTestWorld();
	AChronosGameMode* GM = World->SpawnActor<AChronosGameMode>();
	AChronosEnemy* Enemy = World->SpawnActor<AChronosEnemy>();
	UChronosTestListener* Listener = NewObject<UChronosTestListener>();
	GM->OnLevelCleared.AddDynamic(Listener, &UChronosTestListener::HandleCleared);
	GM->OnEnemyCountChanged.AddDynamic(Listener, &UChronosTestListener::HandleCount);

	BeginPlayTestWorld(World);

	TestEqual("敌人生成后自动注册", GM->GetRemainingEnemies(), 1);
	TestEqual("注册时广播计数", Listener->CountCalls, 1);

	Enemy->GetHealthComponent()->TakeDamage(nullptr);

	TestEqual("死亡后注销", GM->GetRemainingEnemies(), 0);
	TestEqual("清关广播一次", Listener->ClearedCount, 1);
	TestTrue("单关模式视为末关", Listener->bFinalFlag);

	World->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChronosEnemy_AimAndAutoEquip, "Chronos.GameFlow.EnemyAimAndAutoEquip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FChronosEnemy_AimAndAutoEquip::RunTest(const FString& Parameters)
{
	UWorld* World = CreateTestWorld();
	AChronosGameMode* GM = World->SpawnActor<AChronosGameMode>();
	AChronosEnemy* Enemy = World->SpawnActor<AChronosEnemy>();

	UWeaponDataAsset* Data = NewObject<UWeaponDataAsset>();
	Data->AmmoCount = 2;
	Data->FireRate = 600.f;
	Data->ProjectileClass = AChronosProjectile::StaticClass();
	Enemy->EnemyWeaponData = Data;

	AActor* Target = World->SpawnActor<AActor>();
	// 裸 AActor 无 RootComponent，SetActorLocation 是静默 no-op（Actor.cpp L4987），
	// 必须先挂一个根场景组件，目标才能真正离开原点
	Target->SetRootComponent(NewObject<USceneComponent>(Target));
	Target->GetRootComponent()->RegisterComponent();
	Target->SetActorLocation(FVector(0.f, 1000.f, 0.f));
	Enemy->SetCombatTarget(Target);

	BeginPlayTestWorld(World);

	TestTrue("出生自动装备武器", Enemy->GetCombatComponent()->GetCurrentWeapon().GetObject() != nullptr);
	TestTrue("无相机时朝目标瞄准", Enemy->GetAimDirection().Equals(FVector::RightVector, 0.01f));

	World->DestroyWorld(false);
	return true;
}

#endif
