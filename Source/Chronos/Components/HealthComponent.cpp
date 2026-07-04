// BEGIN CODE SNIPPET
// File: Source/Chronos/Components/HealthComponent.cpp
// Purpose: Implementation of the health component

#include "HealthComponent.h"

#include "Chronos/Tags/ChronosTags.h"

UHealthComponent::UHealthComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UHealthComponent::BeginPlay()
{
    Super::BeginPlay();

    OwnedTags.RemoveTag(FChronosTags::State_Dead);
    OwnedTags.AddTag(FChronosTags::State_Alive);
}

// ==================== IDamageable Interface Implementation ====================

void UHealthComponent::TakeDamage_Implementation(float DamageAmount, AActor* DamageCauser)
{
    if (IsDead())
    {
        return;
    }

    OnDamageTaken.Broadcast(DamageAmount, DamageCauser);

    Die(DamageCauser);
}

bool UHealthComponent::IsAlive_Implementation() const
{
    return !IsDead();
}

void UHealthComponent::Die_Implementation(AActor* Killer)
{
    if (IsDead())
    {
        return;
    }

    OwnedTags.RemoveTag(FChronosTags::State_Alive);
    OwnedTags.AddTag(FChronosTags::State_Dead);

    OnDeath.Broadcast(Killer);
}

// ==================== Public Methods ====================

void UHealthComponent::Revive()
{
    OwnedTags.RemoveTag(FChronosTags::State_Dead);
    OwnedTags.AddTag(FChronosTags::State_Alive);
}

bool UHealthComponent::IsDead() const
{
    return OwnedTags.HasTag(FChronosTags::State_Dead);
}

bool UHealthComponent::HasGameplayTag(const FGameplayTag& Tag) const
{
    return OwnedTags.HasTag(Tag);
}

void UHealthComponent::AddGameplayTag(const FGameplayTag& Tag)
{
    OwnedTags.AddTag(Tag);
}

void UHealthComponent::RemoveGameplayTag(const FGameplayTag& Tag)
{
    OwnedTags.RemoveTag(Tag);
}
// END CODE SNIPPET