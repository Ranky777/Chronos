# Chronos 迁移实施计划（Chronos → Chronos_New）

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 在 Chronos_New（UE5.8 Variant_Shooter 模板 + 已有 C++ 框架）上重现 SUPERHOT 风格玩法：多小关卡清敌推进、一击必杀、无输入时间冻结、三把武器、投掷击杀、全流程胜负界面。

**Architecture:** 模板 BP 重父类到 C++ 基类（BP_ShooterWeaponBase→AChronosWeapon，BP_ShooterNPC→AChronosEnemy，BP_ShooterGameMode→AChronosGameMode，BP_ShooterPlayerController→AChronosPlayerController）；C++ 拥有规则与流程，蓝图拥有表现/资产配置/UMG/StateTree。规格见 `docs/superpowers/specs/2026-08-29-chronos-migration-design.md`。

**Tech Stack:** UE 5.8、C++（单模块 Chronos_New）、Enhanced Input、GameplayTags、StateTree、Niagara、UMG、unreal-mcp（编辑器自动化）、UE Automation Testing。

## Global Constraints

- **MCP 调用必须串行**（游戏线程执行，并行会死锁）。每次 `call_tool` 后检查返回，非显式成功视为失败停止。
- **编辑器重编译纪律**：改了 `.h`（新增 UCLASS/UPROPERTY/UFUNCTION）→ 必须关闭编辑器 → UBT 全量构建 → 重启编辑器；只改 `.cpp` 函数体 → 可用 `LiveCodingToolset.CompileLiveCoding`。
- **编辑器关闭流程**：先经 MCP 保存全部脏资产 → 执行控制台命令 `QUIT_EDITOR` 优雅退出（禁止 taskkill）。编辑器重启：后台运行 `<引擎>\Binaries\Win64\UnrealEditor.exe "<uproject>" -log`，轮询 `list_toolsets` 直到恢复。
- **引擎路径探测**：UE 5.8（EngineAssociation "5.8"）。先 `ls "C:\Program Files\Epic Games"` 找 `UE_5.8`；构建命令：
  `"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" Chronos_NewEditor Win64 Development -project="C:\Users\cuilongqi\Desktop\UEProjects\Chronos_New\Chronos_New.uproject" -WaitMutex`
- **批量资产操作前后各保存一次**（AssetTools 保存 API / 控制台 `SAVE_ALL`？以 EditorApp/AssetTools 实际工具为准）。
- **提交纪律**：每任务一提交，`git add` 只加本任务文件（工作区/暂存区有大量用户预存改动，严禁 `git add -A`/`git add .`）。提交信息中文一行 + `Co-Authored-By: Claude Haiku 4.5 (1M context) <noreply@anthropic.com>` 尾行。
- **交接文档随时同步**：每任务最后一步更新 `docs/Architecture.md` 对应小节（该文件由 Task 1 创建骨架）并纳入同次提交。
- **沟通纪律**：每完成一步用一句中文向用户说明"改了什么、为什么"。
- 用户已知决策：近战已从范围移除；BP_Pistol 将被删除；模板 DT_WeaponList/BP_ShooterPickup 不进运行时。

## 现有代码事实（实施者必读）

- `AChronosCharacter::HandleDeath` 已实现：掉武器（CombatComponent→DropCurrentWeapon）、玩家 SetPlayerDead(true)+DisableInput、敌人 UnPossess、布娃娃、`OnCharacterDeath.Broadcast(this)`（`FOnCharacterDeath(AChronosCharacter*)`，dynamic 委托）。
- `UCombatComponent::HandleAmmoChanged` 已实现"弹尽自动朝准星方向扔枪"——敌人空弹行为无需额外开发。
- **已知缺口（Task 1 修复）**：`UCombatComponent::EquipWeapon` 只登记 `CurrentWeapon`，从不调用 `AChronosWeapon::EquipTo`，拾取后武器不附加/不进 Equipped 状态。
- **已知缺口（Task 1 修复）**：`Fire_Implementation`/`FireAtTarget` 在 `ProjectileClass` 为空时直接返回——榴弹发射器（BP 自管投射物）无法走 C++ 弹药记账。需改为"无 ProjectileClass 时不生成子弹但照常扣弹药/冷却/广播/调 OnFireVisuals"。
- `AChronosProjectile`：非 Blueprintable；池入口 `UChronosProjectilePoolSubsystem::FireProjectile(Class, Instigator, Loc, Rot)`。
- `UTimeDilationSubsystem`：`FOnTimeDilationChanged(float)` dynamic 委托名 `OnDilationChanged`；`TriggerKillOverride(Dilation=0.05, Duration=0.3)`。
- `UHealthComponent::TakeDamage(AActor* InstigatorActor)`；`OnDeath` 为 `FChronosOnDeath(AActor* DamagedActor, AActor* Killer)`。
- DefaultGameMode = `/Game/Variant_Shooter/Blueprints/BP_ShooterGameMode.BP_ShooterGameMode_C`；启动图 `/Game/FirstPerson/Lvl_FirstPerson`。
- 碰撞：投射物走预设 `Projectile`（ECC_GameTraceChannel1）。
- 蓝图父类链（MCP 实查）：BP_ShooterCharacter 与 BP_ShooterNPC → BP_FirstPersonCharacter_C → AChronosCharacter；BP_ShooterWeaponBase → AActor；BP_ShooterGameMode → GameModeBase；BP_ShooterPlayerController → BP_FirstPersonPlayerController_C。
- 模板资产路径：武器 BP `/Game/Variant_Shooter/Blueprints/Pickups/Weapons/BP_ShooterWeapon_{Pistol,Rifle,GrenadeLauncher}`；NPC `/Game/Variant_Shooter/Blueprints/AI/BP_ShooterNPC`；StateTree `/Game/Variant_Shooter/Blueprints/AI/ST_Shooter`；控制器 `/Game/Variant_Shooter/Blueprints/BP_ShooterPlayerController`；用户资产 `/Game/Blueprints/Weapons/BP_Pistol`（待删）、`/Game/DataAssets/DA_Pistol`（保留改造）。

---

### Task 1: C++ 修补——武器表现事件、EquipTo 缺口、Fire 空投射物类型、移除近战

**Files:**
- Modify: `Source/Chronos_New/Weapons/ChronosWeapon.h`
- Modify: `Source/Chronos_New/Weapons/ChronosWeapon.cpp`
- Modify: `Source/Chronos_New/Components/CombatComponent.cpp`
- Modify: `Source/Chronos_New/Characters/ChronosCharacter.h`
- Modify: `Source/Chronos_New/Characters/ChronosCharacter.cpp`
- Create: `docs/Architecture.md`（骨架）

**Interfaces:**
- Produces: `AChronosWeapon::OnFireVisuals()` / `AChronosWeapon::OnEquipVisuals()`（BlueprintImplementableEvent，Task 4 的 BP 实现）；`UCombatComponent::EquipWeapon` 语义修正（登记 + EquipTo）；`Fire_Implementation` 允许 `ProjectileClass == nullptr`（Task 4 榴弹发射器依赖）；`docs/Architecture.md` 骨架（后续任务持续更新）。

- [ ] **Step 1: ChronosWeapon.h 添加表现事件**

在 `class AChronosWeapon` 的 `protected:` 区（`BeginPlay` 声明之后）添加：

```cpp
	/** 开火表现钩子：蓝图播放 FireMontage/音效/枪口 FX。C++ 已完成弹道、弹药与冷却 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Chronos|Weapon")
	void OnFireVisuals();

	/** 入手表现钩子：蓝图播放装备动画/音效 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Chronos|Weapon")
	void OnEquipVisuals();
```

- [ ] **Step 2: ChronosWeapon.cpp 调用钩子 + 允许空投射物类型**

`FireAtTarget`：把开头的守卫与生成段替换为（注意不再要求 ProjectileClass 非空）：

```cpp
bool AChronosWeapon::FireAtTarget(const FVector& TargetPoint)
{
	if (!CanFire_Implementation() || !WeaponData)
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const FVector MuzzleLocation = GetMuzzleLocation_Implementation();
	FVector FireDirection = TargetPoint - MuzzleLocation;
	if (FireDirection.IsNearlyZero())
	{
		FireDirection = GetActorForwardVector();
	}
	else
	{
		FireDirection.Normalize();
	}

	// ProjectileClass 为空的武器（如榴弹发射器）由 BP OnFireVisuals 自行生成投射物
	if (WeaponData->ProjectileClass)
	{
		if (UChronosProjectilePoolSubsystem* Pool = World->GetSubsystem<UChronosProjectilePoolSubsystem>())
		{
			Pool->FireProjectile(WeaponData->ProjectileClass, GetOwner(), MuzzleLocation, FireDirection.Rotation());
		}
	}

	--RemainingAmmo;
	LastFireRealTime = FPlatformTime::Seconds();
	RefireCooldown = 1.f / WeaponData->FireRate;

	OnAmmoChanged.Broadcast(RemainingAmmo, WeaponData->AmmoCount);
	OnFireVisuals();

	return true;
}
```

`Fire_Implementation` 同样处理（守卫去掉 `!WeaponData->ProjectileClass`，生成段包 `if (WeaponData->ProjectileClass)`，末尾加 `OnFireVisuals();`）：

```cpp
void AChronosWeapon::Fire_Implementation()
{
	if (!CanFire_Implementation() || !WeaponData)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector MuzzleLocation = GetMuzzleLocation_Implementation();
	const FVector FireDirection = WeaponMeshComponent->GetSocketRotation(WeaponData->MuzzleSocketName).Vector();

	if (WeaponData->ProjectileClass)
	{
		if (UChronosProjectilePoolSubsystem* Pool = World->GetSubsystem<UChronosProjectilePoolSubsystem>())
		{
			Pool->FireProjectile(WeaponData->ProjectileClass, GetOwner(), MuzzleLocation, FireDirection.Rotation());
		}
	}

	--RemainingAmmo;
	LastFireRealTime = FPlatformTime::Seconds();
	RefireCooldown = 1.f / WeaponData->FireRate;

	OnAmmoChanged.Broadcast(RemainingAmmo, WeaponData->AmmoCount);
	OnFireVisuals();
}
```

`EquipTo` 末尾（`AttachToComponent(...)` 之后）追加：

```cpp
	OnEquipVisuals();
```

- [ ] **Step 3: CombatComponent.cpp 修复 EquipTo 缺口**

`EquipWeapon` 中，在"订阅弹药变化"代码块之后、`OnWeaponChanged.Broadcast` 之前插入：

```cpp
	// 修复：接口只登记指针；"附加到手上 + 切换 Equipped 状态"必须由具体武器类执行，
	// 否则交互拾取后武器仍停留在世界状态（无碰撞表现错误且不能开火）
	if (AChronosWeapon* ConcreteWeapon = Cast<AChronosWeapon>(CurrentWeapon.GetObject()))
	{
		ConcreteWeapon->EquipTo(Holder);
	}
```

并在文件头 include 区确认有 `#include "Weapons/ChronosWeapon.h"`（已有）。

- [ ] **Step 4: 移除近战（ChronosCharacter）**

`ChronosCharacter.h` 删除两处：

```cpp
	/** 近战攻击距离 */                                          // 删除整段 UPROPERTY
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Chronos|Melee", meta = (ClampMin = "10"))
	float MeleeRange = 150.f;
```

```cpp
	/** 无武器时的近战拳击：短距离球形扫描，命中即死 */
	void PerformMeleeAttack();
```

`ChronosCharacter.cpp`：
1. `OnFireStarted` 的 else 分支删除，改为：

```cpp
	TScriptInterface<IWeaponUser> Weapon = CombatComponent->GetCurrentWeapon();
	if (Weapon.GetObject())
	{
		IWeaponUser::Execute_Fire(Weapon.GetObject());
	}
```

2. 整个 `PerformMeleeAttack()` 函数体删除。
3. 注释"每个输入处理函数的第一行都调用……"所在类头无需改。

- [ ] **Step 5: 全量构建 + 重启编辑器验证**

按 Global Constraints：保存资产 → `QUIT_EDITOR` → 跑 Build.bat → 重启编辑器 → `list_toolsets` 恢复。

- [ ] **Step 6: PIE 冒烟**

EditorAppToolset 开始 PIE：在 Lvl_FirstPerson 生成一个 `BP_Pistol`（此时仍可用）+ 玩家，控制台 `Summon` 或放置后 PIE，验证 E 交互拾取后武器出现在手上（此前不出现——本任务修复项）、左键能射出子弹。结束 PIE。

- [ ] **Step 7: 创建 docs/Architecture.md 骨架**

```markdown
# Chronos_New 架构与交接文档

> 本文档供后续 AI 会话接手项目，随每次改动同步更新。设计规格：docs/superpowers/specs/2026-08-29-chronos-migration-design.md

## 项目定位
SUPERHOT 风格第一人称 Arena Shooter 求职项目。C++ 系统 + 蓝图表现混合。

## C++ 单元
（随任务填充：类名 / 文件 / 职责一句话）

## 蓝图资产
（随任务填充：资产路径 / 父类 / 用途）

## 关卡
（随任务填充）

## 构建与运行
- 引擎：UE 5.8（EngineAssociation "5.8"）
- 构建：Build.bat Chronos_NewEditor Win64 Development -project=... -WaitMutex
- MCP：编辑器启动后 list_toolsets 确认在线；调用必须串行

## 剩余任务
- 见 docs/superpowers/plans/2026-08-29-chronos-migration.md 勾选状态
```

- [ ] **Step 8: 提交**

```bash
git add Source/Chronos_New/Weapons/ChronosWeapon.h Source/Chronos_New/Weapons/ChronosWeapon.cpp Source/Chronos_New/Components/CombatComponent.cpp Source/Chronos_New/Characters/ChronosCharacter.h Source/Chronos_New/Characters/ChronosCharacter.cpp docs/Architecture.md
git commit -m "$(cat <<'EOF'
fix: 武器拾取 EquipTo 缺口、表现事件钩子、移除近战

Co-Authored-By: Claude Haiku 4.5 (1M context) <noreply@anthropic.com>
EOF
)"
```

---

### Task 2: C++ 游戏流程——AChronosEnemy + AChronosGameMode + 自动化测试

**Files:**
- Create: `Source/Chronos_New/GameFlow/ChronosGameMode.h`
- Create: `Source/Chronos_New/GameFlow/ChronosGameMode.cpp`
- Create: `Source/Chronos_New/Characters/ChronosEnemy.h`
- Create: `Source/Chronos_New/Characters/ChronosEnemy.cpp`
- Create: `Source/Chronos_New/Tests/ChronosGameFlowTests.cpp`
- Modify: `Source/Chronos_New/Chronos_New.Build.cs`
- Modify: `docs/Architecture.md`

**Interfaces:**
- Consumes: `AChronosCharacter::OnCharacterDeath`（dynamic，`FOnCharacterDeath(AChronosCharacter*)`）、`UHealthComponent::TakeDamage(AActor*)`、`UCombatComponent::EquipWeapon`（Task 1 语义）、`AChronosWeapon::InitFromData`。
- Produces（Task 7/8/9 依赖）：`AChronosGameMode`（`LevelList:TArray<TSoftObjectPtr<UWorld>>`、`GetRemainingEnemies()`、`IsFinalLevel()`、`RestartLevel()`、`LoadNextLevel()`、`OnEnemyCountChanged(int32)`、`OnLevelCleared(bool bIsFinalLevel)`）；`AChronosEnemy`（`SetCombatTarget(AActor*)`、`GetCombatTarget()`、`EnemyWeaponData:TObjectPtr<UWeaponDataAsset>`）。

- [ ] **Step 1: 写失败的测试**（Tests/ChronosGameFlowTests.cpp）

```cpp
#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Characters/ChronosCharacter.h"
#include "Characters/ChronosEnemy.h"
#include "Components/CombatComponent.h"
#include "Components/HealthComponent.h"
#include "GameFlow/ChronosGameMode.h"
#include "Weapons/ChronosWeapon.h"
#include "Weapons/WeaponDataAsset.h"

/** 测试监听器：dynamic 委托需要 UObject + UFUNCTION */
UCLASS()
class UChronosTestListener : public UObject
{
	GENERATED_BODY()
public:
	int32 ClearedCount = 0;
	bool bFinalFlag = false;
	int32 CountCalls = 0;

	UFUNCTION()
	void HandleCleared(bool bFinal) { ++ClearedCount; bFinalFlag = bFinal; }
	UFUNCTION()
	void HandleCount(int32) { ++CountCalls; }
};

static UWorld* CreateTestWorld()
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	return World;
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

	World->BeginPlay();

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
	Target->SetActorLocation(FVector(1000.f, 0.f, 0.f));
	Enemy->SetCombatTarget(Target);

	World->BeginPlay();

	TestTrue("出生自动装备武器", Enemy->GetCombatComponent()->GetCurrentWeapon().GetObject() != nullptr);
	TestTrue("无相机时朝目标瞄准", Enemy->GetAimDirection().Equals(FVector::ForwardVector, 0.01f));

	World->DestroyWorld(false);
	return true;
}

#endif
```

注意：`AChronosProjectile` 需 `#include "Projectiles/Projectile.h"`（加在 include 区）。

- [ ] **Step 2: Build.cs 添加编辑器测试依赖**

`Chronos_New.Build.cs` 构造函数末尾追加：

```csharp
	// 自动化测试需要 UnrealEd（FAutomationEditorCommonUtils/CreateWorld 场景）
	if (Target.Type == TargetType.Editor)
	{
		PrivateDependencyModuleNames.Add("UnrealEd");
	}
```

- [ ] **Step 3: 跑构建确认编译失败**（类不存在）

关闭编辑器 → Build.bat。预期：编译错误（找不到 ChronosGameMode.h）。这确认测试先于实现失败。

- [ ] **Step 4: 实现 ChronosGameMode.h**

```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ChronosGameMode.generated.h"

class AChronosCharacter;
class AChronosEnemy;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChronosEnemyCountChanged, int32, RemainingEnemies);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChronosLevelCleared, bool, bIsFinalLevel);

/**
 * SUPERHOT 多小关卡推进：敌人自行注册、死亡注销，计数归零即清关；
 * 关卡列表由 BP 子类配置，OpenLevel 切换后 GameMode 重新实例化时按世界名恢复进度。
 */
UCLASS(Blueprintable)
class CHRONOS_NEW_API AChronosGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AChronosGameMode();

	virtual void BeginPlay() override;

	/** 顺序关卡列表（软引用，在 BP 子类中配置）；为空表示单关模式 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chronos|Flow")
	TArray<TSoftObjectPtr<UWorld>> LevelList;

	UFUNCTION(BlueprintPure, Category = "Chronos|Flow")
	int32 GetRemainingEnemies() const { return ActiveEnemies.Num(); }

	UFUNCTION(BlueprintPure, Category = "Chronos|Flow")
	bool IsFinalLevel() const { return LevelList.Num() > 0 && CurrentLevelIndex == LevelList.Num() - 1; }

	UFUNCTION(BlueprintCallable, Category = "Chronos|Flow")
	void RestartLevel();

	/** 过场 UI 播放完毕后由 PlayerController 调用，加载列表中的下一关 */
	UFUNCTION(BlueprintCallable, Category = "Chronos|Flow")
	void LoadNextLevel();

	UPROPERTY(BlueprintAssignable, Category = "Chronos|Flow")
	FOnChronosEnemyCountChanged OnEnemyCountChanged;

	UPROPERTY(BlueprintAssignable, Category = "Chronos|Flow")
	FOnChronosLevelCleared OnLevelCleared;

	/** 敌人 BeginPlay 时自行注册并订阅死亡 */
	void RegisterEnemy(AChronosEnemy* Enemy);

private:
	UFUNCTION()
	void HandleEnemyDeath(AChronosCharacter* Character);

	void BroadcastCounts();

	UPROPERTY()
	TArray<TObjectPtr<AChronosEnemy>> ActiveEnemies;

	int32 CurrentLevelIndex = INDEX_NONE;
	bool bLevelCleared = false;
};
```

- [ ] **Step 5: 实现 ChronosGameMode.cpp**

```cpp
#include "GameFlow/ChronosGameMode.h"

#include "Characters/ChronosCharacter.h"
#include "Characters/ChronosEnemy.h"
#include "Kismet/GameplayStatics.h"
#include "Subsystems/TimeDilationSubsystem.h"

AChronosGameMode::AChronosGameMode()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AChronosGameMode::BeginPlay()
{
	Super::BeginPlay();

	// OpenLevel 之后 GameMode 会重新实例化，用世界名在列表中恢复进度
	const FString WorldName = FPackageName::GetShortName(GetWorld()->GetName());
	for (int32 Index = 0; Index < LevelList.Num(); ++Index)
	{
		if (LevelList[Index].GetAssetName() == WorldName)
		{
			CurrentLevelIndex = Index;
			break;
		}
	}
}

void AChronosGameMode::RegisterEnemy(AChronosEnemy* Enemy)
{
	if (!Enemy || ActiveEnemies.Contains(Enemy))
	{
		return;
	}

	Enemy->OnCharacterDeath.AddDynamic(this, &AChronosGameMode::HandleEnemyDeath);
	ActiveEnemies.Add(Enemy);
	BroadcastCounts();
}

void AChronosGameMode::HandleEnemyDeath(AChronosCharacter* Character)
{
	AChronosEnemy* Enemy = Cast<AChronosEnemy>(Character);
	if (!Enemy)
	{
		return;
	}

	ActiveEnemies.Remove(Enemy);

	// 击杀瞬间子弹时间（子系统默认 0.05/0.3s）
	if (UTimeDilationSubsystem* Time = GetWorld()->GetSubsystem<UTimeDilationSubsystem>())
	{
		Time->TriggerKillOverride();
	}

	BroadcastCounts();

	if (ActiveEnemies.Num() == 0 && !bLevelCleared)
	{
		bLevelCleared = true;
		OnLevelCleared.Broadcast(IsFinalLevel());
	}
}

void AChronosGameMode::RestartLevel()
{
	UGameplayStatics::OpenLevel(this, FName(*GetWorld()->GetName()));
}

void AChronosGameMode::LoadNextLevel()
{
	if (LevelList.IsValidIndex(CurrentLevelIndex + 1))
	{
		UGameplayStatics::OpenLevel(this, FName(*LevelList[CurrentLevelIndex + 1].GetLongPackageName()));
	}
}

void AChronosGameMode::BroadcastCounts()
{
	OnEnemyCountChanged.Broadcast(ActiveEnemies.Num());
}
```

- [ ] **Step 6: 实现 ChronosEnemy.h**

```cpp
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
```

- [ ] **Step 7: 实现 ChronosEnemy.cpp**

```cpp
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
```

- [ ] **Step 8: 构建 + 重启 + 运行测试**

构建（期望通过）→ 重启编辑器 → `AutomationTestToolset`：先 `DiscoverTests`，再 `ListTests` 过滤 "Chronos."，`RunTests(["Chronos.GameFlow.RegisterAndClear","Chronos.GameFlow.EnemyAimAndAutoEquip"])`，`GetTestResults` 期望 2/2 通过。

- [ ] **Step 9: 更新 docs/Architecture.md 的"C++ 单元"节**（追加 AChronosGameMode/AChronosEnemy 行）+ **Step 10: 提交**

```bash
git add Source/Chronos_New/GameFlow/ChronosGameMode.h Source/Chronos_New/GameFlow/ChronosGameMode.cpp Source/Chronos_New/Characters/ChronosEnemy.h Source/Chronos_New/Characters/ChronosEnemy.cpp Source/Chronos_New/Tests/ChronosGameFlowTests.cpp Source/Chronos_New/Chronos_New.Build.cs docs/Architecture.md
git commit -m "$(cat <<'EOF'
feat: 敌人基类与多关卡 GameMode（注册计数/清关/R重开）+ 自动化测试

Co-Authored-By: Claude Haiku 4.5 (1M context) <noreply@anthropic.com>
EOF
)"
```

---

### Task 3: C++ 玩家控制器——HUD 挂载与流程界面入口

**Files:**
- Create: `Source/Chronos_New/GameFlow/ChronosPlayerController.h`
- Create: `Source/Chronos_New/GameFlow/ChronosPlayerController.cpp`
- Modify: `docs/Architecture.md`

**Interfaces:**
- Consumes: Task 2 的 `AChronosGameMode` 全部公开接口。
- Produces（Task 7 依赖）：`AChronosPlayerController`（`HUDClass/LevelTransitionClass/VictoryClass/DefeatClass:TSubclassOf<UUserWidget>`、`FinishLevelTransition()`）。

- [ ] **Step 1: ChronosPlayerController.h**

```cpp
#pragma once

#include "GameFramework/PlayerController.h"
#include "ChronosPlayerController.generated.h"

class AChronosCharacter;
class UUserWidget;

/**
 * 玩家控制器：创建常驻 HUD 与流程界面（清关过场/胜负），
 * 监听 GameMode 清关与玩家死亡事件；R 键重开当前关（含死亡后）。
 */
UCLASS(Blueprintable)
class CHRONOS_NEW_API AChronosPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void SetupInputComponent() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chronos|UI")
	TSubclassOf<UUserWidget> HUDClass;

	/** 清关过场（SUPERHOT 闪烁字）。其内部动画/计时结束后应调用 FinishLevelTransition */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chronos|UI")
	TSubclassOf<UUserWidget> LevelTransitionClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chronos|UI")
	TSubclassOf<UUserWidget> VictoryClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chronos|UI")
	TSubclassOf<UUserWidget> DefeatClass;

	/** 过场结束：由 WBP_LevelTransition 蓝图调用 */
	UFUNCTION(BlueprintCallable, Category = "Chronos|Flow")
	void FinishLevelTransition();

protected:
	UFUNCTION()
	void HandlePlayerDeath(AChronosCharacter* Character);

	UFUNCTION()
	void HandleLevelCleared(bool bFinalLevel);

	void ShowOverlay(TSubclassOf<UUserWidget> WidgetClass);

	void RestartLevel_IfPermitted();

private:
	UPROPERTY()
	TObjectPtr<UUserWidget> HUD;

	UPROPERTY()
	TObjectPtr<UUserWidget> ActiveOverlay;

	bool bLevelFlowing = false;
};
```

- [ ] **Step 2: ChronosPlayerController.cpp**

```cpp
#include "GameFlow/ChronosPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Characters/ChronosCharacter.h"
#include "GameFlow/ChronosGameMode.h"

void AChronosPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (HUDClass)
	{
		HUD = CreateWidget<UUserWidget>(this, HUDClass);
		if (HUD)
		{
			HUD->AddToViewport();
		}
	}

	if (AChronosGameMode* GM = GetWorld()->GetAuthGameMode<AChronosGameMode>())
	{
		GM->OnLevelCleared.AddDynamic(this, &AChronosPlayerController::HandleLevelCleared);
	}
}

void AChronosPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// 玩家Pawn即 AChronosCharacter，死亡即失败（一击必杀规则）
	if (AChronosCharacter* ChronosPawn = Cast<AChronosCharacter>(InPawn))
	{
		ChronosPawn->OnCharacterDeath.AddUniqueDynamic(this, &AChronosPlayerController::HandlePlayerDeath);
	}
}

void AChronosPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// R 重开是流程级输入，与 Enhanced Input 的 IMC 解耦，直接用原始键绑定
	InputComponent->BindKey(EKeys::R, IE_Pressed, this, &AChronosPlayerController::RestartLevel_IfPermitted);
}

void AChronosPlayerController::RestartLevel_IfPermitted()
{
	if (AChronosGameMode* GM = GetWorld()->GetAuthGameMode<AChronosGameMode>())
	{
		GM->RestartLevel();
	}
}
```

后续函数（`ShowOverlay/HandleLevelCleared/HandlePlayerDeath/FinishLevelTransition`，声明均已在 .h 中）实现为：

```cpp
void AChronosPlayerController::HandlePlayerDeath(AChronosCharacter* Character)
{
	if (DefeatClass)
	{
		ShowOverlay(DefeatClass);
	}
}

void AChronosPlayerController::HandleLevelCleared(bool bFinalLevel)
{
	if (bFinalLevel)
	{
		if (VictoryClass)
		{
			ShowOverlay(VictoryClass);
		}
		return;
	}

	bLevelFlowing = true;
	if (LevelTransitionClass)
	{
		ShowOverlay(LevelTransitionClass);
	}
	else
	{
		FinishLevelTransition(); // 未配置过场则直接进下一关
	}
}

void AChronosPlayerController::FinishLevelTransition()
{
	if (ActiveOverlay)
	{
		ActiveOverlay->RemoveFromParent();
		ActiveOverlay = nullptr;
	}

	if (bLevelFlowing)
	{
		bLevelFlowing = false;
		if (AChronosGameMode* GM = GetWorld()->GetAuthGameMode<AChronosGameMode>())
		{
			GM->LoadNextLevel();
		}
	}
}

void AChronosPlayerController::ShowOverlay(TSubclassOf<UUserWidget> WidgetClass)
{
	if (ActiveOverlay)
	{
		ActiveOverlay->RemoveFromParent();
	}
	ActiveOverlay = CreateWidget<UUserWidget>(this, WidgetClass);
	if (ActiveOverlay)
	{
		ActiveOverlay->AddToViewport(10);
	}
}
```

- [ ] **Step 3: 构建（.h 变更 → 全量+重启）→ Step 4: 更新 Architecture.md + Step 5: 提交**

```bash
git add Source/Chronos_New/GameFlow/ChronosPlayerController.h Source/Chronos_New/GameFlow/ChronosPlayerController.cpp docs/Architecture.md
git commit -m "$(cat <<'EOF'
feat: 玩家控制器 HUD 挂载、清关/胜负界面流程、R 重开

Co-Authored-By: Claude Haiku 4.5 (1M context) <noreply@anthropic.com>
EOF
)"
```

---

### Task 4: 武器 BP 重父类 + 数据资产 + 删除 BP_Pistol

**Files:**（全部为编辑器资产操作，经 unreal-mcp）
- Modify: `/Game/Variant_Shooter/Blueprints/Pickups/BP_ShooterWeaponBase`
- Modify: `/Game/Variant_Shooter/Blueprints/Pickups/Weapons/BP_ShooterWeapon_Pistol|Rifle|GrenadeLauncher`
- Modify: `/Game/DataAssets/DA_Pistol`
- Create: `/Game/DataAssets/DA_Rifle`、`/Game/DataAssets/DA_GrenadeLauncher`、`/Game/DataAssets/DA_Enemy_Pistol`
- Delete: `/Game/Blueprints/Weapons/BP_Pistol`
- Modify: `docs/Architecture.md`

**Interfaces:**
- Consumes: Task 1 的 `OnFireVisuals/OnEquipVisuals` 事件、空 ProjectileClass 语义。
- Produces（Task 6/9 依赖）：三把可运行武器 BP + `DA_Pistol/DA_Rifle/DA_GrenadeLauncher/DA_Enemy_Pistol`（UWeaponDataAsset 实例）。

- [ ] **Step 1: describe_toolset BlueprintTools/AssetTools/DataAssetTools/ObjectTools**，确认 `set_parent/compile_blueprint/remove_variable/remove_function_graph`、资产创建/删除/保存、DataAsset 创建与属性写入的入参。

- [ ] **Step 2: 重父类 BP_ShooterWeaponBase → AChronosWeapon**

`BlueprintTools.set_parent`（blueprint=`/Game/Variant_Shooter/Blueprints/Pickups/BP_ShooterWeaponBase`，parent=`/Script/Chronos_New.ChronosWeapon`）→ `compile_blueprint`。预期成功；若有组件名冲突报错，读取错误信息并把 BP 侧同名组件重命名/删除后再编译（C++ 的 `WeaponMeshComponent` 必须保留，网格应设置在它上面）。

- [ ] **Step 3: 删除 BP 遗留开火逻辑**

在 BP_ShooterWeaponBase 上逐个 `remove_variable`：`Bullet Class`、`Current Bullets`、`Mag Size`、`Refire Rate`、`Full Auto`、`Time of Last Shot`、`Is Firing`、`Refire Timer`、`Aim Variance`、`Firing Recoil`。删除函数图 `Fire Bullet`（`remove_function_graph`）。保留：`Firing Montage`、FP/TP Anim Instance、`Shot Loudness/Noise Range/Noise Tag`（AI 噪声可后续接 AIPerception，暂留）、`Pawn Owner`、`Calculate Bullet Spawn Transform`（OnFireVisuals 里可能复用）→ 若编译报"变量被引用"，用 `find_nodes` 定位引用节点并 `delete_node` 后再删。三个子类各 `compile_blueprint` 并按编译器提示修复引用残留。

- [ ] **Step 4: 三个子类实现表现事件**

每个武器 BP（Pistol/Rifle/GrenadeLauncher）`add_event` 覆写 `OnFireVisuals`：节点序列 = 播放 `Firing Montage`（原 Montage 属性或换 DA 引用）+ 可选音效。`OnEquipVisuals` 先留空实现（仅事件入口，无节点）。GrenadeLauncher 额外在 `OnFireVisuals` 中生成 `/Game/Variant_Shooter/Blueprints/Pickups/Projectiles/BP_ShooterProjectile_Grenade`（SpawnActor from class + `Calculate Bullet Spawn Transform` 输出位姿 + 速度沿方向）。各 `compile_blueprint`。

- [ ] **Step 5: 数据资产**

用 DataAssetTools 以 `UWeaponDataAsset`（`/Script/Chronos_New.WeaponDataAsset`）为基创建 `DA_Rifle`、`DA_GrenadeLauncher`、`DA_Enemy_Pistol`（`/Game/DataAssets/` 下）；`ObjectTools.set_properties` 修改 `DA_Pistol`。网格/音效资产路径先经 AssetTools 搜索确认（在 `/Game/Variant_Shooter` 与 `/Game/Mannequins` 下找 `SKM_Pistol/SKM_Rifle/SKM_GrenadeLauncher` 与枪声 SoundWave）。字段（与 `WeaponDataAsset.h` 一致）：

| 字段 | DA_Pistol（改） | DA_Rifle | DA_GrenadeLauncher | DA_Enemy_Pistol |
|---|---|---|---|---|
| WeaponName | 手枪 | 步枪 | 榴弹发射器 | 敌用手枪 |
| WeaponMesh | SKM_Pistol | SKM_Rifle | SKM_GrenadeLauncher | SKM_Pistol |
| MuzzleSocketName | 用网格实查的枪口 socket（`SkeletalMeshTools` 查 socket 列表；找不到则 `Muzzle` 并在后续视情况调） | 同左 | 同左 | 同 DA_Pistol |
| HandSocketName | hand_rWeaponSocket（默认） | 同 | 同 | 同 |
| AmmoCount | 8 | 20 | 3 | 3 |
| FireRate | 5 | 8 | 1 | 1 |
| bAutomatic | false | true | false | false |
| BulletSpeed | 6000 | 8000 | 4000 | 6000 |
| ProjectileClass | `/Script/Chronos_New.ChronosProjectile` | 同 | **空**（BP 自管） | `/Script/Chronos_New.ChronosProjectile` |
| ThrowSpeed | 1500 | 1500 | 1500 | 1500 |
| FireMontage | 模板手枪 Montage（搜索确认） | `FP_Rifle_Shoot_Montage` | 模板对应 | 无（敌人无声） |

保存全部修改的资产。

- [ ] **Step 6: 武器 BP 各自绑定 DA**

`ObjectTools.set_properties` 设置每个武器 BP CDO 的 `WeaponData`：Pistol→DA_Pistol、Rifle→DA_Rifle、GrenadeLauncher→DA_GrenadeLauncher；各 compile + 保存。

- [ ] **Step 7: 删除 BP_Pistol**

AssetTools 删除 `/Game/Blueprints/Weapons/BP_Pistol`（规格已批准）。随后 `find_nodes`/引用检查 Lvl_FirstPerson 里是否有 BP_Pistol 实例（有则删除实例或替换为 BP_ShooterWeapon_Pistol）。

- [ ] **Step 8: PIE 冒烟**

PIE：地面放 BP_ShooterWeapon_Pistol → 走近 E 拾取（武器附手、准星交互提示出现）→ 左键连射（射速/弹药递减）→ 打空自动扔枪（CombatComponent 现成行为）→ 扔出的枪落地可再捡。GrenadeLauncher 生成验证爆炸。记录异常并修复后重跑。

- [ ] **Step 9: 更新 Architecture.md（蓝图资产/数据资产节）+ Step 10: 提交**（资产 .uasset 与文档一起 `git add` 具体路径）

---

### Task 5: 红色弹道——慢门增亮

**Files:**
- Modify: `Source/Chronos_New/Projectiles/Projectile.h/.cpp`
- Create: `/Game/Variant_Shooter/FX/NS_ChronosBulletTrail`（由模板尾迹 NS 复制改造）
- Modify: `docs/Architecture.md`

**Interfaces:**
- Consumes: `UTimeDilationSubsystem::OnDilationChanged(float)`。
- Produces: DA_Pistol/DA_Rifle 的 FireMontage 不变；投射物默认 TrailSystem 引用 NS_ChronosBulletTrail（Task 4 的 DA 可加 `ProjectileClass` 指向带尾迹的 BP 子类，或直接在 BP_ChronosProjectile 上设 TrailSystem——实施时取最小路径：新建 `BP_ChronosProjectile`（父 AChronosProjectile）设 TrailSystem=NS_ChronosBulletTrail，DA 的 ProjectileClass 指向它）。

- [ ] **Step 1: C++——订阅时间缩放并写入 Niagara 用户参数**

`Projectile.h` 添加：

```cpp
protected:
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
	UFUNCTION()
	void HandleTimeDilationChanged(float NewDilation);
```

`Projectile.cpp`：include `Subsystems/TimeDilationSubsystem.h`；`ActivateProjectile` 末尾追加：

```cpp
	if (UTimeDilationSubsystem* Time = GetWorld()->GetSubsystem<UTimeDilationSubsystem>())
	{
		if (!Time->OnDilationChanged.IsAlreadyBound(this, &AChronosProjectile::HandleTimeDilationChanged))
		{
			Time->OnDilationChanged.AddDynamic(this, &AChronosProjectile::HandleTimeDilationChanged);
		}
		HandleTimeDilationChanged(Time->GetCurrentDilation());
	}
```

新增：

```cpp
void AChronosProjectile::EndPlay(const EEndPlayReason::Type Reason)
{
	if (UTimeDilationSubsystem* Time = GetWorld()->GetSubsystem<UTimeDilationSubsystem>())
	{
		Time->OnDilationChanged.RemoveDynamic(this, &AChronosProjectile::HandleTimeDilationChanged);
	}
	Super::EndPlay(Reason);
}

void AChronosProjectile::HandleTimeDilationChanged(float NewDilation)
{
	// 慢门（值越小越慢）时尾迹更亮，强化"时间凝固的子弹"读感
	TrailComponent->SetVariableFloat(TEXT("User.TimeDilation"), NewDilation);
}
```

- [ ] **Step 2: Niagara 资产**

`NiagaraToolset`（先 `describe_toolset` 各 Niagara 子工具集 + `NiagaraToolset_Assets.FindNiagaraScripts` 搜模板尾迹）：定位模板中适合做拖尾的 NS（搜索 Trail/Projectile 关键词；无合适资产则用 NiagaraToolset_System 从空系统搭建：Sprite Renderer + 位置生成于上一帧 + 红色颜色模块）。复制为 `NS_ChronosBulletTrail`：颜色设为红（约 RGB(1, 0.08, 0.08)），添加用户参数 `User.TimeDilation`（float，默认 1.0），颜色亮度按 `1 / max(TimeDilation, 0.05)` 做缩放（节点可用 Normalize/Clamp 组合；若图形编辑受阻，改为 `User.TimeDilation` 直接驱动 Emissive 缩放的简化连线）。保存。

- [ ] **Step 3: BP_ChronosProjectile + DA 指向**

`BlueprintTools.create` 父类 `/Script/Chronos_New.ChronosProjectile` 生成 `BP_ChronosProjectile`（`/Game/Blueprints/Projectiles/`）；`ObjectTools.set_properties` 设 `TrailSystem=NS_ChronosBulletTrail`；Task 4 的 DA_Pistol/DA_Rifle/DA_Enemy_Pistol `ProjectileClass` 改指 `BP_ChronosProjectile_C`。保存、编译。

- [ ] **Step 4: 全量构建 + 重启（.h 变更）→ Step 5: PIE 验证**

PIE：正常速度弹道暗红；对墙开枪后立即停止输入（时间冻结）观察弹道变亮悬停。异常修复重跑。

- [ ] **Step 6: 更新 Architecture.md + Step 7: 提交**

---

### Task 6: NPC 重父类 + 红色材质 + StateTree 开火接线

**Files:**（编辑器资产 + 视 StateTree 结构的小 BP）
- Modify: `/Game/Variant_Shooter/Blueprints/AI/BP_ShooterNPC`
- Create: `/Game/Variant_Shooter/Characters/MI_Mannequin_Red`（材质实例）
- Create 或 Modify: StateTree Shoot 任务资产
- Modify: `/Game/DataAssets/DA_Enemy_Pistol`（如 Task 4 未定网格 socket 则此处校准）
- Modify: `docs/Architecture.md`

**Interfaces:**
- Consumes: Task 2 `AChronosEnemy::SetCombatTarget/EnemyWeaponData`；Task 4 武器接口链。
- Produces: 可作战的红色敌人（感知→面向→开火→走位）。

- [ ] **Step 1: 重父类**

`set_parent` BP_ShooterNPC → `/Script/Chronos_New.ChronosEnemy` → `compile_blueprint`。冲突排查同 Task 4 Step 2 模式。

- [ ] **Step 2: 红色材质**

`MaterialInstanceTools`（先 describe）：以模板 mannequin 身体材质（BP_ShooterNPC 网格 Material 列表实查）为父创建 `MI_Mannequin_Red`，VectorParameter（BaseColor）设红 (1, 0.05, 0.05, 1)；`ObjectTools.set_properties` 把 NPC BP CDO 网格的 `OverrideMaterials` 指到 MI。编译保存。

- [ ] **Step 3: 敌人武器数据落到 BP**

`ObjectTools.set_properties`：BP_ShooterNPC CDO `EnemyWeaponData = DA_Enemy_Pistol`。

- [ ] **Step 4: StateTree 实查与接线**

`StateTreeTools`（describe + inspect）读 `ST_Shooter` 与 `StateTreeTask_ShootAtTarget`。目标接线（按实查结果二选一）：
- **首选**：`StateTreeTask_ShootAtTarget` 是 Blueprint 任务资产 → `BlueprintTools.read_graph_dsl` 读图，把对目标的开火调用改为：`GetCombatComponent().GetCurrentWeapon()` 为空则先 `EquipTo` 不适用（敌人已自动装备）→ 直接 `IWeaponUser.FireAtTarget(TargetActorLocation)`（对 C++ 接口的 BlueprintCallable 调用：`FireAtTarget` 是 `AChronosWeapon` 的 BlueprintCallable，Cast 后调用）；并在任务进入时对 NPC 调 `SetCombatTarget(TargetActor)`（供瞄准方向）。`write_graph_dsl` 写回 + compile。
- **备选**：模板任务不可编辑 → 新建 BP StateTree 任务（父类 `StateTreeTaskBlueprintBase`，`/Game/Variant_Shooter/Blueprints/AI/StateTree/STTask_ChronosShoot`）实现上述逻辑，在 ST_Shooter 中替换原 Shoot 状态的任务。

同时确认 EQS 走位任务保留不动。

- [ ] **Step 5: 早期风险验证——时间冻结下 StateTree 是否静止**（规格 §9 风险表要求）

PIE：放 1 个 NPC，玩家完全不动不输入 3~5 秒。预期：全局时间缩放生效，NPC 的 StateTree（移动/转身/开火）同步近乎冻结；恢复输入后 NPC 立即继续行动。若 NPC 在冻结期仍明显移动/射击，检查其移动是否走了 `bIgnoreTimeDilation` 类路径或 StateTree 任务用了真实时间，记录并修复（必要时在该任务改用游戏时间）。

- [ ] **Step 6: PIE 验证（对抗全链路）**

PIE：放 1 个 NPC（视野内）+ 玩家空手。验证：NPC 面向玩家并开枪（玩家会被一击致死→Defeat 前提逻辑已具备）；NPC 弹尽后自动扔枪（CombatComponent 行为）；玩家捡枪反击 → NPC 死亡掉枪 + 慢镜。修复重跑。

- [ ] **Step 7: 更新 Architecture.md + Step 8: 提交**

---

### Task 7: UMG——HUD 与流程界面

**Files:**
- Create: `/Game/Blueprints/UI/WBP_HUD`、`WBP_LevelTransition`、`WBP_Victory`、`WBP_Defeat`
- Modify: `docs/Architecture.md`

**Interfaces:**
- Consumes: `AChronosPlayerController`（Task 3）四个 WidgetClass 槽位与 `FinishLevelTransition`；`UCombatComponent::OnWeaponChanged/OnAmmoChanged`；`AChronosWeapon::OnAmmoChanged`；`AChronosCharacter::OnInteractionFocusChanged`；`UTimeDilationSubsystem::OnDilationChanged`；`AChronosGameMode::OnEnemyCountChanged`。
- Produces: 四个可挂载 Widget。

- [ ] **Step 1: describe UMGToolSet**，按其工作流（list_properties → get/set_properties）创建。

- [ ] **Step 2: WBP_HUD**

元素（Canvas 根）：
- `Crosshair`（两条 Image 或 TextBlock "+"，居中）
- `InteractionText`（TextBlock，默认隐藏；绑 `OnInteractionFocusChanged(bHasFocus, Text)`：显示/隐藏 + 文本）
- `AmmoText`（TextBlock，右下；初始绑当前武器弹药，监听 pawn 的 CombatComponent OnWeaponChanged → 换绑 OnAmmoChanged → 文本 `"{Remaining} / {Max}"`；武器为空显示 `"--"`）
- `EnemyCountText`（左上；`Cast GetGameMode→AChronosGameMode`，绑 `OnEnemyCountChanged` → `"敌人数: {N}"`）
- `TimeIndicator`（小 ProgressBar 或 Image；绑 `OnDilationChanged`：`IsTimeSlowed` 概念下令其颜色/透明度反馈，简化为 FillAmount = Dilation）

蓝图图（BlueprintTools write_graph_dsl 或事件绑定）：`Construct` 时取 `GetOwningPlayerPawn` → Cast AChronosCharacter → 取组件绑委托。

- [ ] **Step 3: WBP_LevelTransition**

全屏黑底 + 居中大字 `SUPERHOT`（TextBlock，字号 ~120，白色）。事件图：`Construct` → `SetTimerByEvent 1.2s` → `Call FinishLevelTransition（GetOwningPlayerPlayerController Cast AChronosPlayerController）`。可选闪烁（Tween 透明度正弦）不阻塞交付。

- [ ] **Step 4: WBP_Victory / WBP_Defeat**

全屏半透明黑底；Victory 大字 `YOU WIN`（绿色）；Defeat 大字 `DEFEAT`（红色）+ 小字 `按 R 重试`。

- [ ] **Step 5: 保存 + Step 6: 更新 Architecture.md + Step 7: 提交**

---

### Task 8: GameMode/PlayerController BP 重父类与配置

**Files:**
- Modify: `/Game/Variant_Shooter/Blueprints/BP_ShooterGameMode`
- Modify: `/Game/Variant_Shooter/Blueprints/BP_ShooterPlayerController`
- Modify: `docs/Architecture.md`

**Interfaces:**
- Consumes: Task 3 C++ 控制器四槽位；Task 7 四个 WBP。

- [ ] **Step 1: 重父类**

`set_parent` BP_ShooterGameMode → `/Script/Chronos_New.ChronosGameMode` → compile；`set_parent` BP_ShooterPlayerController → `/Script/Chronos_New.ChronosPlayerController` → compile。检查两者原 CDO 覆写（DefaultPawnClass=BP_ShooterCharacter、PlayerControllerClass=BP_ShooterPlayerController）重父类后仍保留（ObjectTools.get_properties 验证）。

- [ ] **Step 2: 配置槽位**

PlayerController CDO：`HUDClass=WBP_HUD_C`、`LevelTransitionClass=WBP_LevelTransition_C`、`VictoryClass=WBP_Victory_C`、`DefeatClass=WBP_Defeat_C`。GameMode CDO：`LevelList=[]`（暂空，Task 9/10 填）。保存。

- [ ] **Step 3: PIE 验证**

PIE Lvl_FirstPerson：HUD 出现（弹药 --、敌人数 0）；杀死任意 placed NPC（若无 placed 实例，临时放一个验证后移除）→ 敌人数变化；空手撞敌弹死亡 → DEFEAT 界面 → R 重开。修复重跑。

- [ ] **Step 4: 更新 Architecture.md + Step 5: 提交**

---

### Task 9: L01 关卡改造 + 全链路 PIE

**Files:**
- Modify: `/Game/FirstPerson/Lvl_FirstPerson`
- Modify: `/Game/Variant_Shooter/Blueprints/BP_ShooterGameMode`（LevelList 第一项）
- Modify: `docs/Architecture.md`

**Interfaces:**
- Produces: 可完整游玩的第一关（默认启动图）。

- [ ] **Step 1: 布置**

SceneTools 加载 Lvl_FirstPerson；清理模板遗留靶子/无关 Actor；放置：
- `BP_ShooterWeapon_Pistol` ×1（玩家出生点附近地面，作为初始拾取）
- `BP_ShooterNPC` ×2（掩体后 + 通道口，间距拉开）
- 玩家 PlayerStart 确认存在且朝向战斗区
保存关卡。

- [ ] **Step 2: GameMode LevelList 配置**

`ObjectTools.set_properties`：BP_ShooterGameMode `LevelList=[/Game/FirstPerson/Lvl_FirstPerson]`（当前单关）。保存。

- [ ] **Step 3: 全链路 PIE**

剧本：出生→拾枪→击杀 2 敌（每杀慢镜+掉枪）→敌人数归零。注意：本步 `LevelList` 只配了当前一关，`IsFinalLevel()` 为 true，清关后应直接出现 **WBP_Victory（YOU WIN）** 而非过场——这是预期行为（Task 8 已配好 VictoryClass）；过场→下一关的链路到 Task 10 配三项列表后再验证。随后主动送死→DEFEAT→R 重开→再通一次。每项记录通过/失败。用 EditorAppToolset 的 PIE 控制与 LogsToolset 看报错。

- [ ] **Step 4: 修复发现的问题并重跑** → **Step 5: 更新 Architecture.md（关卡节）+ Step 6: 提交**

---

### Task 10: L02/L03 + 关卡列表 + 完整流程回归

**Files:**
- Create: `/Game/Maps/Lvl_Arena02`、`/Game/Maps/Lvl_Arena03`（由 Lvl_FirstPerson 复制改造）
- Modify: `/Game/Variant_Shooter/Blueprints/BP_ShooterGameMode`（LevelList 三项）
- Modify: `docs/Architecture.md`

- [ ] **Step 1: 复制关卡**

AssetTools duplicate `/Game/FirstPerson/Lvl_FirstPerson` → `/Game/Maps/Lvl_Arena02`、`/Game/Maps/Lvl_Arena03`。

- [ ] **Step 2: L02 布置**

加载 Lvl_Arena02：玩家出生旁放 `BP_ShooterWeapon_Rifle`（DA_Rifle 全自动）；NPC ×3~4（其中 1~2 个用 rifle 数据——新建 `DA_Enemy_Rifle`（Task 4 模式）或直接复制 NPC 改 `EnemyWeaponData`）；保留 PlayerStart。保存。

- [ ] **Step 3: L03 布置**

Lvl_Arena03：`BP_ShooterWeapon_GrenadeLauncher` + 手枪备选；NPC ×5~6 混合武器。保存。

- [ ] **Step 4: LevelList 三项**

BP_ShooterGameMode `LevelList=[Lvl_FirstPerson, Lvl_Arena02, Lvl_Arena03]`。保存。

- [ ] **Step 5: 完整回归 PIE**

三关连打：L01→过场→L02→过场→L03 清空→YOU WIN；中途失败 R 重开当前关（进度正确）。投掷武器砸死敌人（600 阈值）至少验证一次。全记录。

- [ ] **Step 6: 修复重跑 → Step 7: 更新 Architecture.md → Step 8: 提交**

---

### Task 11: 交接文档终版与收尾回归

**Files:**
- Modify: `CLAUDE.md`（根目录，新建）
- Modify: `docs/Architecture.md`（终校）
- Modify: `docs/superpowers/plans/2026-08-29-chronos-migration.md`（勾选全部完成项）

- [ ] **Step 1: 写根目录 CLAUDE.md**

内容：项目定位（SUPERHOT 风格求职项目）、C++/蓝图分工原则、构建运行命令、MCP 使用纪律（串行/保存/重启流程）、关键入口文件索引（Characters/Weapons/GameFlow/Subsystems）、"改 UI 去哪找 WBP""改敌人行为去哪找 ST_Shooter"等导航、指向 docs/Architecture.md 与规格/计划文档。

- [ ] **Step 2: Architecture.md 终校**：与实际资产/类清单逐项核对（MCP `list` 类工具抽查），删除过时描述。

- [ ] **Step 3: 最终回归**：Task 10 Step 5 剧本完整重跑一遍确认无回归。

- [ ] **Step 4: 提交**

```bash
git add CLAUDE.md docs/Architecture.md docs/superpowers/plans/2026-08-29-chronos-migration.md
git commit -m "$(cat <<'EOF'
docs: 交接文档终版与迁移完成回归记录

Co-Authored-By: Claude Haiku 4.5 (1M context) <noreply@anthropic.com>
EOF
)"
```
