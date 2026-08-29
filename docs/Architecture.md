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
- 引擎：UE 5.8（EngineAssociation "5.8"，安装于 D:/Unreal/UE_5.8）
- 构建：Build.bat Chronos_NewEditor Win64 Development -project=... -WaitMutex
- MCP：编辑器启动后 list_toolsets 确认在线；调用必须串行

## 剩余任务
- 见 docs/superpowers/plans/2026-08-29-chronos-migration.md 勾选状态
