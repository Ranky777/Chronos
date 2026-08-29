#include "Components/HealthComponent.h"

#include "ChronosTags.h"

UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	StateTags.AddTag(ChronosTags::State_Alive);
}

void UHealthComponent::TakeDamage(AActor* InstigatorActor)
{
	if (!IsAlive())
	{
		return;
	}

	AActor* Owner = GetOwner();
	OnDamageTaken.Broadcast(Owner, InstigatorActor);

	// 一击必杀：直接切换到死亡状态，不存在中间血量
	StateTags.RemoveTag(ChronosTags::State_Alive);
	StateTags.AddTag(ChronosTags::State_Dead);

	OnDeath.Broadcast(Owner, InstigatorActor);
}

bool UHealthComponent::IsAlive() const
{
	return StateTags.HasTagExact(ChronosTags::State_Alive);
}

void UHealthComponent::Revive()
{
	if (IsAlive())
	{
		return;
	}

	StateTags.RemoveTag(ChronosTags::State_Dead);
	StateTags.AddTag(ChronosTags::State_Alive);
	OnRevived.Broadcast(GetOwner());
}
