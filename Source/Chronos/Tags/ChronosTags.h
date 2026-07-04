#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

namespace FChronosTags
{
	CHRONOS_API extern const FGameplayTag State_Alive;
	CHRONOS_API extern const FGameplayTag State_Dead;
	CHRONOS_API extern const FGameplayTag State_Shooting;
	CHRONOS_API extern const FGameplayTag State_Reloading;
	CHRONOS_API extern const FGameplayTag State_Moving;
	
	CHRONOS_API extern const FGameplayTag Event_PlayerDied;
	CHRONOS_API extern const FGameplayTag Event_EnemyDied;
	CHRONOS_API extern const FGameplayTag Event_LevelComplete;
}