# Chronos_New 架构与交接文档

> 本文档供后续 AI 会话接手项目，随每次改动同步更新。设计规格：docs/superpowers/specs/2026-08-29-chronos-migration-design.md

## 项目定位
SUPERHOT 风格第一人称 Arena Shooter 求职项目。C++ 系统 + 蓝图表现混合。

## C++ 单元
（随任务填充：类名 / 文件 / 职责一句话）

| 类 | 文件 | 职责 |
|---|---|---|
| AChronosGameMode | Source/Chronos_New/GameFlow/ChronosGameMode.h/.cpp | 敌人注册/注销计数（OnEnemyCountChanged）、清关广播（OnLevelCleared）、多关卡推进（LevelList/RestartLevel/LoadNextLevel）、击杀触发子弹时间 |
| AChronosEnemy | Source/Chronos_New/Characters/ChronosEnemy.h/.cpp | 敌人基类：出生按 EnemyWeaponData 自动生成并装备武器、SetCombatTarget/GetCombatTarget、无相机时瞄准回退到朝目标方向、BeginPlay 自注册到 AChronosGameMode |
| AChronosPlayerController | Source/Chronos_New/GameFlow/ChronosPlayerController.h/.cpp | 玩家控制器：常驻 HUD 与流程界面（HUD/LevelTransition/Victory/Defeat 四个 WidgetClass）、监听 OnLevelCleared/OnCharacterDeath、FinishLevelTransition 推进下一关、R 键重开当前关 |
| AChronosProjectile | Source/Chronos_New/Projectiles/Projectile.h/.cpp | 池化子弹：直线无重力飞行、一击必杀（UHealthComponent）、命中物理体施加冲量；Task 5 起订阅 UTimeDilationSubsystem::OnDilationChanged（ActivateProjectile 绑定 + 立即同步当前值，EndPlay 解绑），HandleTimeDilationChanged 把当前时间缩放写入 Niagara 用户参数 User.TimeDilation，实现慢门时尾迹增亮（1/max(dilation,0.05)） |

自动化测试：Source/Chronos_New/Tests/ChronosGameFlowTests.cpp（编辑器 Automation "Chronos.GameFlow.*" 2 用例：注册清关计数 / 出生自动装备+瞄准；监听器 UChronosTestListener 在 Tests/ChronosTestListener.h）

## 蓝图资产
| 资产 | 父类 | 用途 |
|---|---|---|
| /Game/Variant_Shooter/Blueprints/Pickups/BP_ShooterWeaponBase | AChronosWeapon | 武器基类 BP：旧模板开火逻辑已删（10 变量 + Fire Bullet 函数），仅保留 Firing Montage、FP/TP Anim Instance、噪声参数、Pawn Owner、Calculate Bullet Spawn Transform，及 5 个空存根事件（BeginPlay/StartFiring/StopFiring/ActivateWeapon/DeactivateWeapon，供跨 BP 编译兼容） |
| /Game/Variant_Shooter/Blueprints/Pickups/Weapons/BP_ShooterWeapon_Pistol | BP_ShooterWeaponBase | 手枪：CDO WeaponData=DA_Pistol；OnFireVisuals 播 MM_Pistol_Fire_Montage（FP 手臂），OnEquipVisuals 空实现 |
| /Game/Variant_Shooter/Blueprints/Pickups/Weapons/BP_ShooterWeapon_Rifle | BP_ShooterWeaponBase | 步枪：CDO WeaponData=DA_Rifle；OnFireVisuals 播 FP_Rifle_Shoot_Montage |
| /Game/Variant_Shooter/Blueprints/Pickups/Weapons/BP_ShooterWeapon_GrenadeLauncher | BP_ShooterWeaponBase | 榴弹发射器：CDO WeaponData=DA_GrenadeLauncher（ProjectileClass 为空，弹道走 BP）；OnFireVisuals 在枪口 socket 位姿处 SpawnActor BP_ShooterProjectile_Grenade + PlaySound2D |
| /Game/Blueprints/Weapons/BP_Pistol | （已删除） | Task 3 临时武器 BP，Task 4 已删除（无引用、无关卡实例） |
| /Game/Blueprints/Projectiles/BP_ChronosProjectile | AChronosProjectile | 子弹 BP：CDO TrailSystem=NS_ChronosBulletTrail；DA_Pistol/DA_Rifle/DA_Enemy_Pistol 的 ProjectileClass 均指向它 |

## 特效资产
| 资产 | 用途 |
|---|---|
| /Game/Variant_Shooter/FX/NS_ChronosBulletTrail | 子弹尾迹：红色（约 RGB(1,0.08,0.08)），用户参数 User.TimeDilation（float，默认 1.0）经 1/Max(TimeDilation,0.05) 缩放颜色——正常速度暗红、时间冻结（dilation→0.05）时增亮至 20 倍，强化"凝固子弹"读感；由 AChronosProjectile 在 OnDilationChanged 时写入 |

## 数据资产
（UWeaponDataAsset 实例，/Game/DataAssets/ 下；字段见 Source/Chronos_New/Weapons/WeaponDataAsset.h）

| 资产 | WeaponName | 网格 | 弹药/射速/全自动/弹速 | ProjectileClass | FireMontage |
|---|---|---|---|---|---|
| DA_Pistol | 手枪 | SKM_Pistol | 8 / 5 / 否 / 6000 | BP_ChronosProjectile | MM_Pistol_Fire_Montage |
| DA_Rifle | 步枪 | SKM_Rifle | 20 / 8 / 是 / 8000 | BP_ChronosProjectile | FP_Rifle_Shoot_Montage |
| DA_GrenadeLauncher | 榴弹发射器 | SKM_GrenadeLauncher | 3 / 1 / 否 / 4000 | 空（BP 自管生成榴弹） | FP_Rifle_Shoot_Montage |
| DA_Enemy_Pistol | 敌用手枪 | SKM_Pistol | 3 / 1 / 否 / 6000 | BP_ChronosProjectile | 无（敌人无声） |

MuzzleSocketName=Muzzle（实测三把网格均有该 socket）、HandSocketName=hand_rWeaponSocket、ThrowSpeed=1500 四资产一致。关卡直放的武器实例由 AChronosWeapon::BeginPlay 检测到 WeaponData+无网格+零弹药后自动 InitFromData。

## 关卡
- /Game/FirstPerson/Lvl_FirstPerson：PIE 冒烟关卡，PlayerStart 前方 (120,0,100) 放有一把 BP_ShooterWeapon_Pistol 实例（Task 4 冒烟验证用，Task 9 L01 完成后由正式关卡取代）

## 构建与运行
- 引擎：UE 5.8（EngineAssociation "5.8"，安装于 D:/Unreal/UE_5.8）
- 构建：Build.bat Chronos_NewEditor Win64 Development -project=... -WaitMutex
- MCP：编辑器启动后 list_toolsets 确认在线；调用必须串行

## 剩余任务
- 见 docs/superpowers/plans/2026-08-29-chronos-migration.md 勾选状态
