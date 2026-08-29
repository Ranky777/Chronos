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

自动化测试：Source/Chronos_New/Tests/ChronosGameFlowTests.cpp（编辑器 Automation "Chronos.GameFlow.*" 2 用例：注册清关计数 / 出生自动装备+瞄准；监听器 UChronosTestListener 在 Tests/ChronosTestListener.h）

## 蓝图资产
（随任务填充：资产路径 / 父类 / 用途）

## 关卡
（随任务填充）

## 构建与运行
- 引擎：UE 5.8（EngineAssociation "5.8"，安装于 D:/Unreal/UE_5.8）
- 构建：Build.bat Chronos_NewEditor Win64 Development -project=... -WaitMutex
- MCP：编辑器启动后 list_toolsets 确认在线；调用必须串行

## 剩余任务
- 见 docs/superpowers/plans/2026-08-29-chronos-migration.md 勾选状态
