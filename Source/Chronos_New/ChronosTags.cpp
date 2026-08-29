#include "ChronosTags.h"

namespace ChronosTags
{
	UE_DEFINE_GAMEPLAY_TAG(State_Alive, "State.Alive");
	UE_DEFINE_GAMEPLAY_TAG(State_Dead, "State.Dead");

	UE_DEFINE_GAMEPLAY_TAG(State_Shooting, "State.Shooting");
	UE_DEFINE_GAMEPLAY_TAG(State_MeleeAttacking, "State.MeleeAttacking");
	UE_DEFINE_GAMEPLAY_TAG(State_Throwing, "State.Throwing");

	UE_DEFINE_GAMEPLAY_TAG(Event_PlayerDied, "Event.PlayerDied");
	UE_DEFINE_GAMEPLAY_TAG(Event_EnemyDied, "Event.EnemyDied");
	UE_DEFINE_GAMEPLAY_TAG(Event_LevelComplete, "Event.LevelComplete");
}
