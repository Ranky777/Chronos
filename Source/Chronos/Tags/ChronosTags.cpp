#include "ChronosTags.h"

namespace FChronosTags
{
	const FGameplayTag State_Alive = FGameplayTag::RequestGameplayTag(FName("State.Alive"));
	const FGameplayTag State_Dead = FGameplayTag::RequestGameplayTag(FName("State.Dead"));
	const FGameplayTag State_Shooting = FGameplayTag::RequestGameplayTag(FName("State.Shooting"));
	const FGameplayTag State_Moving = FGameplayTag::RequestGameplayTag(FName("State.Moving"));

	const FGameplayTag Event_PlayerDied = FGameplayTag::RequestGameplayTag(FName("Event.PlayerDied"));
	const FGameplayTag Event_EnemyDied = FGameplayTag::RequestGameplayTag(FName("Event.EnemyDied"));
	const FGameplayTag Event_LevelComplete = FGameplayTag::RequestGameplayTag(FName("Event.LevelComplete"));
}