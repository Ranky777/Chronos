# Chronos 迁移设计规格（Chronos → Chronos_New）

- 日期：2026-08-29
- 状态：已与用户逐节确认
- 目标：在基于 UE5 第一人称 Arena Shooter 模板（Variant_Shooter）的 Chronos_New 中，重现旧项目 Chronos 的 SUPERHOT 风格玩法，作为**求职项目**展示 C++/蓝图混合开发能力。

## 0. 背景与原则

旧项目 Chronos（`C:\Users\cuilongqi\Desktop\UEProjects\Chronos`）因美术/动画处理困难被放弃。其设计文档（`.trae/specs/project-chronos/spec.md`、`superhot-gap-analysis/spec.md`、`weapon_pickup_system_plan.md`）是本迁移的设计来源。

核心原则（用户明确要求）：
1. **模板 BP 就是运行时资产**——通过"重父类到 C++ 类 + 删遗留逻辑 + 补 SUPERHOT 语义"复用，不新造重复资产。
2. C++ 承担系统/规则层，蓝图承担资产配置/表现/UMG/StateTree——混合分工本身是展示点。
3. 操作手感贴近原版 SUPERHOT。

## 1. 现状盘点

### 1.1 C++（Source/Chronos_New，已存在，不改动除非设计要求）
| 单元 | 说明 |
|---|---|
| AChronosCharacter (Abstract) | 玩家/敌人共用基类：Enhanced Input、时间上报 NotifyPlayerInput、瞄准射线、投掷/交互流程、武器附着手臂 socket |
| AChronosWeapon | 唯一武器 Actor：Equipped/InWorld 双状态、IWeaponUser+IInteractable、弹药记账、射击冷却、池化投射物、投掷击杀（速度阈值 600）、0.5s 拾取延迟 |
| IWeaponUser / IInteractable | C++ 接口，解耦具体实现 |
| UCombatComponent | 单武器槽，持 TScriptInterface<IWeaponUser> |
| UHealthComponent | 一击必杀，GameplayTag State.Alive/Dead，OnDeath/OnRevived |
| UWeaponDataAsset | 武器数据：网格/muzzle socket/弹量/射速/全自动/弹速/ProjectileClass/FireMontage |
| AChronosProjectile + UChronosProjectilePoolSubsystem | 池化直线子弹（专用碰撞通道 GameTraceChannel1），Niagara 尾迹 |
| UTimeDilationSubsystem | 真实时间戳输入窗口（无旧项目 0.5s 延迟 Bug）、击杀慢镜 Override、玩家死亡强制满速、OnDilationChanged |
| ChronosTags | 原生 GameplayTag |

### 1.2 模板蓝图继承链（已用 MCP 实查）
| 蓝图 | 父类链 | 迁移动作 |
|---|---|---|
| BP_ShooterCharacter（玩家，默认 Pawn） | → BP_FirstPersonCharacter → AChronosCharacter | 无需动（已接通） |
| BP_ShooterNPC（敌人） | → BP_FirstPersonCharacter → AChronosCharacter | 重父类 → AChronosEnemy（新增 C++） |
| BP_ShooterWeaponBase（+Pistol/Rifle/GrenadeLauncher 三子类） | → AActor | 重父类 → AChronosWeapon，删 BP 内旧开火逻辑 |
| BP_ShooterPlayerController | → BP_FirstPersonPlayerController | 重父类 → AChronosPlayerController（新增 C++） |
| BP_ShooterGameMode（默认 GameMode） | → GameModeBase | 重父类 → AChronosGameMode（新增 C++） |
| BP_Pistol（用户自建） | → AChronosWeapon | **删除**（被 BP_ShooterWeapon_Pistol 取代）；DA_Pistol 保留为数据配置样板 |
| BP_ShooterProjectile_Grenade + 爆炸 | 模板投射物 | 保留，供榴弹发射器使用 |
| ST_Shooter / EQS / StateTree 任务 | 模板 AI | 保留，仅 Shoot 任务改走武器接口 |
| UI_Shooter / UI_ShooterBulletCounter | 模板 UMG | 参考风格；新 WBP 为主 |
| DT_WeaponList / BP_ShooterPickup / BPI_WeaponHolder | 模板拾取体系 | **不进运行时**（AChronosWeapon InWorld 状态即拾取物） |

## 2. 新增 C++ 单元

| 类 | 职责 |
|---|---|
| AChronosEnemy : AChronosCharacter | 敌人语义：GetAimDirection 无相机回退（朝目标）；BeginPlay 按配置 EnemyWeaponData 生成+装备武器；死亡→武器 Drop 到 InWorld + 广播 OnEnemyDied |
| AChronosGameMode : GameModeBase | TArray<TSoftObjectPtr<UWorld>> 关卡列表（BP 子类配置）；敌人注册/注销计数（事件驱动，基于 HealthComponent 死亡委托+Tag）；清空→过场→OpenLevel 下一关；末关→胜利；收到重启请求→重开当前关；敌死触发 TriggerKillOverride |
| AChronosPlayerController : PlayerController | 创建/持有 HUD Widget 类（BP 配置）；失败界面；R 重启输入；关卡切换时的过场 UI 触发 |

**AChronosWeapon 修改**：新增 `OnFireVisuals`/`OnEquipVisuals` BlueprintImplementableEvent（武器播放 Montage/音效/枪口 FX 的 BP 通道）；**移除近战**（AChronosCharacter::PerformMeleeAttack 与其输入绑定删除，见 §6 范围裁剪）。

## 3. 武器系统（重父类后的 C++/BP 分工）

- C++：状态机、弹药、冷却、池化投射物、投掷击杀、拾取（全部已有）。
- BP（BP_ShooterWeaponBase 及三子类）：删除旧开火逻辑（Bullet Class/Current Bullets/Fire Bullet/Refire Timer/Full Auto/Time of Last Shot 等变量与 Fire Bullet 函数）；在 OnFireVisuals/OnEquipVisuals 中播放模板 Montage（如 FP_Rifle_Shoot_Montage）、音效、枪口 FX。
- 数据：DA_Pistol（已有，改引模板 SKM_Pistol）、新增 DA_Rifle、DA_GrenadeLauncher（SKM_Rifle/SKM_GrenadeLauncher + 对应 Montage/音效）。
- 投射物：手枪/步枪 → AChronosProjectile（红色 Niagara 弹道，慢门时增亮：Projectile 监听 IsTimeSlowed 切换 Niagara 用户参数）；榴弹发射器 → 模板 BP_ShooterProjectile_Grenade + BP_Shooter_GrenadeExplosion（全局时间缩放自动同步其运动与爆炸）。
- 敌人武器：DA_Enemy_* 实例（弹量低于玩家版，制造"敌人会打空"的窗口）。

## 4. 敌人 AI

- BP_ShooterNPC 重父类 AChronosEnemy；材质换红色 MI 实例（SUPERHOT 敌人识别）。
- StateTree（ST_Shooter）骨架保留：感知→FaceActor→ShootAtTarget→EQS 走位；Shoot 任务开火调用改走 CombatComponent 武器接口（FireAtTarget）。
- 弹药打空的敌人：**扔枪砸玩家**（AChronosWeapon.ThrowWeapon 现成，零新资产），之后赤手进入 EQS 游走状态。
- 全局时间缩放（WorldSettings）使敌人/子弹/物理只在时间流动时推进——SUPERHOT 核心体验，无需额外代码。
- 敌人子弹为真实可躲避投射物（与玩家子弹同池/同通道）。

## 5. 游戏流程与关卡

- 多小关卡结构（模仿原版）：3 个 umap。L01 教学单房间（地上手枪+2 敌人）；L02 步枪+3-4 敌人；L03 榴弹发射器+5-6 敌人压轴。用模板竞技场资产 + LevelPrototyping（BP_DoorFrame 等）搭建。Lvl_FirstPerson 改造为 L01（保持启动图）。
- 敌人 BeginPlay 向 GameMode 注册，死亡注销并广播；计数归零→WBP_LevelTransition（SUPERHOT 闪烁大字）→OpenLevel 下一关→末关 WBP_Victory。
- 玩家死亡→SetPlayerDead(true)（时间满速）→WBP_Defeat→R 重开当前关。
- 击杀瞬间 TriggerKillOverride 慢镜（已有）。

## 6. 范围裁剪（明确不做，防蔓延）

| 项 | 决定 | 原因 |
|---|---|---|
| 近战（玩家/敌人） | 不做 | 模板无近战 ABP/动画资产（用户决定，覆盖旧 gap-analysis 计划） |
| Hotswitch 换身 | 后期可选 | 工作量大，先保完整核心体验 |
| 环境投掷物 | 后期可选 | 武器投掷已覆盖核心体验 |
| 换弹 | 永不做 | SUPERHOT 本就没有 |

## 7. UI（UMG）

- WBP_HUD：准星+交互提示（OnInteractionFocusChanged）、弹药计数（OnAmmoChanged）、敌人计数、时间流速指示（OnDilationChanged）。
- WBP_LevelTransition / WBP_Victory / WBP_Defeat。
- 挂载：AChronosPlayerController 创建，Widget 类在其 BP 子类中配置。

## 8. 测试与验证

- C++ Automation 测试（求职加分）：时间缩放插值、弹药耗尽、投掷击杀阈值、拾取延迟等纯逻辑用例。
- 每个可交付单元用 MCP 在 PIE 实测全链路：开枪→杀敌→慢镜→掉枪→拾取→投掷。
- 交接文档同步纪律：CLAUDE.md（根目录）+ docs/Architecture.md 随每完成一个单元即更新，不攒批。

## 9. 风险与对策

| 风险 | 对策 |
|---|---|
| BP_ShooterWeaponBase 重父类后与 AChronosWeapon 组件/变量冲突 | 先实查 BP 组件模板；网格统一走 WeaponMeshComponent+DA；删 BP 遗留变量；重父类后立即编译+PIE 验证 |
| AChronosCharacter Abstract 阻断某些重父类路径 | 敌人经 AChronosEnemy（具体类）中转；已验证 BP_ShooterNPC 现链路可行 |
| 模板 StateTree 任务与全局时间缩放交互未知 | 实施早期先做"时间缩放下 StateTree 是否冻结"的最小验证 |
| UMG 数字动画依赖模板实现 | 新 WBP 参考其风格自建，不直接依赖模板控件内部 |
