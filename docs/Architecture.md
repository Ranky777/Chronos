# Chronos_New 架构与交接文档

> 本文档供后续 AI 会话接手项目，随每次改动同步更新。设计规格：docs/superpowers/specs/2026-08-29-chronos-migration-design.md

> **2026-09 扩展轮**：新增设置 / 存档 / 程序化音频 / 打击感 / 程序化后处理 / 统计评级 六套系统，
> 详见文末「扩展轮：设置·存档·音频·打击感·后处理·统计」章节。

## 项目定位
SUPERHOT 风格第一人称 Arena Shooter 求职项目。C++ 系统 + 蓝图表现混合。

## C++ 单元
（随任务填充：类名 / 文件 / 职责一句话）

| 类 | 文件 | 职责 |
|---|---|---|
| AChronosCharacter | Source/Chronos_New/Characters/ChronosCharacter.h/.cpp | 玩家角色基类：移动/跳跃/蹲伏、Enhanced Input 绑定、第一人称相机；**表现层 API**：GetFirstPersonMesh（按名字模糊匹配 FP 手臂网格）、SetWeaponAnimClasses/RestoreDefaultAnimClasses（装备时把武器 ABP 套到 FP+TP 网格，脱手时还原 DefaultFirstPersonAnimClass/DefaultThirdPersonAnimClass）、PlayWeaponMontage（FP/TP 双通道播放）；属性 DefaultFirstPersonAnimClass、DefaultThirdPersonAnimClass、ThrowMontage；右键投掷走 ThrowCurrentWeapon 并同步播 ThrowMontage |
| AChronosWeapon | Source/Chronos_New/Weapons/ChronosWeapon.h/.cpp | 武器 Actor 三态机（InWorld / Equipped / AtRest）：InitFromData、EquipTo、ThrowWeapon、DropWeapon、FireAt/FireAtTarget；**投掷自伤免疫**（见"三个已修 Bug"）；EnterEquippedState 隐藏物理网格并显示 BP 视觉网格，EnterWorldState 反之；OnEquipVisuals / OnUnequipVisuals 为 BlueprintImplementableEvent，具体挂接与动画交给蓝图 |
| UCombatComponent | Source/Chronos_New/Components/CombatComponent.h/.cpp | 角色侧武器持有：EquipWeapon / ThrowCurrentWeapon / DropCurrentWeapon / GetCurrentWeapon，桥接 AChronosWeapon 与角色 |
| UHealthComponent | Source/Chronos_New/Components/HealthComponent.h/.cpp | 血量：一击必杀语义，OnDeath 广播供 GameMode / PlayerController 监听 |
| AChronosGameMode | Source/Chronos_New/GameFlow/ChronosGameMode.h/.cpp | 敌人注册/注销计数（OnEnemyCountChanged）、清关广播（OnLevelCleared）、多关卡推进（LevelList/RestartLevel/LoadNextLevel）、击杀触发子弹时间 |
| AChronosEnemy | Source/Chronos_New/Characters/ChronosEnemy.h/.cpp | 敌人基类：出生按 EnemyWeaponData 自动生成并装备武器、SetCombatTarget/GetCombatTarget、无相机时瞄准回退到朝目标方向、BeginPlay 自注册到 AChronosGameMode；Task 6 增 GetPerceivedTarget（AI 感知→玩家 Pawn 兜底）与 HasLineOfSightToCombatTarget（视锥半角 35°+5 条垂直射线），供 StateTree 任务侧解析目标与视线门控；**DefaultWeaponClass**（TSoftClassPtr\<AChronosWeapon\>，config）决定出生生成哪把武器——必须指向带 FP/TP 视觉网格的蓝图武器类，否则敌人空手（详见"敌人空手"条目） |
| AChronosPlayerController | Source/Chronos_New/GameFlow/ChronosPlayerController.h/.cpp | 玩家控制器：常驻 HUD 与流程界面（HUD/LevelTransition/Victory/Defeat 四个 WidgetClass）、监听 OnLevelCleared/OnCharacterDeath、FinishLevelTransition 推进下一关、R 键重开当前关 |
| AChronosProjectile | Source/Chronos_New/Projectiles/Projectile.h/.cpp | 池化子弹：直线无重力飞行、一击必杀（UHealthComponent）、命中物理体施加冲量；Task 5 起订阅 UTimeDilationSubsystem::OnDilationChanged（ActivateProjectile 绑定 + 立即同步当前值，EndPlay 解绑），HandleTimeDilationChanged 把当前时间缩放写入 Niagara 用户参数 User.TimeDilation，实现慢门时尾迹增亮（1/max(dilation,0.05)） |
| UTimeDilationSubsystem | Source/Chronos_New/Subsystems/TimeDilationSubsystem.h/.cpp | SUPERHOT 式全局时间门：真实时间戳判定输入活跃窗口（0.1s）——无输入→SlowDilation(0.05)、有输入→1.0，用真实时差插值避免膨胀失真；击杀子弹时间 Override、玩家死亡强制恢复满速。开局即慢门（首帧直接 ApplyDilation(SlowDilation)，LastInputRealTime 初值 0 = "从未输入"），世界等你动 |

自动化测试：Source/Chronos_New/Tests/ChronosGameFlowTests.cpp（编辑器 Automation "Chronos.GameFlow.*" 2 用例：注册清关计数 / 出生自动装备+瞄准；监听器 UChronosTestListener 在 Tests/ChronosTestListener.h）

## 蓝图资产
| 资产 | 父类 | 用途 |
|---|---|---|
| /Game/Variant_Shooter/Blueprints/Pickups/BP_ShooterWeaponBase | AChronosWeapon | 武器基类 BP：旧模板开火逻辑已删（10 变量 + Fire Bullet 函数），仅保留 Firing Montage、FP/TP Anim Instance、噪声参数、Pawn Owner、Calculate Bullet Spawn Transform，及 5 个空存根事件（BeginPlay/StartFiring/StopFiring/ActivateWeapon/DeactivateWeapon，供跨 BP 编译兼容）。**EventGraph 新增 EventOnEquipVisuals / EventOnUnequipVisuals 实现**，是所有武器共用的"挂接 + 换 ABP + 播装备蒙太奇"链路（详见下节） |
| /Game/Variant_Shooter/Blueprints/Pickups/Weapons/BP_ShooterWeapon_Pistol | BP_ShooterWeaponBase | 手枪：CDO WeaponData=DA_Pistol；OnFireVisuals 播 MM_Pistol_Fire_Montage（FP 手臂）。**子类里的空 OnEquipVisuals 覆盖已删除**，否则会屏蔽基类挂接逻辑 |
| /Game/Variant_Shooter/Blueprints/Pickups/Weapons/BP_ShooterWeapon_Rifle | BP_ShooterWeaponBase | 步枪：CDO WeaponData=DA_Rifle；OnFireVisuals 播 FP_Rifle_Shoot_Montage；空 OnEquipVisuals 覆盖已删除 |
| /Game/Variant_Shooter/Blueprints/Pickups/Weapons/BP_ShooterWeapon_GrenadeLauncher | BP_ShooterWeaponBase | 榴弹发射器：CDO WeaponData=DA_GrenadeLauncher（ProjectileClass 为空，弹道走 BP）；OnFireVisuals 在枪口 socket 位姿处 SpawnActor BP_ShooterProjectile_Grenade + PlaySound2D；空 OnEquipVisuals 覆盖已删除 |
| /Game/Blueprints/Weapons/BP_Pistol | （已删除） | Task 3 临时武器 BP，Task 4 已删除（无引用、无关卡实例） |
| /Game/Blueprints/Projectiles/BP_ChronosProjectile | AChronosProjectile | 子弹 BP：CDO TrailSystem=NS_ChronosBulletTrail；DA_Pistol/DA_Rifle/DA_Enemy_Pistol 的 ProjectileClass 均指向它 |

## 武器表现链路（FP/TP）——改这块前必读

> **血泪教训**：`OnEquipVisuals` / `OnUnequipVisuals` 是 `BlueprintNativeEvent`，
> C++ 有默认实现（动画实例取自 `WeaponData`），BP 覆写。
> 这条默认实现是"敌人生成了裸武器"时表现仍然正常的最后保险 —— 别删。

### 1. AChronosWeapon 的网格分工
- `WeaponMeshComponent`（C++ 创建的 USkeletalMeshComponent，Actor Root）：世界态 = 可见掉落物 + 物理体；**装备态隐藏**，只留枪口 socket 供弹道解算。
- `FP_Weapon` / `TP_Weapon`（模板 BP 自带的视觉网格）：装备态显示，世界态隐藏。C++ 不硬编码组件名，`SetVisualMeshesHidden` 把"除 WeaponMeshComponent 外所有 SkeletalMeshComponent"一律当视觉网格。
- 防御性回退：若 BP 没有视觉网格（裸 `AChronosWeapon`），装备态**不隐藏**物理网格，避免"完全空手"。
- `TP_Weapon` 的 `bOwnerNoSee = true`（已设），本地玩家看不到第三人称枪 —— "双枪"问题的一半。

### 2. EventOnEquipVisuals（BP_ShooterWeaponBase 覆写，子类不要再覆盖）
`GetOwner → CastToChronosCharacter` → `AttachComponentToComponent(FP_Weapon→FP 手, TP_Weapon→TP 身体, Socket="HandGrip_R")` → `SetWeaponAnimClasses(武器 BP 的 FP/TP 变量)` → `PlayWeaponMontage(WeaponData.EquipMontage)`。
FP 手臂网格没有 `hand_rWeaponSocket`，C++ 回退到 `HandGrip_R`，与蓝图一致。

### 3. EventOnUnequipVisuals(PreviousHolder)
`RestoreDefaultAnimClasses(PreviousHolder)` → Detach FP_Weapon / TP_Weapon。

### 4. 蒙太奇资产（本轮新建，/Game/Variant_Shooter/Anims/）
模板只有 2 个真正的 Montage（开火的），装备/投掷蒙太奇是**复制现有 Montage 再改 slotAnimTracks** 造的：

| 资产 | 复制自 | 指向动画 | Slot |
|---|---|---|---|
| MM_Pistol_Equip_Montage | MM_Pistol_Fire_Montage | MM_Pistol_Equip | Arms |
| MM_Rifle_Equip_Montage | FP_Rifle_Shoot_Montage | MM_Rifle_Equip | Arms |
| MM_Throw_Montage | FP_Rifle_Shoot_Montage | MM_Attack_01 | Arms |

⚠️ `sequenceLength` 是只读派生值，改不了，用 `animPlayRate` 把长动画压进原 Montage 时长（Rifle Equip 3.125 倍速，**偏快，需人工复核**）。Slot 名必须是 `Arms`。

## 敌人 AI（Task 6）
- BP_ShooterNPC（/Game/Variant_Shooter/Blueprints/AI/BP_ShooterNPC）重父类 → AChronosEnemy；CDO 配置 OverrideMaterials=[MI_Mannequin_Red]、EnemyWeaponData=DA_Enemy_Pistol。BeginPlay 覆写已整体移除（原覆写丢失父调用，会跳过 C++ 的自动装武器 + GameMode 注册；移除后原生 BeginPlay 生效）。EventGraph 保留模板事件：AnyDamage（CurrentHP 归零 → Die）、Die（OnDeath 广播 + Ragdoll + 10s 后 DestroyActor；模板 IncrementTeamScore 已随旧父类消失而删除该调用）、AttachWeaponMeshes / StartShooting / StopShooting / OnSemiWeaponRefire。
- MI_Mannequin_Red（/Game/Variant_Shooter/Characters/）：以模板 mannequin 身体材质为父的实例；模板身体材质无 BaseColor 参数，改设 Paint Tint 为红 (1, 0.05, 0.05, 1) 达成同效果。
- StateTree（模板 ST_Shooter / EQS 走位任务未改动）：StateTreeTask_ShootAtTarget 进状态时 SetCombatTarget(Character, GetPerceivedTarget(Character))，再经 HasLineOfSightToCombatTarget 门控 → CastToChronosWeapon → FireAtTarget(目标位置)；ExitState 调 StopShooting。
- StateTreeCondition_HasLineOfSightToTarget 刻意改为恒 true，视线门控移到任务侧。根因：ST 蓝图进入条件在实例数据填充前求值（条件的 Character 输入运行时恒空，探针矩阵证实），且感知链路在本项目从未触发（全项目无 PerceptionStimuliSource，LogAIPerception Verbose 零输出）；ST 蓝图上下文里 GetPlayerPawn 等世界上下文函数也拿不到玩家。目标解析与视线判定全部下沉到 C++（见 AChronosEnemy 行）。
- BP_ShooterAIController 未改动（模板原样，含感知组件配置）。
- **DefaultWeaponClass = BP_ShooterWeapon_Pistol**。全局默认值写在 `Config/DefaultEngine.ini` 的 `[/Script/Chronos_New.ChronosEnemy]`；但因为蓝图 CDO 会序列化自己的值并覆盖 ini 默认，**实际生效值必须设在 BP_ShooterNPC 的类默认值上**（已设）。改敌人武器时两处都要动，或只改 BP（BP 优先）。

## 特效资产
| 资产 | 用途 |
|---|---|
| /Game/Variant_Shooter/FX/NS_ChronosBulletTrail | 子弹尾迹：红色（约 RGB(1,0.08,0.08)），用户参数 User.TimeDilation（float，默认 1.0）经 1/Max(TimeDilation,0.05) 缩放颜色——正常速度暗红、时间冻结（dilation→0.05）时增亮至 20 倍，强化"凝固子弹"读感；由 AChronosProjectile 在 OnDilationChanged 时写入 |

## 数据资产
（UWeaponDataAsset 实例，/Game/DataAssets/ 下；字段见 Source/Chronos_New/Weapons/WeaponDataAsset.h）

| 资产 | WeaponName | 网格 | 弹药/射速/全自动/弹速 | ProjectileClass | FireMontage | EquipMontage |
|---|---|---|---|---|---|---|
| DA_Pistol | 手枪 | SKM_Pistol | 8 / 5 / 否 / 6000 | BP_ChronosProjectile | MM_Pistol_Fire_Montage | MM_Pistol_Equip_Montage |
| DA_Rifle | 步枪 | SKM_Rifle | 20 / 8 / 是 / 8000 | BP_ChronosProjectile | FP_Rifle_Shoot_Montage | MM_Rifle_Equip_Montage |
| DA_GrenadeLauncher | 榴弹发射器 | SKM_GrenadeLauncher | 3 / 1 / 否 / 4000 | 空（BP 自管生成榴弹） | FP_Rifle_Shoot_Montage | MM_Rifle_Equip_Montage |

**敌人与玩家共用同一份武器数据**。曾经存在 `DA_Enemy_Pistol`（弹药 3 / 射速 1），已删除——
"手枪"就是"手枪"，敌人拿的和你捡的是同一把枪（SUPERHOT 即如此）。
敌人侧的难度差异现在放在 `AChronosEnemy` 上，而不是复制一份数据资产：

| 参数 | 默认值 | 说明 |
|---|---|---|
| `FireInterval` | 1.0s | 敌人开火间隔（原来靠 DA 的 FireRate=1 实现） |
| `StartingAmmoOverride` | 3 | 敌人出生弹药；0 = 沿用数据资产（原来靠 DA 的 AmmoCount=3 实现） |

**想让某个敌人拿别的枪**：`DefaultWeaponClass` 已放开为 `EditAnywhere`，
直接在关卡里选中该敌人 → Details → `Chronos|AI` → `Default Weapon Class` 换成
`BP_ShooterWeapon_Rifle` 即可，武器数据（弹药/网格/射速）会跟着这个类走。

⚠️ `EnemyWeaponData` 必须保持非空（当前 = DA_Pistol）：它是**兜底**。
`SpawnDefaultWeapon` 的规则是"武器蓝图自带的数据优先，EnemyWeaponData 兜底"；
若两边都空，`InitFromData(nullptr)` 会让武器既没有数据也没有弹药，
表现为**敌人举着枪却完全不开火**（`CanFire` 因 `RemainingAmmo == 0` 恒假）。
回归用例：`Chronos.GameFlow.EnemySpawnedWeaponIsLoaded`。

⚠️ 改这类"类默认值"时注意：ini 全局默认会被 BP CDO 覆盖，BP CDO 又会被**关卡里的 Actor 实例**覆盖
（`EnemyWeaponData` 是 `EditDefaultsOnly`，实例上的值改不动，只能删掉实例重建）。
迁移时就踩了这个：CDO 已改成 DA_Pistol，关卡里那个敌人仍抱着 DA_Enemy_Pistol 不放。

MuzzleSocketName=Muzzle（实测三把网格均有该 socket）、HandSocketName=hand_rWeaponSocket、ThrowSpeed=1500 四资产一致。关卡直放的武器实例由 AChronosWeapon::BeginPlay 检测到 WeaponData+无网格+零弹药后自动 InitFromData。

## 关卡

三关顺序由 `BP_ShooterGameMode` 的 `LevelList` 驱动，清空当前关全部敌人后自动过场进入下一关：

| # | 关卡 | 敌人 | 玩家可用武器 |
|---|---|---|---|
| L01 | `/Game/FirstPerson/Lvl_FirstPerson` | 2（全手枪） | 手枪、步枪各一把（地面拾取） |
| L02 | `/Game/Maps/Lvl_Arena02` | 4（2 手枪 + 2 步枪） | 同 L01 布局 |
| L03 | `/Game/Maps/Lvl_Arena03` | 5（2 手枪 + 3 步枪） | 同 L01 布局 |

L02/L03 由 L01 复制而来，新增敌人放在 `(700,450)`、`(-550,780)`、`(650,-400)`、`(-700,300)`、`(200,900)`
附近——**坐标是脚本按经验给的，第一版大概率需要你在编辑器里微调**（可能贴墙或悬空）。
想让某个敌人换武器：选中它 → Details → `Chronos|AI` → `Default Weapon Class`。

## 已修 Bug（根因 + 防线）

### 1. 右键投掷 / 弹药耗尽自动丢弃 → 自杀
- 根因：`ThrowWeapon_Implementation` 把 `LastThrower.Get()` 传给 `Throw()`，而 `EquipTo` 会先把 `LastThrower` 置空 —— 投掷者身份在进入世界前就丢了，`OnWeaponBeginOverlap` 拿到的 Thrower 恒为空，自伤免疫完全失效。
- 修复：引入 `CurrentHolder`（`TWeakObjectPtr<AChronosCharacter>`），`EquipTo` 时赋值、`EnterWorldState` 时清空；`ThrowWeapon` / `DropWeapon` 一律用 `CurrentHolder` 作为免疫对象。`LastThrower` 保留给"投掷后能否立刻捡回"的判定。
- 双重防线：`bHasHitWorldSinceThrown`（`OnWeaponHit` 置 true）+ `ThrowerImmunityDuration`（默认 0.35s）。**未撞击任何物体前永久免疫投掷者**，撞击后才切到 0.35s 时间窗。

### 2. 第一人称看到两把枪
- 根因：装备态下 C++ 的 `WeaponMeshComponent`（物理/掉落用）与模板 BP 的 `FP_Weapon` 同时可见。
- 修复：`EnterEquippedState` 隐藏 `WeaponMeshComponent` 并显示 BP 视觉网格（世界态反之）；`TP_Weapon` 设 `bOwnerNoSee=true`，本地玩家看不到第三人称枪。
- 副作用已处理：隐藏物理网格后，若武器没有 BP 视觉网格（如裸 `AChronosWeapon`）就会彻底空手 —— 故 `SetVisualMeshesHidden` 带防御性回退（无视觉网格时不隐藏），且敌人改为生成蓝图武器类。

### 3. 敌人空手
- 根因：`SpawnDefaultWeapon` 原先 `SpawnActor<AChronosWeapon>(AChronosWeapon::StaticClass())` —— 裸 C++ 类没有 FP/TP 视觉网格，装备态隐藏物理网格后敌人手里什么都没有。
- 修复：新增 `DefaultWeaponClass`，默认 `BP_ShooterWeapon_Pistol`。

### 4. 拾取敌人掉落的枪不切换 ABP（第二轮的 2 号问题）
- 现象：预置武器拾取正常，敌人掉落的枪拾取后角色仍是徒手姿势。
- 根因：**敌人生成的是裸 C++ 类 `AChronosWeapon`，不是 `BP_ShooterWeapon_Pistol`**。`DefaultWeaponClass` 是 `TSoftClassPtr`，一旦软引用加载失败就会静默回退到 `AChronosWeapon::StaticClass()`；裸类没有 BP 的 `OnEquipVisuals` 实现（`BlueprintImplementableEvent` 无实现即空函数），于是既不挂接网格也不切换 ABP。
- 修复（双保险）：
  1. `SpawnDefaultWeapon` 在软引用为空时先 `LoadClass` 模板手枪作为硬回退，**绝不退化成裸类**。
  2. `OnEquipVisuals` / `OnUnequipVisuals` 改为 `BlueprintNativeEvent`，C++ 提供默认实现：从 `WeaponData` 的 `FirstPerson/ThirdPersonAnimClass` 切换动画实例并播 EquipMontage。这样即便哪天又生成了裸武器，表现依然正确。
- 配套：`UWeaponDataAsset` 新增 `FirstPersonAnimClass` / `ThirdPersonAnimClass` 两个字段（四个 DA 均已配置）。这是 C++ 默认实现的动画来源 —— **新增武器时记得填，否则裸武器路径会退化**。
- 回归测试：`Chronos.Weapon.AnimClassOnEquip`（见下）覆盖"装备→脱手→再装备"与"敌人武器→玩家拾取"两条路径。

### 5. 子弹尾迹来回曲折（第二轮的 1 号问题）
- 根因：子弹走对象池，`TrailComponent->Activate(true)` 不会清除上一发 Ribbon 的采样点，复用时从旧位置向新位置连线 → Z 字形尾迹（能命中正确位置，只是轨迹画花了）。
- 修复：`ActivateProjectile` 改用 `ReinitializeSystem()`（彻底重建模拟状态），`DeactivateProjectile` 改用 `DeactivateImmediate()`。注意 `ReinitializeSystem` 会清掉用户参数，因此 `User.TimeDilation` 必须在其后重新写入（当前代码顺序已保证：先 Reinitialize，末尾 `HandleTimeDilationChanged`）。

### 4a. 退出时 `Accessed None ... CallFunc_Array_Random_OutItem` + 死亡被自动重生

- 现象：退出 PIE 时报
  `Blueprint Runtime Error: "Accessed None trying to read property CallFunc_Array_Random_OutItem"`，
  节点在 `BP_ShooterPlayerController` 的 `SpawnActor BP Shooter Character`。
- 根因：模板遗留的**死亡自动重生**逻辑。`EventOnPossess` 给 Pawn 绑了 `OnDestroyed`，
  Pawn 销毁时去`GetAllActorsOfClassWithTag(PlayerStart, "BLUE"/"RED")`，
  再 `RandomArrayItem` 随机选一个出生点重生。
  我们的 PlayerStart **没有这些 Tag** → 数组为空 → 随机取到 None → 读取其 Transform 报错。
  而且这套重生跟"死亡 → DEFEAT → 按 R 重开"的设计直接冲突。
- 修复：删除 `EventOnPossess` 的 `AssignOnDestroyed` 以及重生链
  （`GetAllActorsWithTag` → `RandomArrayItem` → `SpawnActor` → `Possess`）。
  保留输入映射与 UI 更新事件（后者有 `IsValid` 保护）。

👉 模板的**分屏/队伍重生**逻辑是给多人对战用的，单人对抗项目必须整段摘掉，
   只删报错的那一个节点会留下半截链路（比如 `Possess(None)`）。

### 4b. 步枪按一下就不停开枪（应是按住才连发）

- 现象：按一次鼠标，步枪会一直打到弹药耗尽，松手不停。
- 根因：开火输入只绑了 `ETriggerEvent::Started`，**从没绑 `Completed`**，
  而 `IWeaponUser::StopFiring` 在整个工程里**没有任何调用点**。
  于是全自动连发标志 `bIsFiring` 被置 true 后再也没人清除。
- 修复：新增 `AChronosCharacter::OnFireCompleted`，绑到 `IA_Fire` 的 `Completed`，
  调 `IWeaponUser::Execute_StopFiring`。
- 兜底：武器脱手（投掷/丢弃/死亡）时 `EnterWorldState` 也会置 `bIsFiring=false`，
  避免松开事件丢失时卡在连发状态。

👉 教训：**加了"按住"状态就要同时保证"松开"路径存在**。
`grep StopFiring` 只在接口声明里出现、没有调用点，就是这个 bug 的直接证据。

### 5. 敌人打队友却打玩家时不开枪（放第二个敌人才暴露）

- 现象：场上只有一个敌人时正常；放下第二个敌人后，敌人开始互相开火，
  而面朝玩家时"会瞄准但不开枪"，直到另一个敌人进入视野才击发。
- 根因：`AChronosEnemy::GetPerceivedTarget()` 直接返回
  `Perception->GetCurrentlyPerceivedActors(...)[0]`。模板的 AI 感知按**角色基类**配置、不区分敌我，
  所以第二个敌人也会进感知列表；取到的 [0] 是谁取决于感知顺序，于是同伴可能被当成交战目标。
  一个敌人时列表里只有玩家，所以问题一直潜伏着。
- 修复：
  - `GetPerceivedTarget()` 只认玩家 —— 玩家被感知到才返回玩家，其余感知物（含同伴）一律忽略；
    没有感知组件时（自动化测试）无条件返回玩家。
  - `SetCombatTarget()` 增加防御：传入 `AChronosEnemy` 一律拒绝。
- 回归用例：`Chronos.GameFlow.EnemyRejectsAllyAsTarget`。

👉 教训：**AI 感知列表不等于敌人列表**。模板的感知配置按基类走，
多人/多敌场景下直接取 `[0]` 必然出错，必须显式过滤同类 / 只认真正的对手。

### 6. StateTree 报 `bValidContextRequirements` / "it's now invalid"（敌人死后持续刷屏）

- 现象：游戏中隔一段时间就刷
  `Ensure condition failed: bValidContextRequirements ... The tree started with a valid context and it's now invalid`。
- 根因：`HandleDeath` 只调了 `Controller->UnPossess()`，**没有停掉 AI 逻辑**。
  `UStateTreeComponent` 继承自 `UBrainComponent`，它的上下文是「Actor + Controller」；
  UnPossess 以及蓝图里 10 秒后的 `DestroyActor` 会让上下文失效，而 StateTree 仍在 Tick。
- 修复：`HandleDeath` 里先 `BrainComponent->StopLogic("Death")` 再 `UnPossess()`。
  附带好处：`StopLogic` 会走一遍 ExitState，正好触发 `StopShooting`，连发计时器也随之清理。
- 回归测试：`Chronos.GameFlow.EnemyDeathStopsAILogic`（给敌人挂真实 `AAIController` 再杀死）。

👉 通用教训：**销毁/脱离任何由 Controller 驱动的 Actor 前，先停掉它的 BrainComponent**
（行为树同理），否则行为逻辑会拿着失效上下文继续 Tick。

### 7. StateTreeTask_ShootAtTarget 编译失败（顺带修）
- 现象：`Could not find a function named "Stop Shooting" in 'BP_ShooterNPC_C'` —— **敌人根本不会射击**。
- 根因：`StartShooting` / `StopShooting` 原本在模板的 NPC 父类里，NPC 重父类为 `AChronosEnemy` 后丢失；StateTree 任务的节点还留着失效的 pin，单纯补函数不够，必须删节点重建。
- 修复：C++ `AChronosEnemy` 新增 `StartShooting()` / `StopShooting()`（`BlueprintCallable`），实现按武器射速定时连发、视线丢失自动停火；StateTree 任务里删掉旧的 `Stop Shooting` 节点并重连到 `Chronos|AI|StopShooting`。

## 构建与运行
- 引擎：UE 5.8（EngineAssociation "5.8"，安装于 D:/Unreal/UE_5.8）
- **构建必须加 `-NoUBA`**：
  ```
  D:/Unreal/UE_5.8/Engine/Build/BatchFiles/Build.bat Chronos_NewEditor Win64 Development ^
    -project="C:/Users/cuilongqi/Desktop/UEProjects/Chronos_New/Chronos_New.uproject" -WaitMutex -NoUBA
  ```
  本机 UBA（Unreal Build Accelerator）对 `ProgramData\Epic\UnrealBuildAccelerator` 持续 Access denied，会把 `.lib` 写成 0 字节并导致 `LNK1136: invalid or corrupt file`。遇到 LNK1136 先删 `Intermediate/Build/Win64/x64/UnrealEditor/Development/Chronos_New/UnrealEditor-Chronos_New.lib` 再加 `-NoUBA` 重来。
- 改完 C++ 后**不要依赖 Live Coding 收尾**：Live Coding 产物不等同于正式构建，最后务必关编辑器跑一次上面的命令行，再重启编辑器。
- MCP：编辑器启动后 `list_toolsets` 确认在线；调用必须串行。
  - 编辑器重启后 MCP 客户端常报 `MCP session not found`（客户端缓存了旧会话 ID）。WorkBuddy 侧的 `mcp__unreal-mcp__*` 工具要重试若干次才可能重连；**可靠兜底是直连 HTTP**：`POST http://127.0.0.1:8000/mcp` 携带 `Accept: application/json, text/event-stream` 先 `initialize` 拿 `Mcp-Session-Id` 响应头，之后每个请求带上该 header 调 `tools/call`（参数 `toolset_name` / `tool_name` / `arguments`）。服务器只暴露 `list_toolsets` / `describe_toolset` / `call_tool` 三个工具。
  - `ObjectTools.get_properties` 的属性名是**小驼峰**（`bHiddenInGame`、`skeletalMeshAsset`、`animClass`），写成 `HiddenInGame` 会静默读不到。拿不准就先 `list_properties`。
  - 蓝图组件的 refPath：本类组件用 `<BP路径>.<BP名>_C:TP_Weapon_GEN_VARIABLE`；继承自父类的组件用 `Default__<BP名>_C` 前缀枚举 `get_components` 拿准确名字。

## 自动化测试

`Source/Chronos_New/Tests/ChronosGameFlowTests.cpp`，编辑器 Automation 下 `Chronos.*`：

| 用例 | 覆盖 |
|---|---|
| Chronos.GameFlow.RegisterAndClear | 敌人注册/注销计数、清关广播 |
| Chronos.GameFlow.EnemyAimAndAutoEquip | 出生自动装备武器、无相机时瞄准回退 |
| Chronos.Weapon.AnimClassOnEquip | **装备/脱手/再装备**与**敌人武器→玩家拾取**两条路径的 ABP 切换，日志打 `[AnimDiag]` |
| Chronos.GameFlow.EnemyDeathStopsAILogic | 敌人带 `AAIController` 死亡后必须停掉 BrainComponent（StateTree），见 Bug #6 |
| Chronos.GameFlow.EnemyRejectsAllyAsTarget | 敌人不能把同类当交战目标（感知列表不区分敌我），见 Bug #5 |
| Chronos.GameFlow.EnemySpawnedWeaponIsLoaded | 出生武器必须有数据且弹药 > 0，否则举枪不开火 |

诊断这类"表现层不生效"的问题时，自动化用例比 PIE 手动测可靠得多：
MCP 的 Slate `PressKey` **无法把按键送进 PIE**（实测按 R 不触发重开关卡），
且 `ObjectTools` 只能读写属性、不能调用函数，PIE 里没有可靠的触发拾取手段。

## 流程界面与关卡推进

`Content/Blueprints/UI/` 下四个 Widget，全部挂在 `BP_ShooterPlayerController` 的 CDO 上
（BP 资产里的值会持久化；只改 C++ CDO 重启会丢）：

| Widget | 触发时机 | 内容 |
|---|---|---|
| `WBP_HUD` | Possess 后常驻 | 准星（居中）、武器名与弹药（右下） |
| `WBP_LevelTransition` | 清关且非末关 | 全屏黑底 + `SUPERHOT` |
| `WBP_Victory` | 清空末关 | `YOU WIN` + 用时/击杀（SUPERHOT 式结算） |
| `WBP_Defeat` | 玩家死亡 | `DEFEAT` + `按 R 重试` |
| `WBP_MainMenu` | 启动（菜单关卡） | `CHRONOS` + 开始游戏 / 退出 |
| `WBP_PauseMenu` | ESC | 继续 / 重开当前关 / 返回主菜单 |

**按钮点击一律放 C++**：UMG 的 `OnClicked` 是事件节点，编辑器脚本创建不了
（`AddEvent|XXX does not exist`）。三个需要交互的 Widget 都配了 C++ 父类，
用 `BindWidget` 绑控件 + `OnClicked.AddDynamic`：
- `UChronosMainMenuWidget`（开始 / 退出）
- `UChronosPauseWidget`（继续 / 重开 / 返回主菜单）
- `UChronosVictoryWidget` 只需显示数据，走 `SetStats()` 外部推送

⚠️ **主菜单场景靠"不在关卡列表里"识别**：`GameMode::IsConfiguredLevel()` 为 false 时
控制器显示主菜单并把输入切成 UI 模式。菜单关卡 `Lvl_MainMenu` 不在 `LevelList` 中，
所以不会跟正式关卡冲突。

⚠️ **暂停用 `SetGamePaused` 而不是时间缩放子系统**，两者是独立机制，互不干扰。
进入/退出主菜单前**必须先解除暂停**，否则会把暂停状态带进下一个场景。

推进链路：`GameMode.RegisterEnemy` → 敌人死亡注销 → 计数归零 → `OnLevelCleared(bIsFinalLevel)`
→ 末关显示 Victory / 否则显示过场 → `FinishLevelTransition()` → `GameMode.LoadNextLevel()`。
`LevelList` 在 `BP_ShooterGameMode` 上配置，当前三项：
`Lvl_FirstPerson` → `Lvl_Arena02` → `Lvl_Arena03`。

⚠️ **世界名恢复索引要兼容 PIE 前缀**：`BeginPlay` 里按世界名在 `LevelList` 中恢复
`CurrentLevelIndex`。PIE 下世界名带 `UEDPIE_0_` 前缀（如 `UEDPIE_0_Lvl_Arena02`），
精确比较会失配，索引停在 `INDEX_NONE`，于是 `LoadNextLevel` 用 `-1+1=0` **又打开第一关** ——
表现为"第二关打完不进第三关"。已改成包含匹配（`WorldPath.Contains(AssetName)`）。

⚠️ **过场有 C++ 兜底定时器**（`LevelTransitionFallbackSeconds`，默认 2 秒）。
过场 UI 理应在自己的动画结束后调用 `FinishLevelTransition`，但若它没调流程会**永久卡在过场上**，
所以无论蓝图是否调用都起一个定时器兜底；被调用时先 `ClearTimer`，不会重复推进。

## UI 视觉规范（SUPERHOT 风格）

三色极简，无贴图无装饰，全部靠色块 + 粗体大字实现：

| 用途 | 颜色 | 说明 |
|---|---|---|
| 背景 | `#050505` @ 0.85~1.0 alpha | 全屏 Image，CanvasPanelSlot 的 `zOrder = -10` 压在最底层 |
| 主文字 | 白 `#FFFFFF` | 标题 60~96px，正文 22~30px |
| 强调 | 红 `#E10500` | 仅用于"开始游戏"、DEFEAT 这类需要抓眼的地方 |
| 次要 | 灰 `#9E9E9E` | 标签、退出、返回主菜单 |

**HUD 必须带描边**：准星 34px / 弹药 40px / 武器名 18px，都用
`outlineSettings: {outlineSize: 2~3, outlineColor: 黑}`。
SUPERHOT 的场景有大片白色，纯白文字不描边会直接糊掉 —— 这是可读性的关键。

所有界面统一走上述规范：主菜单、暂停、结算、失败、过场。
改动入口就是各 WBP 里的 TextBlock 的 `font` 与 `colorAndOpacity`。

⚠️ MCP **没有导入资源的接口**（AssetTools 只有 create_folder），
所以外部生成的贴图进不来 —— UI 也因此刻意保持"不依赖任何贴图"。

## HUD（弹药/武器名/准星）

已完成并生效。`WBP_HUD`（`/Game/Blueprints/UI/WBP_HUD`）含一个 CanvasPanel 与三个 TextBlock
`AmmoText` / `WeaponText` / `Crosshair`（准星居中，武器名与弹药在右下角）。
C++ 侧（`Source/Chronos_New/UI/ChronosHUD.h/.cpp`）：
- `UChronosHUD : UUserWidget`，`OnAmmoChanged` / `OnWeaponChanged` 是 `BlueprintNativeEvent`，
  C++ 有默认实现（更新文本），蓝图可覆盖。
- `AChronosPlayerController` 在 `OnPossess` 时把 `CombatComponent` 的
  `OnWeaponChanged` / `OnAmmoChanged`（新加的对外转发委托）桥接到 HUD，并在换枪时主动推一次状态。
- 空枪显示"空 · 可投掷"而不是 0 —— 空枪是有效投掷物，别让玩家以为捡了把废枪。
- `AChronosPlayerController` 构造函数用 `ConstructorHelpers` 把 `HUDClass` 默认指向 `WBP_HUD`
  （项目没有 PC 的蓝图子类，只在编辑器改 CDO 的话重启就丢了，非 config 属性的 CDO 值不持久化）。
- ⚠️ **`BeginPlay` 里创建 HUD 后必须补推一次状态**：`OnPossess` 可能早于 `BeginPlay`，
  那时 HUD 还是 nullptr，推送被丢弃，HUD 就一直停在蓝图里的默认文本（"TextBlock"）上。
  WBP_HUD 里三个 TextBlock 的默认文本也都清成了空（准星是 "+")。

### 模板自带 UI 必须拆掉

模板的 `UI_Shooter`（含准星）和 `UI_ShooterBulletCounter` 会同时显示，导致**两个准星**。
已在两处删除创建逻辑（保留输入映射等其余逻辑）：

- `BP_ShooterGameMode` EventGraph：`CreateWidget(UI_Shooter)` + `AddToViewport`
- `BP_ShooterPlayerController` EventGraph：`CreateWidget(UI_ShooterBulletCounter)`

⚠️ 这两个资产仍在 Content 里但已无人引用。找"还有谁在显示 UI"时，
用 `UMGToolSet.ListWidgetBlueprints` 列出全部 Widget，再在各 BP 的 EventGraph 里
`find_nodes` / `read_graph_dsl` 搜 `CreateWidget`、`AddToViewport`。

⚠️ 教训：创建 Widget BP 时如果父类 C++ 类还没编译出 `BindWidget` 变量，资产会在保存时**静默丢失**
（目录都不生成）。务必**先编译 C++，再建 Widget BP**，且建完先加齐控件再保存。

### 顺带修掉的更大问题：GameMode / PlayerController 从未生效

排查 HUD 时发现：**实际跑的 GameMode 和 PlayerController 都是模板的原版类，没接上 Chronos 的 C++ 逻辑**
——敌人注册 / 清关推进 / 关卡列表 / R 键重开 全是断的。

ini 里其实一直有 `GlobalDefaultGameMode=/Game/Variant_Shooter/Blueprints/BP_ShooterGameMode...`
（模板自带，我一开始 grep 漏了），问题在于这两个模板 BP **没有重父类**：

| 资产 | 原父类 | 现父类 |
|---|---|---|
| `BP_ShooterGameMode` | 引擎 AGameModeBase（无 `LevelList`） | `AChronosGameMode` |
| `BP_ShooterPlayerController` | 引擎 APlayerController（无 `HUDClass`） | `AChronosPlayerController` |

重父类后模板自己的配置（`PlayerControllerClass`、`DefaultPawnClass=BP_ShooterCharacter`）都保留了下来。
另外 `AChronosGameMode` 构造函数补了 `PlayerControllerClass = AChronosPlayerController::StaticClass()`
（`AGameModeBase` 默认是引擎的 `APlayerController`，不设就不会用我们的 PC）。

👉 教训：查"实际生效的是哪个类"要顺着三层看 —— ini `GlobalDefaultGameMode`
→ GameMode CDO 的默认类 → **关卡 WorldSettings**（优先级最高）。
更可靠的是直接查 PIE 里的活对象：`find_actors` 找到后 `get_class`，
再用"它有没有 C++ 父类的特有属性"（如 `LevelList`、`HUDClass`）反推继承是否生效。
另：ini 里同一个键出现两次时**后者生效**，加配置前先 grep 一遍。

## 验证记录（最近一次 PIE 冒烟）
- PIE 启动无 `Accessed None`、无蓝图运行时错误（仅模板自带的 DLL 警告与 GameFeatureData 配置提示）。
- 敌人成功生成 `BP_ShooterWeapon_Pistol_C`（而非裸 `AChronosWeapon`）—— 敌人空手问题已消除。
- 玩家角色 CDO：`ThrowMontage=MM_Throw_Montage`、`DefaultFirstPersonAnimClass=ABP_FP_Copy`、`DefaultThirdPersonAnimClass=ABP_Unarmed`（敌人用 `ABP_TP_Rifle`）。
- 四把武器 TP_Weapon `bOwnerNoSee=true`；四个 DataAsset 的 EquipMontage 均已配置。

## 剩余任务
- 见 docs/superpowers/plans/2026-08-29-chronos-migration.md 勾选状态
- 已由 `Chronos.Weapon.AnimClassOnEquip` 覆盖：ABP 切换（含敌人武器路径）、二次装备。
- **以下仍需人工实机确认**：
  1. 子弹尾迹是否变直（`ReinitializeSystem` 修的是池化残留，若仍曲折需去 Niagara 里查 Ribbon 的采样率/生命周期）。
  2. 右键投掷、弹药耗尽自动丢弃是否仍会自伤（免疫双保险已加，手感与 0.35s 时长需实机调）。
  3. 拾取后第一人称是否只剩一把枪（F8 切第三人称应能看到 TP 枪）。
  4. 敌人现在会连发（新增 `StartShooting`/`StopShooting`），需确认节奏是否合理。
  5. 装备/投掷蒙太奇观感 —— 用 `animPlayRate` 压缩过（Rifle Equip 3.125 倍速），偏快，建议重新烘焙成正常速率的 Montage。
- 仍未接入：拾取时的手部动画、换弹表现、投掷命中角色的处决反馈。

---

# 扩展轮：设置·存档·音频·打击感·后处理·统计

> 目标：把"核心玩法能跑"提升成"像一款完整产品"。全部新增内容都不依赖任何外部美术/音频资产。

## 一、设置系统

| 类 | 文件 | 职责 |
|---|---|---|
| `UChronosGameUserSettings` | `Settings/ChronosGameUserSettings.h/.cpp` | 继承 `UGameUserSettings`，四组设置（图像/音频/控制/玩法）走 `config` 持久化；`ApplyAllSettings()` 统一下发并落盘 |
| `UChronosSettingsWidget` | `UI/ChronosSettingsWidget.h/.cpp` | **整棵控件树由 C++ 现场构建**的四个标签页设置界面 |
| `UChronosSettingProxy` | 同上 | 行事件代理：动态委托只能绑 UFUNCTION，用它把 (行索引, 值) 转发回界面 |

生效链路：滑块/开关 → `SetXxxValue()`（只改内存）→ 点「应用」→ `ApplyAllSettings()` →
`Scalability` / cvar（`r.ScreenPercentage`、`t.MaxFPS`、`GEngine->DisplayGamma`）→ `SaveSettings()`。

⚠️ **必须在 `DefaultEngine.ini` 的 `[/Script/Engine.Engine]` 里写**
`GameUserSettingsClassName=/Script/Chronos_New.ChronosGameUserSettings`，
否则引擎实例化的是基类，CHRONOS 的字段全部不存在。

⚠️ **UHT 阴影检查**：不要在派生类里声明与基类同名的 UPROPERTY（踩过 `FrameRateLimit`，
已改名 `FrameRateCap`）。同理局部变量不要叫 `Slot`（`UWidget` 有同名成员，`-Wshadow` 会报错）。

玩法设置的下发依赖"活世界"（要拿到时间子系统），所以 `ApplyGameplaySettings()`
用 `GEngine->GetCurrentPlayWorld()` 兜底查找，在没有任何世界时静默跳过。

## 二、存档

| 类 | 职责 |
|---|---|
| `UChronosSaveGame` | 玩家档案：每关最佳成绩 `TMap<FString, FChronosLevelRecord>` + 累计统计 + 解锁进度 |
| `UChronosSaveSubsystem` | **GameInstance 级**（跨 OpenLevel 存活）：加载/落盘、`SubmitRunResult()`、`UnlockLevel()` |

为什么挂 GameInstance：通关是 OpenLevel 硬切，World 子系统会随世界销毁。
设置不进存档 —— 设置需要在没有存档时也能改（还没开始游戏就调音量），走 `GameUserSettings.ini`。

评级 `FChronosRunResult::GetScore()` = 准度 50 + 效率 30 + 速度 20 + 连杀奖励 − 死亡惩罚；
速度按"每个敌人 12 秒"给满分，`GetRating()` 按 85/70/55/40 分档给出 S/A/B/C/D。

## 三、程序化音频（零资产）

| 类 | 职责 |
|---|---|
| `UChronosSynthComponent` | 继承 `USynthComponent`（**在 AudioMixer 模块里**，Build.cs 已加依赖），`OnGenerateAudio` 里做加法合成：15 种预设（枪声/命中/击杀/拾取/投掷/时间冻结/UI…），扫频振荡器 + 噪声 + 低频体，指数包络，输出端一阶低通（低通量由时间流速驱动）+ 软削波 |
| `UChronosAudioSubsystem` | 宿主管理（自建 Actor 挂合成器）、音量总线、距离衰减、`PlaySfx` / `PlaySfxAtLocation` |

关键设计：**时间冻结时世界闷响** —— `HandleTimeDilationChanged` 把 `1 - Dilation` 映射成低通系数，
新音效还会整体降调（`FMath::Lerp(0.55, 1.0, Dilation)`）。这是文件音频做不到的。

`USynthComponent` 头文件在 `Runtime/AudioMixer/Public/Components/SynthComponent.h`；
`NumChannels` 是基类成员（构造里置 1），`Init(int32& SampleRate)` 拿采样率。

## 四、打击感

| 类 | 职责 |
|---|---|
| `UChronosFeedbackSubsystem` | 唯一出口：`NotifyWeaponFired` / `NotifyHitConfirmed` / `NotifyKill` / `NotifyExplosion` / `NotifyPlayerDamaged` / UI 音效 |
| `UChronosCameraModifier` | Trauma 制镜头效果（位移取 Trauma²），挂在本地 `PlayerCameraManager` 上，支持震动倍率与 FOV 冲击 |

- **不用引擎 CameraShake 资产体系**：5.8 里 Shake 走 `UCameraShakeBase + ShakePattern` 资产，
  而这里要的是"代码按帧喂强度"的动态震动，直接改 `FMinimalViewInfo` 更简单也更可控。
- `AddNewCameraModifier` **不会去重**，每次调用都新建实例；必须先用
  `ForEachCameraModifier`（`ModifierList` 是 protected）查一遍已有的，否则镜头会越震越夸张。
- 顿帧只在满速时给（慢门里再顿帧像卡住）；击杀走 `TimeDilationSubsystem::TriggerKillOverride()` 的子弹时间。

## 五、程序化后处理

`UChronosVisualSubsystem`（`Visuals/`）：**运行时生成一个 `bUnbound` 的 PostProcessVolume**，
每帧按状态改写 `FPostProcessSettings`（`ColorSaturation` / `ColorContrast` / `ColorGain` /
`VignetteIntensity` / `VignetteColor` / `SceneFringeIntensity`），无需任何材质或贴图资产。

- 时间越慢 → 去饱和 + 暗角 + 色散越强（SUPERHOT 的"世界停住了"）；
- 击杀 → `ColorGain` 短促提亮（连杀越高越猛）；受伤 → 暗角转红。
- 强度统一乘 `Settings->PostProcessIntensity`，拉到 0 就回到引擎默认画面（此时整个 volume 关掉）。

## 六、统计与结算

`UChronosStatsSubsystem`（`Stats/`）：**全部按真实时间统计**（`FPlatformTime::Seconds`）——
这游戏 90% 时间处于 0.05 倍速，用游戏时间统计出来的"用时"毫无意义。
连杀窗口也按真实秒计（时间冻结时游戏内几乎不流逝，但玩家操作是连续的）。

清关链路：`GameMode.OnLevelCleared` → `PC.SubmitCurrentLevelResult()`（每关都结算一次）
→ `SaveSubsystem.SubmitRunResult()` + `UnlockLevel(Index+1)` → 末关时 `VictoryWidget.SetRunResult()`。

## 七、UI 补充（重要教训）

**`ObjectTools.set_properties` 在本项目恒返回 false**（连资产对象都写不动），
所以新增控件一律改为 **C++ 在运行时自建并配置**：

| 界面 | C++ 自建的控件 |
|---|---|
| HUD | `HitMarker` / `ComboText` / `DamageIndicator` / `FocusText` |
| 主菜单、暂停菜单 | `SettingsButton`（+ 标签），排在最后一个按钮下方 |
| 结算 | `AccuracyValueText` / `RatingText` / `ComboValueText` / `BestTimeText` / `NewRecordText` |

实现模式是 `EnsureXxx()`：蓝图里有同名控件就复用（只重写样式），没有就
`WidgetTree->ConstructWidget` 现场建并设好 `UCanvasPanelSlot` 的锚点/对齐/偏移。
好处是外观与行为只有一份定义，不受资产编辑能力限制。
C++ 自建时文本里自带标签（"命中率 65%"），蓝图版通常另外配了标签，只放数值。

⚠️ 新增的 BindWidget 一律用 `BindWidgetOptional`，否则老资产缺控件会导致整个界面编译失败。
⚠️ 动态委托绑定要求签名逐字一致：`FText` 按值 vs `const FText&` 会导致 C2665，
   角色 → HUD 的交互提示因此走 PlayerController 的 `HandleInteractionFocusChanged` 转发一层。

## 八、自动化构建（AI 侧）

编辑器重启后 MCP 客户端会报 `Unknown session id`，官方客户端工具连不上，
可靠兜底是直连 HTTP：`POST http://127.0.0.1:8000/mcp` → `initialize` 取 `Mcp-Session-Id` 响应头 →
后续请求带上该 header 调 `tools/call`。
项目里已封装两个脚本：`.workbuddy/mcp_call.py`（单次）与 `.workbuddy/mcp_batch.py`（批量，支持从上次结果取值）。

构建流程：保存资产 → 关编辑器 → `Build.bat ... -NoUBA` → 读 `Saved/_build.log` 排错 → 重启编辑器。
