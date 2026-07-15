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
    FString OwnerName = GetOwner() ? FString(GetOwner()->GetName()) : TEXT("NoOwner");
    FString CauserName = DamageCauser ? FString(DamageCauser->GetName()) : TEXT("Unknown");
    
    GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, 
        FString::Printf(TEXT("[HealthComponent] TakeDamage - Owner=%s, Damage=%f, Causer=%s"), 
        *OwnerName, DamageAmount, *CauserName));

    if (IsDead())
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, 
            FString::Printf(TEXT("[HealthComponent] TakeDamage: %s is already dead"), 
            *OwnerName));
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
    FString OwnerName = GetOwner() ? FString(GetOwner()->GetName()) : TEXT("NoOwner");
    FString KillerName = Killer ? FString(Killer->GetName()) : TEXT("Unknown");
    
    GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, 
        FString::Printf(TEXT("[HealthComponent] Die - Owner=%s, Killer=%s"), 
        *OwnerName, *KillerName));

    if (IsDead())
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, 
            FString::Printf(TEXT("[HealthComponent] Die: %s is already dead"), 
            *OwnerName));
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