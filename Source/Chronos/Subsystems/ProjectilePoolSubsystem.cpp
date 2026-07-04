// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectilePoolSubsystem.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"


UProjectilePoolSubsystem::UProjectilePoolSubsystem()
{
}

void UProjectilePoolSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UProjectilePoolSubsystem::Deinitialize()
{
	for (auto& Pair : ProjectilePoolMap)
	{
		for (AActor* Projectile : Pair.Value)
		{
			if (Projectile && !Projectile->IsPendingKillPending())
			{
				Projectile->Destroy();
			}
		}
		
		Pair.Value.Empty();
	}
	
	ProjectilePoolMap.Empty();
	ActiveProjectiles.Empty();
	
	Super::Deinitialize();
}

void UProjectilePoolSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	// 检查超时的子弹并回收
	float CurrentTime = GetWorld()->GetTimeSeconds();
	
	TArray<TObjectPtr<AActor>> ProjectilesToReturn;
	
	for (const auto& Pair : ActiveProjectiles)
	{
		if (CurrentTime - Pair.Value > MaxProjectileLifetime)
		{
			ProjectilesToReturn.Add(Pair.Key);
		}
	}
	
	for (AActor* Projectile : ProjectilesToReturn)
	{
		ReturnProjectile(Projectile);
	}
}

AActor* UProjectilePoolSubsystem::GetProjectile(TSubclassOf<AActor> ProjectileClass, const FTransform& SpawnTransform)
{
	if (!ProjectileClass || !GetWorld())
	{
		return nullptr;
	}
	
	AActor* Projectile = nullptr;
	
	// 尝试从池中获取可用子弹
	TArray<TObjectPtr<AActor>>* AvailableProjectiles = ProjectilePoolMap.Find(ProjectileClass);
	
	if (AvailableProjectiles && AvailableProjectiles->Num() > 0)
	{
		Projectile = AvailableProjectiles->Pop();
	}
	else
	{
		// 池中没有可用子弹，生成新的
		Projectile = GetWorld()->SpawnActor<AActor>(ProjectileClass, SpawnTransform);
	}
	
	if (Projectile)
	{
		// 激活子弹
		Projectile->SetActorTransform(SpawnTransform);
		Projectile->SetActorRotation(SpawnTransform.GetRotation());		
		Projectile->SetActorEnableCollision(true);
		Projectile->SetActorHiddenInGame(false);
		Projectile->SetActorTickEnabled(true);
		
		// 记录激活时间
		ActiveProjectiles.Add(Projectile, GetWorld()->GetTimeSeconds());
	}
	
	return Projectile;
}

void UProjectilePoolSubsystem::ReturnProjectile(AActor* Projectile)
{
	if (!Projectile || Projectile->IsPendingKillPending())
	{
		return;
	}
	
	ActiveProjectiles.Remove(Projectile);
	
	TSubclassOf<AActor> ProjectileClass = Projectile->GetClass();
	
	TArray<TObjectPtr<AActor>> AvailableProjectiles = ProjectilePoolMap.FindOrAdd(ProjectileClass);
	
	Projectile->SetActorEnableCollision(false);
	Projectile->SetActorHiddenInGame(true);
	Projectile->SetActorTickEnabled(false);
	
	// 将子弹移到远离场景的位置，避免碰撞检测
	Projectile->SetActorLocation(FVector(-99999.0f, -99999.0f, -99999.0f));

	// 添加到可用列表
	AvailableProjectiles.Add(Projectile);
}

void UProjectilePoolSubsystem::PreallocateProjectiles(TSubclassOf<AActor> ProjectileClass, int32 Count)
{
	if (!ProjectileClass || !GetWorld() || Count <= 0)
	{
		return;
	}
	
	TArray<TObjectPtr<AActor>>& AvailableProjectiles = ProjectilePoolMap.FindOrAdd(ProjectileClass);
	
	for (int32 i = 0; i < Count; i++)
	{
		TObjectPtr<AActor> Projectile = GetWorld()->SpawnActor<AActor>(ProjectileClass, FTransform::Identity);
		
		if (Projectile)
		{
			// 初始化为非活跃状态
			Projectile->SetActorEnableCollision(false);
			Projectile->SetActorHiddenInGame(true);
			Projectile->SetActorTickEnabled(false);
			Projectile->SetActorLocation(FVector(-99999.0f, -99999.0f, -99999.0f));

			AvailableProjectiles.Add(Projectile);
		}
	}
}

int32 UProjectilePoolSubsystem::GetAvailableProjectileCount(TSubclassOf<AActor> ProjectileClass) const
{
	const TArray<TObjectPtr<AActor>>* AvailableProjectiles = ProjectilePoolMap.Find(ProjectileClass);
	
	return AvailableProjectiles ? AvailableProjectiles->Num() : 0;
}
