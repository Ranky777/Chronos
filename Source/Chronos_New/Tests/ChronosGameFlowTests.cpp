#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "AIController.h"
#include "Characters/ChronosCharacter.h"
#include "Characters/ChronosEnemy.h"
#include "Components/CombatComponent.h"
#include "Components/HealthComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
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

/**
 * 回归：多关模式下清空非末关时，清关广播必须带 bIsFinalLevel = false。
 *
 * 决定"显示过场进下一关"还是"显示 YOU WIN"的就是这个标志：
 * 只有它是 false，PlayerController 才会显示 LevelTransition 并推进关卡。
 * 现有 RegisterAndClear 只覆盖了单关（必然是末关），这里补多关分支。
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChronosGameFlow_MultiLevelNotFinal, "Chronos.GameFlow.MultiLevelNotFinal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FChronosGameFlow_MultiLevelNotFinal::RunTest(const FString& Parameters)
{
	UWorld* World = CreateTestWorld();
	AChronosGameMode* GM = World->SpawnActor<AChronosGameMode>();

	// 三关列表（软引用，只要数量与顺序构成"多于一项"即可触发非末关判定）
	GM->LevelList.Add(TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/FirstPerson/Lvl_FirstPerson.Lvl_FirstPerson"))));
	GM->LevelList.Add(TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/Maps/Lvl_Arena02.Lvl_Arena02"))));
	GM->LevelList.Add(TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/Maps/Lvl_Arena03.Lvl_Arena03"))));

	TestEqual(TEXT("关卡列表已配置三项"), GM->LevelList.Num(), 3);
	TestFalse(TEXT("当前关不是末关"), GM->IsFinalLevel());

	UChronosTestListener* Listener = NewObject<UChronosTestListener>();
	GM->OnLevelCleared.AddDynamic(Listener, &UChronosTestListener::HandleCleared);

	AChronosEnemy* Enemy = World->SpawnActor<AChronosEnemy>();
	BeginPlayTestWorld(World);
	Enemy->GetHealthComponent()->TakeDamage(nullptr);

	TestEqual(TEXT("清关广播一次"), Listener->ClearedCount, 1);
	TestFalse(TEXT("多关模式下首关清空不是末关 —— 应走过场而非直接 YOU WIN"), Listener->bFinalFlag);

	World->DestroyWorld(false);
	return true;
}

/**
 * 回归：敌人不能把同类（另一个敌人）当作交战目标。
 *
 * 场上有多个敌人时，模板的 AI 感知按角色基类感知、不区分敌我，
 * 若直接取感知列表第一个，敌人会把同伴当目标 —— 表现为"敌人互相开火却打玩家时不开枪"。
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChronosEnemy_RejectsAllyAsTarget, "Chronos.GameFlow.EnemyRejectsAllyAsTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FChronosEnemy_RejectsAllyAsTarget::RunTest(const FString& Parameters)
{
	UWorld* World = CreateTestWorld();
	World->SpawnActor<AChronosGameMode>();
	AChronosEnemy* Enemy = World->SpawnActor<AChronosEnemy>();
	AChronosEnemy* Ally = World->SpawnActor<AChronosEnemy>();
	BeginPlayTestWorld(World);

	Enemy->SetCombatTarget(Ally);
	TestTrue(TEXT("同类不会被设为交战目标"), Enemy->GetCombatTarget() != Ally);

	// 非同类应当可以正常设置
	AActor* Other = World->SpawnActor<AActor>();
	Enemy->SetCombatTarget(Other);
	TestEqual(TEXT("非同类可正常设为目标"), Enemy->GetCombatTarget(), Other);

	World->DestroyWorld(false);
	return true;
}

/**
 * 回归：敌人死亡时必须先停掉 AI 逻辑（StateTree）再 UnPossess。
 *
 * StateTree 组件的上下文是「Actor + Controller」，UnPossess 与随后蓝图里的 DestroyActor
 * 会让上下文失效，而 StateTree 仍在 Tick，于是持续刷
 * "Ensure condition failed: bValidContextRequirements ... it's now invalid"。
 * 这里给敌人挂一个真实的 AAIController 再杀死它，确保死亡路径不会留下跑着的行为逻辑。
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChronosEnemy_DeathStopsAILogic, "Chronos.GameFlow.EnemyDeathStopsAILogic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FChronosEnemy_DeathStopsAILogic::RunTest(const FString& Parameters)
{
	UWorld* World = CreateTestWorld();
	World->SpawnActor<AChronosGameMode>();
	AChronosEnemy* Enemy = World->SpawnActor<AChronosEnemy>();
	BeginPlayTestWorld(World);

	AAIController* AI = World->SpawnActor<AAIController>();
	AI->Possess(Enemy);
	TestTrue(TEXT("AI 已接管敌人"), Enemy->GetController() == AI);

	Enemy->GetHealthComponent()->TakeDamage(nullptr);

	TestTrue(TEXT("死亡后已脱离控制器"), Enemy->GetController() == nullptr);
	TestTrue(TEXT("角色已标记为死亡"), !Enemy->GetHealthComponent()->IsAlive());

	World->DestroyWorld(false);
	return true;
}

/**
 * 回归：敌人出生武器必须"有数据且弹药 > 0"。
 *
 * 这是"敌人举着枪却完全不开火"的直接原因 —— 武器没有数据资产就谈不上 RemainingAmmo，
 * 弹药为 0 时 CanFire 恒假。难度参数 StartingAmmoOverride 也应生效。
 *
 * 注：测试世界里 DefaultWeaponClass 的软引用加载链路受限，这里显式给一份数据兜底，
 * 验证的是"给到数据后武器确实被正确初始化"这一契约。
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChronosEnemy_SpawnedWeaponIsLoaded, "Chronos.GameFlow.EnemySpawnedWeaponIsLoaded",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FChronosEnemy_SpawnedWeaponIsLoaded::RunTest(const FString& Parameters)
{
	UWorld* World = CreateTestWorld();
	World->SpawnActor<AChronosGameMode>();
	AChronosEnemy* Enemy = World->SpawnActor<AChronosEnemy>();

	UWeaponDataAsset* Data = NewObject<UWeaponDataAsset>();
	Data->AmmoCount = 8;
	Data->FireRate = 600.f;
	Data->ProjectileClass = AChronosProjectile::StaticClass();
	Enemy->EnemyWeaponData = Data;
	Enemy->StartingAmmoOverride = 3;

	BeginPlayTestWorld(World);

	AChronosWeapon* W = Cast<AChronosWeapon>(Enemy->GetCombatComponent()->GetCurrentWeapon().GetObject());
	TestTrue(TEXT("敌人出生有武器"), W != nullptr);

	if (W)
	{
		TestTrue(TEXT("武器数据非空"), W->WeaponData != nullptr);
		TestTrue(TEXT("武器有弹药（无弹药 = 举枪不开火）"), W->GetCurrentAmmo_Implementation() > 0);
		TestEqual(TEXT("难度参数覆写了出生弹药"), W->GetCurrentAmmo_Implementation(), 3);
	}

	World->DestroyWorld(false);
	return true;
}

/**
 * 诊断：武器装备时角色动画实例（ABP）是否切换。
 *
 * 覆盖两个场景：
 *   A) 同一把武器"装备 → 脱手 → 再装备"，验证二次装备是否仍切换 ABP；
 *   B) 敌人出生自动装备的武器脱手后由玩家拾取，验证跨角色交接是否切换 ABP。
 * 必须用 BP_ShooterCharacter（裸 C++ 角色没有 FirstPersonMesh 组件）。
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChronosWeapon_AnimClassOnEquip, "Chronos.Weapon.AnimClassOnEquip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

static FString NameOf(const UClass* Class)
{
	return Class ? Class->GetName() : TEXT("None");
}

bool FChronosWeapon_AnimClassOnEquip::RunTest(const FString& Parameters)
{
	UWorld* World = CreateTestWorld();

	UClass* PlayerClass = LoadClass<AActor>(nullptr,
		TEXT("/Game/Variant_Shooter/Blueprints/BP_ShooterCharacter.BP_ShooterCharacter_C"));
	UClass* WeaponClass = LoadClass<AActor>(nullptr,
		TEXT("/Game/Variant_Shooter/Blueprints/Pickups/Weapons/BP_ShooterWeapon_Pistol.BP_ShooterWeapon_Pistol_C"));

	if (!PlayerClass || !WeaponClass)
	{
		AddError(TEXT("无法加载 BP_ShooterCharacter 或 BP_ShooterWeapon_Pistol"));
		World->DestroyWorld(false);
		return false;
	}

	AChronosCharacter* Player = World->SpawnActor<AChronosCharacter>(PlayerClass);
	AChronosEnemy* Enemy = World->SpawnActor<AChronosEnemy>();

	// 敌人与玩家共用同一份武器数据（难度差异在 AI 侧的 FireInterval / StartingAmmoOverride）
	UWeaponDataAsset* EnemyData = LoadObject<UWeaponDataAsset>(nullptr,
		TEXT("/Game/DataAssets/DA_Pistol.DA_Pistol"));
	Enemy->EnemyWeaponData = EnemyData;

	BeginPlayTestWorld(World);

	USkeletalMeshComponent* FPMesh = Player->GetFirstPersonMesh();
	USkeletalMeshComponent* TPMesh = Player->GetMesh();
	TestTrue(TEXT("玩家存在第一人称网格"), FPMesh != nullptr);
	if (!FPMesh)
	{
		World->DestroyWorld(false);
		return false;
	}

	const FString Initial = NameOf(FPMesh->AnimClass);

	// ---- 场景 A：装备 → 脱手 → 再装备 ----
	AChronosWeapon* Weapon = World->SpawnActor<AChronosWeapon>(WeaponClass);
	Player->GetCombatComponent()->EquipWeapon(Weapon);
	const FString AfterEquip1 = NameOf(FPMesh->AnimClass);
	const FString AfterEquip1TP = NameOf(TPMesh ? TPMesh->AnimClass : nullptr);

	Player->GetCombatComponent()->DropCurrentWeapon();
	const FString AfterDrop = NameOf(FPMesh->AnimClass);

	Player->GetCombatComponent()->EquipWeapon(Weapon);
	const FString AfterEquip2 = NameOf(FPMesh->AnimClass);

	UE_LOG(LogTemp, Warning, TEXT("[AnimDiag] 初始=%s | 首次装备=%s (TP=%s) | 脱手=%s | 二次装备=%s"),
		*Initial, *AfterEquip1, *AfterEquip1TP, *AfterDrop, *AfterEquip2);

	TestTrue(TEXT("A1 首次装备切换了第一人称 ABP"), AfterEquip1 != Initial);
	TestTrue(TEXT("A2 脱手后恢复默认 ABP"), AfterDrop == Initial);
	TestTrue(TEXT("A3 二次装备仍切换第一人称 ABP"), AfterEquip2 != Initial);

	// ---- 场景 B：敌人武器 → 玩家拾取 ----
	AChronosWeapon* EnemyWeapon = Cast<AChronosWeapon>(Enemy->GetCombatComponent()->GetCurrentWeapon().GetObject());
	TestTrue(TEXT("B0 敌人出生已装备武器"), EnemyWeapon != nullptr);
	if (EnemyWeapon)
	{
		// 模拟敌人死亡：HandleDeath 会调 CombatComponent->DropCurrentWeapon()
		Enemy->GetHealthComponent()->TakeDamage(nullptr);
		const FString AfterEnemyDeath = NameOf(FPMesh->AnimClass);

		Player->GetCombatComponent()->DropCurrentWeapon();   // 先腾空玩家手
		const FString BeforeEnemyPickup = NameOf(FPMesh->AnimClass);
		Player->GetCombatComponent()->EquipWeapon(EnemyWeapon);
		const FString AfterEnemyPickup = NameOf(FPMesh->AnimClass);
		const FString AfterEnemyPickupTP = NameOf(TPMesh ? TPMesh->AnimClass : nullptr);

		UE_LOG(LogTemp, Warning,
			TEXT("[AnimDiag] 敌人武器=%s 数据=%s | 敌人死亡后玩家ABP=%s | 拾取前=%s | 拾取后=%s (TP=%s)"),
			*EnemyWeapon->GetClass()->GetName(),
			*(EnemyWeapon->WeaponData ? EnemyWeapon->WeaponData->GetName() : TEXT("None")),
			*AfterEnemyDeath, *BeforeEnemyPickup, *AfterEnemyPickup, *AfterEnemyPickupTP);

		TestTrue(TEXT("B1 拾取敌人掉落的武器切换了第一人称 ABP"), AfterEnemyPickup != Initial);
	}

	World->DestroyWorld(false);
	return true;
}

#endif
