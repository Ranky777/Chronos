#include "Subsystems/ProjectilePoolSubsystem.h"

#include "Engine/World.h"
#include "Projectiles/Projectile.h"

void UChronosProjectilePoolSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
}

AChronosProjectile* UChronosProjectilePoolSubsystem::GetOrCreateProjectile(TSubclassOf<AChronosProjectile> ProjectileClass)
{
	UWorld* World = GetWorld();
	if (!World || !*ProjectileClass)
	{
		return nullptr;
	}

	FChronosProjectilePool& Pool = Pools.FindOrAdd(ProjectileClass);

	while (Pool.InactiveProjectiles.Num() > 0)
	{
		AChronosProjectile* Candidate = Pool.InactiveProjectiles.Pop();
		if (IsValid(Candidate))
		{
			return Candidate;
		}
	}

	AChronosProjectile* NewProjectile = World->SpawnActorDeferred<AChronosProjectile>(
		ProjectileClass, FTransform::Identity, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	if (NewProjectile)
	{
		NewProjectile->FinishSpawning(FTransform::Identity);
	}

	return NewProjectile;
}

AChronosProjectile* UChronosProjectilePoolSubsystem::FireProjectile(TSubclassOf<AChronosProjectile> ProjectileClass,
	AActor* Instigator, const FVector& Location, const FRotator& Rotation)
{
	AChronosProjectile* Projectile = GetOrCreateProjectile(ProjectileClass);
	if (!Projectile)
	{
		return nullptr;
	}

	Projectile->ActivateProjectile(Instigator, Location, Rotation);

	FChronosActiveProjectile ActiveEntry;
	ActiveEntry.Projectile = Projectile;
	ActiveEntry.ActivatedRealTime = FPlatformTime::Seconds();
	ActiveProjectiles.Add(ActiveEntry);

	return Projectile;
}

void UChronosProjectilePoolSubsystem::ReturnProjectile(AChronosProjectile* Projectile)
{
	if (!IsValid(Projectile))
	{
		return;
	}

	for (int32 Index = ActiveProjectiles.Num() - 1; Index >= 0; --Index)
	{
		if (ActiveProjectiles[Index].Projectile == Projectile)
		{
			ActiveProjectiles.RemoveAtSwap(Index);
			break;
		}
	}

	Projectile->DeactivateProjectile();

	if (TSubclassOf<AChronosProjectile> Class = Projectile->GetClass())
	{
		Pools.FindOrAdd(Class).InactiveProjectiles.Push(Projectile);
	}
}

void UChronosProjectilePoolSubsystem::PreallocateProjectiles(TSubclassOf<AChronosProjectile> ProjectileClass, int32 Count)
{
	for (int32 Index = 0; Index < Count; ++Index)
	{
		if (AChronosProjectile* Projectile = GetOrCreateProjectile(ProjectileClass))
		{
			Projectile->DeactivateProjectile();
			Pools.FindOrAdd(ProjectileClass).InactiveProjectiles.Push(Projectile);
		}
	}
}

void UChronosProjectilePoolSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const double Now = FPlatformTime::Seconds();

	for (int32 Index = ActiveProjectiles.Num() - 1; Index >= 0; --Index)
	{
		AChronosProjectile* Projectile = ActiveProjectiles[Index].Projectile.Get();
		if (!IsValid(Projectile) || !Projectile->IsProjectileActive())
		{
			ActiveProjectiles.RemoveAtSwap(Index);
			continue;
		}

		if (Now - ActiveProjectiles[Index].ActivatedRealTime > MaxRealLifetime)
		{
			ReturnProjectile(Projectile);
		}
	}
}

TStatId UChronosProjectilePoolSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UChronosProjectilePoolSubsystem, STATGROUP_Tickables);
}

bool UChronosProjectilePoolSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}
