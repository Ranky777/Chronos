// Fill out your copyright notice in the Description page of Project Settings.


#include "WeaponComponent.h"

#include "Chronos/Chronos.h"
#include "Chronos/DataAssets/WeaponDataAsset.h"
#include "Chronos/Subsystems/ProjectilePoolSubsystem.h"
#include "Chronos/Tags/ChronosTags.h"
#include "GameFramework/ProjectileMovementComponent.h"

// Sets default values for this component's properties
UWeaponComponent::UWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}


// Called when the game starts
void UWeaponComponent::BeginPlay()
{
	Super::BeginPlay();

	// 如果有默认武器数据，自动装备
	if (CurrentWeaponData != nullptr)
	{
		EquipWeapon(CurrentWeaponData);
	}
}

void UWeaponComponent::EquipWeapon_Implementation(UWeaponDataAsset* WeaponData)
{
	if (!WeaponData)
	{
		return;
	}
	
	// 停止正在进行的换弹
	if (IsReloading())
	{
		GetWorld()->GetTimerManager().ClearTimer(ReloadTimerHandle);
		OwnedTags.RemoveTag(FChronosTags::State_Reloading);
	}
	
	// 设置新武器
	CurrentWeaponData = WeaponData;
    
	// 装满弹药
	CurrentAmmo = CurrentWeaponData->WeaponData.MagazineSize;
	
	UpdateWeaponState();
}

UWeaponDataAsset* UWeaponComponent::GetCurrentWeapon_Implementation() const
{
	return CurrentWeaponData;
}

void UWeaponComponent::Fire_Implementation()
{
	if (!CanFire())
	{
		return;
	}
	
	// 更新射击状态
	OwnedTags.AddTag(FChronosTags::State_Shooting);
	
	CurrentAmmo--;
	
	// 记录射击时间
	LastFireTime = GetWorld()->GetTimeSeconds();

	// 生成子弹
	SpawnProjectile();

	// 触发射击事件
	OnFired.Broadcast();

	// 更新状态
	UpdateWeaponState();
}

void UWeaponComponent::Reload_Implementation()
{
	if (!CanReload())
	{
		return;
	}

	// 更新换弹状态
	OwnedTags.AddTag(FChronosTags::State_Reloading);

	// 触发换弹开始事件
	OnReloadStarted.Broadcast();

	// 设置换弹计时器
	float ReloadTime = CurrentWeaponData->WeaponData.ReloadTime;
	GetWorld()->GetTimerManager().SetTimer(ReloadTimerHandle, this, &UWeaponComponent::OnReloadTimerFinished, ReloadTime, false);
}

bool UWeaponComponent::CanFire_Implementation() const
{
	if (!CurrentWeaponData || !GetWorld())
	{
		return false;
	}

	// 检查弹药
	if (CurrentAmmo <= 0)
	{
		return false;
	}

	// 检查换弹状态
	if (OwnedTags.HasTag(FChronosTags::State_Reloading))
	{
		return false;
	}

	// 检查射速冷却
	float FireRate = CurrentWeaponData->WeaponData.FireRate;
	float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastFireTime < FireRate)
	{
		return false;
	}

	return true;
}

bool UWeaponComponent::CanReload_Implementation() const
{
	if (!CurrentWeaponData)
	{
		return false;
	}

	// 检查是否已经装满弹药
	if (CurrentAmmo >= CurrentWeaponData->WeaponData.MagazineSize)
	{
		return false;
	}

	// 检查是否正在换弹
	if (OwnedTags.HasTag(FChronosTags::State_Reloading))
	{
		return false;
	}

	return true;
}

FVector UWeaponComponent::GetMuzzleLocation_Implementation() const
{
	if (const AActor* Owner = GetOwner())
	{
		return Owner->GetActorLocation() + Owner->GetActorForwardVector() * 100.0f;
	}
	return FVector::ZeroVector;
}

FRotator UWeaponComponent::GetMuzzleRotation_Implementation() const
{
	if (const AActor* Owner = GetOwner())
	{
		return Owner->GetActorRotation();
	}
	return FRotator::ZeroRotator;
}

int32 UWeaponComponent::GetMagazineSize() const
{
	return CurrentWeaponData ? CurrentWeaponData->WeaponData.MagazineSize : 0;
}

float UWeaponComponent::GetReloadTime() const
{ 
	return CurrentWeaponData ? CurrentWeaponData->WeaponData.ReloadTime : 0.0f;
}

float UWeaponComponent::GetFireRate() const
{
	return CurrentWeaponData ? CurrentWeaponData->WeaponData.FireRate : 0.0f;
}

bool UWeaponComponent::IsReloading() const
{
	return OwnedTags.HasTag(FChronosTags::State_Reloading);
}

bool UWeaponComponent::IsShooting() const
{	
	return OwnedTags.HasTag(FChronosTags::State_Shooting);
}

void UWeaponComponent::SpawnProjectile() const
{
	if (!CurrentWeaponData || !GetWorld())
	{
		return;
	}
	
	UProjectilePoolSubsystem* PoolSubsystem = ChronosSubsystems::GetProjectilePoolSubsystem(GetWorld());
	
	if (!PoolSubsystem)
	{
		return;
	}
	
	// 获取武器数据
	const FWeaponData& WeaponData = CurrentWeaponData->WeaponData;
	
	FTransform SpawnTransform;
	SpawnTransform.SetLocation(GetMuzzleLocation());
	SpawnTransform.SetRotation(GetMuzzleRotation().Quaternion());
	
	// 从对象池获取子弹
	AActor* Projectile = PoolSubsystem->GetProjectile(WeaponData.ProjectileClass, SpawnTransform);
	
	// 如果获取到子弹，设置速度
	if (Projectile)
	{
		UProjectileMovementComponent* ProjectileMovement = Projectile->FindComponentByClass<UProjectileMovementComponent>();
		
		if (ProjectileMovement)
		{
			ProjectileMovement->InitialSpeed = WeaponData.BulletSpeed;
			ProjectileMovement->MaxSpeed = WeaponData.BulletSpeed;
		}
	}
}

void UWeaponComponent::OnReloadTimerFinished()
{
	// 清除换弹状态
	OwnedTags.RemoveTag(FChronosTags::State_Reloading);

	// 装满弹药
	if (CurrentWeaponData)
	{
		CurrentAmmo = CurrentWeaponData->WeaponData.MagazineSize;
	}

	// 更新状态
	UpdateWeaponState();

	// 触发换弹完成事件
	OnReloadCompleted.Broadcast();
}

void UWeaponComponent::UpdateWeaponState()
{
	// 根据当前状态更新GameplayTags
	if (CurrentAmmo <= 0)
	{
		// 需要换弹
		OwnedTags.RemoveTag(FChronosTags::State_Shooting);
	}
	else
	{
		// 射击状态会在Fire后自动移除（通过时间判断）
		// 这里只需要确保状态正确
	}
}
