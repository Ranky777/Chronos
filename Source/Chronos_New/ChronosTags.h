#pragma once

#include "NativeGameplayTags.h"

/**
 * 项目原生 GameplayTag 集中定义。
 * 使用 C++ 原生标签而非 DefaultGameplayTags.ini 配置，
 * 保证编译期即可校验、杜绝配置与代码不同步的问题；蓝图同样可以直接使用这些标签。
 */
namespace ChronosTags
{
	/** 实体存活状态 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Alive);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Dead);

	/** 行为状态 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Shooting);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_MeleeAttacking);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Throwing);

	/** 关键事件（供蓝图/StateTree 监听） */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_PlayerDied);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_EnemyDied);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_LevelComplete);
}
