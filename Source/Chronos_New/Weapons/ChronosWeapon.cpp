#include "Weapons/ChronosWeapon.h"

#include "Characters/ChronosCharacter.h"
#include "Components/CombatComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/HealthComponent.h"
#include "GameFramework/Pawn.h"
#include "Projectiles/Projectile.h"
#include "Subsystems/ProjectilePoolSubsystem.h"
#include "Weapons/WeaponDataAsset.h"

AChronosWeapon::AChronosWeapon()
{
	PrimaryActorTick.bCanEverTick = true;

	WeaponMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMeshComponent"));
	WeaponMeshComponent->SetCollisionProfileName(TEXT("NoCollision"));
	WeaponMeshComponent->bReceivesDecals = false;
	RootComponent = WeaponMeshComponent;
}

void AChronosWeapon::BeginPlay()
{
	Super::BeginPlay();

	// 直接放置在关卡中的武器实例：开始运行时应用数据资产配置。
	// 以"未赋值弹药 + 无网格"作为未初始化标记，避免对已初始化的武器重复执行
	if (WeaponData && RemainingAmmo <= 0 && !WeaponMeshComponent->GetSkeletalMeshAsset())
	{
		InitFromData(WeaponData);
	}
}

void AChronosWeapon::InitFromData(UWeaponDataAsset* Data)
{
	WeaponData = Data;

	if (!Data)
	{
		return;
	}

	if (Data->WeaponMesh)
	{
		WeaponMeshComponent->SetSkeletalMesh(Data->WeaponMesh);
	}

	RemainingAmmo = Data->AmmoCount;

	// 初始处于世界状态（作为地面拾取物生成）
	EnterWorldState(FVector::ZeroVector);
}

void AChronosWeapon::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateRestState();
}

void AChronosWeapon::UpdateRestState()
{
	if (WeaponState != EWeaponState::InWorld)
	{
		return;
	}

	const FVector Velocity = WeaponMeshComponent->GetPhysicsLinearVelocity();
	bAtRest = !WeaponMeshComponent->IsSimulatingPhysics() ||
		Velocity.SizeSquared() < FMath::Square(10.f);
}

bool AChronosWeapon::CanFire_Implementation()
{
	return WeaponState == EWeaponState::Equipped &&
		RemainingAmmo > 0 &&
		(FPlatformTime::Seconds() - LastFireRealTime) >= RefireCooldown;
}

bool AChronosWeapon::FireAtTarget(const FVector& TargetPoint)
{
	if (!CanFire_Implementation() || !WeaponData)
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const FVector MuzzleLocation = GetMuzzleLocation_Implementation();
	FVector FireDirection = TargetPoint - MuzzleLocation;
	if (FireDirection.IsNearlyZero())
	{
		FireDirection = GetActorForwardVector();
	}
	else
	{
		FireDirection.Normalize();
	}

	// ProjectileClass 为空的武器（如榴弹发射器）由 BP OnFireVisuals 自行生成投射物
	if (WeaponData->ProjectileClass)
	{
		if (UChronosProjectilePoolSubsystem* Pool = World->GetSubsystem<UChronosProjectilePoolSubsystem>())
		{
			Pool->FireProjectile(WeaponData->ProjectileClass, GetOwner(), MuzzleLocation, FireDirection.Rotation());
		}
	}

	--RemainingAmmo;
	LastFireRealTime = FPlatformTime::Seconds();
	RefireCooldown = 1.f / WeaponData->FireRate;

	OnAmmoChanged.Broadcast(RemainingAmmo, WeaponData->AmmoCount);
	OnFireVisuals();

	return true;
}

FVector AChronosWeapon::GetMuzzleLocation_Implementation()
{
	if (WeaponMeshComponent && WeaponData &&
		WeaponMeshComponent->DoesSocketExist(WeaponData->MuzzleSocketName))
	{
		return WeaponMeshComponent->GetSocketLocation(WeaponData->MuzzleSocketName);
	}

	return GetActorLocation();
}

void AChronosWeapon::EquipTo(AChronosCharacter* NewHolder)
{
	if (!NewHolder)
	{
		return;
	}

	WeaponState = EWeaponState::Equipped;
	LastThrower = nullptr;
	SetInstigator(Cast<APawn>(NewHolder));
	SetOwner(NewHolder);

	EnterEquippedState();

	AttachToComponent(NewHolder->GetHeldWeaponAttachComponent(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		NewHolder->GetHeldWeaponSocketName());

	OnEquipVisuals();
}

//~ IWeaponUser interface implementations
void AChronosWeapon::Fire_Implementation()
{
	// 接口版本：使用枪口朝向发射
	if (!CanFire_Implementation() || !WeaponData)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector MuzzleLocation = GetMuzzleLocation_Implementation();
	const FVector FireDirection = WeaponMeshComponent->GetSocketRotation(WeaponData->MuzzleSocketName).Vector();

	// ProjectileClass 为空的武器（如榴弹发射器）由 BP OnFireVisuals 自行生成投射物
	if (WeaponData->ProjectileClass)
	{
		if (UChronosProjectilePoolSubsystem* Pool = World->GetSubsystem<UChronosProjectilePoolSubsystem>())
		{
			Pool->FireProjectile(WeaponData->ProjectileClass, GetOwner(), MuzzleLocation, FireDirection.Rotation());
		}
	}

	--RemainingAmmo;
	LastFireRealTime = FPlatformTime::Seconds();
	RefireCooldown = 1.f / WeaponData->FireRate;

	OnAmmoChanged.Broadcast(RemainingAmmo, WeaponData->AmmoCount);
	OnFireVisuals();
}

void AChronosWeapon::StopFiring_Implementation()
{
	// 半自动武器无需处理；全自动武器若需要可在此处理停止连发逻辑
}

int32 AChronosWeapon::GetCurrentAmmo_Implementation()
{
	return RemainingAmmo;
}

int32 AChronosWeapon::GetMaxAmmo_Implementation()
{
	return WeaponData ? WeaponData->AmmoCount : 0;
}

FName AChronosWeapon::GetMuzzleSocketName_Implementation()
{
	return WeaponData ? WeaponData->MuzzleSocketName : NAME_None;
}

void AChronosWeapon::ThrowWeapon_Implementation(const FVector& Direction)
{
	Throw(Direction, LastThrower.Get());
}

void AChronosWeapon::DropWeapon_Implementation()
{
	Drop(nullptr);
}

void AChronosWeapon::EnterEquippedState()
{
	WeaponState = EWeaponState::Equipped;

	WeaponMeshComponent->SetCollisionProfileName(TEXT("NoCollision"));
	WeaponMeshComponent->SetSimulatePhysics(false);
	WeaponMeshComponent->SetEnableGravity(false);
	WeaponMeshComponent->SetGenerateOverlapEvents(false);

	SetActorTickEnabled(true);
}

void AChronosWeapon::EnterWorldState(const FVector& InitialVelocity)
{
	WeaponState = EWeaponState::InWorld;
	bAtRest = false;
	LastWorldEnterRealTime = FPlatformTime::Seconds();

	SetOwner(nullptr);

	WeaponMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	WeaponMeshComponent->SetCollisionObjectType(ECC_WorldDynamic);
	WeaponMeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
	WeaponMeshComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	WeaponMeshComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	WeaponMeshComponent->SetGenerateOverlapEvents(true);
	WeaponMeshComponent->OnComponentBeginOverlap.AddUniqueDynamic(this, &AChronosWeapon::OnWeaponBeginOverlap);

	WeaponMeshComponent->SetSimulatePhysics(true);
	WeaponMeshComponent->SetEnableGravity(true);
	if (!InitialVelocity.IsZero())
	{
		WeaponMeshComponent->SetPhysicsLinearVelocity(InitialVelocity);
	}
}

void AChronosWeapon::OnWeaponHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// 预留：命中可击碎物理体的表现处理（P4 打磨）
}

void AChronosWeapon::OnWeaponBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (WeaponState != EWeaponState::InWorld || !OtherActor || OtherActor == GetOwner())
	{
		return;
	}

	// 投掷者自身免疫，避免扔出瞬间自伤（旧项目的 PI-5 教训）
	AChronosCharacter* Thrower = LastThrower.Get();
	if (Thrower && OtherActor == Thrower)
	{
		return;
	}

	const float Speed = WeaponMeshComponent->GetPhysicsLinearVelocity().Size();
	if (Speed < ThrowKillSpeedThreshold)
	{
		return;
	}

	if (UHealthComponent* TargetHealth = OtherActor->FindComponentByClass<UHealthComponent>())
	{
		TargetHealth->TakeDamage(Thrower ? Cast<AActor>(Thrower) : this);
	}
}

void AChronosWeapon::OnInteract(AChronosCharacter* Interactor)
{
	if (!CanInteract(Interactor))
	{
		return;
	}

	Interactor->GetCombatComponent()->EquipWeapon(this);
}

bool AChronosWeapon::CanInteract(const AChronosCharacter* Interactor) const
{
	if (WeaponState != EWeaponState::InWorld || !bAtRest)
	{
		return false;
	}

	// 刚被自己扔出的武器在短延迟内不可立即捡回
	if (LastThrower.IsValid() && LastThrower.Get() == Interactor)
	{
		const double Now = FPlatformTime::Seconds();
		if (Now - LastWorldEnterRealTime < MinPickupDelay)
		{
			return false;
		}
	}

	return true;
}

FText AChronosWeapon::GetInteractionText() const
{
	if (WeaponData)
	{
		return FText::Format(NSLOCTEXT("Chronos", "PickupFormat", "拾取 {0}"), WeaponData->WeaponName);
	}

	return NSLOCTEXT("Chronos", "PickupWeapon", "拾取武器");
}

void AChronosWeapon::Throw(const FVector& Direction, AChronosCharacter* Thrower)
{
	const float ThrowSpeed = WeaponData ? WeaponData->ThrowSpeed : 1500.f;
	LastThrower = Thrower;

	if (Thrower)
	{
		SetInstigator(Cast<APawn>(Thrower));
		SetOwner(Thrower);
	}

	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	EnterWorldState(Direction * ThrowSpeed);
}

void AChronosWeapon::Drop(AChronosCharacter* Dropper)
{
	LastThrower = Dropper;
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	EnterWorldState(FVector::ZeroVector);
}